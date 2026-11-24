/* pe-reloc.c
 *
 * objcopy --output-target=efi-app-x86_64 copies ELF sections into a PE32+
 * and sets IMAGE_FILE_RELOCS_STRIPPED. Firmware loads that image only at
 * ImageBase, and ImageBase is 0, so the load fails. R_X86_64_RELATIVE
 * entries in the ELF are the fixups. This writes them as a .reloc section
 * and points data directory 5 at that section.
 *
 * An image with no relative fixup still gets one ABSOLUTE pad. A zero
 * length base-relocation directory is what the host check rejects.
 *
 * usage: pe-reloc input.elf output.efi
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define RELATIVE 8u
#define DIR64 10u
#define ABSOLUTE 0u
#define RELOCS_STRIPPED 0x0001u
#define SHT_RELA 4u
#define PE32PLUS 0x20Bu
#define SCN_CHARS (0x00000040u | 0x40000000u | 0x02000000u)

typedef struct Buf {
    uint8_t *data;
    size_t len;
    size_t cap;
} Buf;

typedef struct Fixup {
    uint64_t rva;
    uint64_t addend;
} Fixup;

typedef struct Section {
    uint32_t vsize;
    uint32_t va;
    uint32_t rawsize;
    uint32_t rawptr;
    size_t header_off;
} Section;

static void fail(const char *msg)
{
    fprintf(stderr, "pe-reloc: %s\n", msg);
    exit(1);
}

static uint16_t ru16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t ru32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t ru64(const uint8_t *p)
{
    return (uint64_t)ru32(p) | ((uint64_t)ru32(p + 4) << 32);
}

static void wu16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void wu32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void need(size_t len, size_t off, size_t n, const char *what)
{
    if (n > len || off > len - n) {
        fprintf(stderr, "pe-reloc: truncated %s\n", what);
        exit(1);
    }
}

static size_t align_up(size_t value, size_t boundary)
{
    if (boundary == 0 || (boundary & (boundary - 1)) != 0)
        fail("section alignment is not a power of two");
    return (value + boundary - 1) & ~(boundary - 1);
}

static void buf_reserve(Buf *b, size_t want)
{
    size_t cap;
    uint8_t *grown;

    if (want <= b->cap)
        return;
    cap = b->cap ? b->cap : 4096u;
    while (cap < want) {
        if (cap > (SIZE_MAX / 2u))
            fail("out of memory");
        cap *= 2u;
    }
    grown = realloc(b->data, cap);
    if (!grown)
        fail("out of memory");
    b->data = grown;
    b->cap = cap;
}

static void buf_insert_zeros(Buf *b, size_t at, size_t n)
{
    if (at > b->len)
        fail("insert is past the end of the image");
    buf_reserve(b, b->len + n);
    memmove(b->data + at + n, b->data + at, b->len - at);
    memset(b->data + at, 0, n);
    b->len += n;
}

static void buf_append(Buf *b, const void *src, size_t n)
{
    buf_reserve(b, b->len + n);
    memcpy(b->data + b->len, src, n);
    b->len += n;
}

static void buf_append_zeros(Buf *b, size_t n)
{
    buf_reserve(b, b->len + n);
    memset(b->data + b->len, 0, n);
    b->len += n;
}

static uint8_t *read_whole(const char *path, size_t *len_out)
{
    FILE *fp;
    long sz;
    uint8_t *data;
    size_t got;

    fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "pe-reloc: cannot read %s\n", path);
        exit(1);
    }
    if (fseek(fp, 0, SEEK_END) != 0)
        fail("seek failed");
    sz = ftell(fp);
    if (sz < 0)
        fail("tell failed");
    if (fseek(fp, 0, SEEK_SET) != 0)
        fail("seek failed");
    data = malloc((size_t)sz + 1u);
    if (!data)
        fail("out of memory");
    got = fread(data, 1, (size_t)sz, fp);
    fclose(fp);
    if (got != (size_t)sz)
        fail("short read");
    *len_out = (size_t)sz;
    return data;
}

static int cmp_fixup(const void *left, const void *right)
{
    const Fixup *a = left;
    const Fixup *b = right;
    if (a->rva < b->rva)
        return -1;
    if (a->rva > b->rva)
        return 1;
    return 0;
}

static Fixup *elf_relatives(const uint8_t *elf, size_t len, size_t *count_out)
{
    uint64_t shoff;
    uint16_t shentsize;
    uint16_t shnum;
    Fixup *found;
    size_t count = 0;
    size_t cap = 8;
    unsigned index;

    need(len, 0, 64, "ELF header");
    if (elf[0] != 0x7f || elf[1] != 'E' || elf[2] != 'L' || elf[3] != 'F')
        fail("input is not an ELF file");
    if (elf[4] != 2 || elf[5] != 1)
        fail("input is not a 64-bit little-endian ELF file");
    shoff = ru64(elf + 40);
    shentsize = ru16(elf + 58);
    shnum = ru16(elf + 60);
    if (shentsize < 64)
        fail("ELF section header is shorter than 64 bytes");

    found = malloc(cap * sizeof *found);
    if (!found)
        fail("out of memory");
    for (index = 0; index < shnum; index++) {
        size_t off = (size_t)shoff + (size_t)index * shentsize;
        uint32_t sh_type;
        uint64_t sh_offset, sh_size, sh_entsize;
        uint64_t entry;

        need(len, off, 64, "ELF section header");
        sh_type = ru32(elf + off + 4);
        sh_offset = ru64(elf + off + 24);
        sh_size = ru64(elf + off + 32);
        sh_entsize = ru64(elf + off + 56);
        if (sh_type != SHT_RELA || sh_entsize != 24 || sh_size == 0)
            continue;
        if (sh_size % 24u != 0)
            fail("RELA section size is not a multiple of 24");
        need(len, (size_t)sh_offset, (size_t)sh_size, "RELA section");
        for (entry = 0; entry < sh_size / 24u; entry++) {
            size_t at = (size_t)sh_offset + (size_t)entry * 24u;
            uint64_t r_info = ru64(elf + at + 8);
            if ((uint32_t)r_info != RELATIVE)
                continue;
            if (count == cap) {
                Fixup *grown;
                cap *= 2u;
                grown = realloc(found, cap * sizeof *found);
                if (!grown)
                    fail("out of memory");
                found = grown;
            }
            found[count].rva = ru64(elf + at);
            found[count].addend = ru64(elf + at + 16);
            count++;
        }
    }
    qsort(found, count, sizeof *found, cmp_fixup);
    *count_out = count;
    return found;
}

static void blob_u16(Buf *blob, uint16_t v)
{
    uint8_t bytes[2];
    wu16(bytes, v);
    buf_append(blob, bytes, 2);
}

static void blob_u32(Buf *blob, uint32_t v)
{
    uint8_t bytes[4];
    wu32(bytes, v);
    buf_append(blob, bytes, 4);
}

/* Page-grouped base relocation blocks. A block is 8 bytes of header
 * plus 2 bytes per entry, padded to a multiple of 4 bytes with an
 * ABSOLUTE slot when the entry count is odd. */
static Buf reloc_blob(const Fixup *fixups, size_t count)
{
    Buf blob;
    size_t i;

    memset(&blob, 0, sizeof blob);
    if (count == 0) {
        blob_u32(&blob, 0);
        blob_u32(&blob, 12);
        blob_u16(&blob, ABSOLUTE);
        blob_u16(&blob, 0);
        return blob;
    }
    i = 0;
    while (i < count) {
        uint64_t page = fixups[i].rva & ~UINT64_C(0xFFF);
        size_t j = i + 1;
        size_t nent;
        size_t k;
        int pad;
        uint32_t block;

        while (j < count && (fixups[j].rva & ~UINT64_C(0xFFF)) == page)
            j++;
        nent = j - i;
        pad = (nent & 1u) ? 1 : 0;
        if ((page >> 32) != 0 || nent > 0x10000u)
            fail("relocation RVA does not fit in a PE block");
        block = (uint32_t)(8u + 2u * (nent + (size_t)pad));
        blob_u32(&blob, (uint32_t)page);
        blob_u32(&blob, block);
        for (k = i; k < j; k++) {
            uint32_t off = (uint32_t)(fixups[k].rva & 0xFFFu);
            blob_u16(&blob, (uint16_t)((DIR64 << 12) | off));
        }
        if (pad)
            blob_u16(&blob, ABSOLUTE);
        i = j;
    }
    return blob;
}

static void write_section(uint8_t *dst, uint32_t vsize, uint32_t va,
                          uint32_t rawsize, uint32_t rawptr)
{
    memset(dst, 0, 40);
    memcpy(dst, ".reloc", 6);
    wu32(dst + 8, vsize);
    wu32(dst + 12, va);
    wu32(dst + 16, rawsize);
    wu32(dst + 20, rawptr);
    wu32(dst + 36, SCN_CHARS);
}

static void add_reloc(Buf *pe, const uint8_t *blob, size_t blob_len, Section *sections,
                      uint16_t num_sections)
{
    size_t e_lfanew;
    size_t coff;
    size_t opt;
    uint16_t opt_size;
    uint32_t section_align;
    uint32_t file_align;
    uint32_t size_of_image;
    uint32_t size_of_headers;
    uint32_t dir_count;
    size_t sec_off;
    size_t headers_end;
    size_t min_raw;
    int saw_raw = 0;
    uint16_t index;
    size_t raw_ptr;
    size_t raw_size;
    uint64_t va;
    uint64_t image_end;
    uint16_t characteristics;
    size_t header_at;

    need(pe->len, 0, 0x40, "MZ header");
    if (pe->data[0] != 'M' || pe->data[1] != 'Z')
        fail("missing MZ header");
    e_lfanew = ru32(pe->data + 0x3C);
    need(pe->len, e_lfanew, 24, "PE signature");
    if (memcmp(pe->data + e_lfanew, "PE\0\0", 4) != 0)
        fail("missing PE signature");
    coff = e_lfanew + 4;
    opt_size = ru16(pe->data + coff + 16);
    opt = coff + 20;
    need(pe->len, opt, opt_size, "optional header");
    if (ru16(pe->data + opt) != PE32PLUS)
        fail("optional header is not PE32+");
    section_align = ru32(pe->data + opt + 32);
    file_align = ru32(pe->data + opt + 36);
    size_of_image = ru32(pe->data + opt + 56);
    size_of_headers = ru32(pe->data + opt + 60);
    dir_count = ru32(pe->data + opt + 108);
    if (dir_count <= 5)
        fail("PE has no base-relocation directory slot");

    sec_off = opt + opt_size;
    need(pe->len, sec_off, (size_t)num_sections * 40u, "section table");
    headers_end = sec_off + (size_t)(num_sections + 1u) * 40u;
    min_raw = pe->len;
    for (index = 0; index < num_sections; index++) {
        if (sections[index].rawptr == 0)
            continue;
        if (!saw_raw || sections[index].rawptr < min_raw)
            min_raw = sections[index].rawptr;
        saw_raw = 1;
    }
    if (!saw_raw)
        min_raw = pe->len;
    if (sec_off >= min_raw || sec_off + (size_t)num_sections * 40u > min_raw)
        fail("section table is not entirely before the raw data");

    /* One more 40-byte header has to land in the header region, and that
     * region has to end on a file alignment boundary before raw data. */
    if (headers_end > size_of_headers || headers_end > min_raw) {
        size_t new_headers = align_up(headers_end, file_align);
        size_t chosen;
        if (new_headers > min_raw) {
            size_t delta = new_headers - min_raw;
            buf_insert_zeros(pe, min_raw, delta);
            for (index = 0; index < num_sections; index++) {
                if (sections[index].rawptr == 0)
                    continue;
                sections[index].rawptr += (uint32_t)delta;
                wu32(pe->data + sections[index].header_off + 20,
                     sections[index].rawptr);
            }
            min_raw = new_headers;
        }
        chosen = (min_raw >= headers_end) ? min_raw : new_headers;
        size_of_headers = (uint32_t)(new_headers > chosen ? new_headers : chosen);
        if (size_of_headers < headers_end)
            fail("no room for a .reloc section header");
    }

    raw_ptr = align_up(pe->len, file_align);
    if (raw_ptr > pe->len)
        buf_append_zeros(pe, raw_ptr - pe->len);
    raw_size = align_up(blob_len, file_align);
    buf_append(pe, blob, blob_len);
    if (raw_size > blob_len)
        buf_append_zeros(pe, raw_size - blob_len);

    va = align_up(size_of_image, section_align);
    for (index = 0; index < num_sections; index++) {
        uint64_t end = align_up((uint64_t)sections[index].va +
                                sections[index].vsize, section_align);
        if (end > va)
            va = end;
    }
    image_end = align_up(va + blob_len, section_align);
    if (va > 0xFFFFFFFFu || image_end > 0xFFFFFFFFu || raw_ptr > 0xFFFFFFFFu ||
        raw_size > 0xFFFFFFFFu)
        fail("relocated image does not fit in a PE32+ size field");

    header_at = sec_off + (size_t)num_sections * 40u;
    need(pe->len, header_at, 40, ".reloc section header");
    write_section(pe->data + header_at, (uint32_t)blob_len, (uint32_t)va,
                  (uint32_t)raw_size, (uint32_t)raw_ptr);
    wu16(pe->data + coff + 2, (uint16_t)(num_sections + 1u));
    characteristics = ru16(pe->data + coff + 18);
    wu16(pe->data + coff + 18, (uint16_t)(characteristics & ~RELOCS_STRIPPED));
    wu32(pe->data + opt + 56, (uint32_t)image_end);
    wu32(pe->data + opt + 60, size_of_headers);
    wu32(pe->data + opt + 64, 0);
    wu32(pe->data + opt + 112 + 5 * 8, (uint32_t)va);
    wu32(pe->data + opt + 112 + 5 * 8 + 4, (uint32_t)blob_len);
}

static void check_addends(const Buf *pe, const Section *sections, uint16_t count,
                          const Fixup *fixups, size_t nfix)
{
    size_t i;

    for (i = 0; i < nfix; i++) {
        uint16_t s;
        int found = 0;
        size_t file_off = 0;
        uint64_t stored;

        for (s = 0; s < count; s++) {
            uint32_t span = sections[s].vsize > sections[s].rawsize
                                ? sections[s].vsize
                                : sections[s].rawsize;
            if ((uint64_t)sections[s].va <= fixups[i].rva &&
                fixups[i].rva < (uint64_t)sections[s].va + span) {
                file_off = (size_t)sections[s].rawptr +
                           (size_t)(fixups[i].rva - sections[s].va);
                found = 1;
                break;
            }
        }
        if (!found) {
            fprintf(stderr, "pe-reloc: fixup RVA 0x%llx is not in a copied section\n",
                    (unsigned long long)fixups[i].rva);
            exit(1);
        }
        need(pe->len, file_off, 8, "fixup addend");
        stored = ru64(pe->data + file_off);
        if (stored != fixups[i].addend) {
            fprintf(stderr,
                    "pe-reloc: fixup at 0x%llx holds 0x%llx, ELF addend is 0x%llx\n",
                    (unsigned long long)fixups[i].rva,
                    (unsigned long long)stored,
                    (unsigned long long)fixups[i].addend);
            exit(1);
        }
    }
}

static void load_sections(const uint8_t *pe, size_t len, Section *sections,
                          uint16_t *count_out, size_t *sec_off_out)
{
    size_t e_lfanew;
    size_t coff;
    uint16_t count;
    uint16_t opt_size;
    size_t sec_off;
    uint16_t index;

    need(len, 0, 0x40, "MZ header");
    e_lfanew = ru32(pe + 0x3C);
    need(len, e_lfanew, 24, "COFF header");
    coff = e_lfanew + 4;
    count = ru16(pe + coff + 2);
    opt_size = ru16(pe + coff + 16);
    if (count == 0 || count > 96)
        fail("section count is outside the range this tool accepts");
    sec_off = coff + 20 + opt_size;
    need(len, sec_off, (size_t)count * 40u, "section table");
    for (index = 0; index < count; index++) {
        size_t off = sec_off + (size_t)index * 40u;
        sections[index].vsize = ru32(pe + off + 8);
        sections[index].va = ru32(pe + off + 12);
        sections[index].rawsize = ru32(pe + off + 16);
        sections[index].rawptr = ru32(pe + off + 20);
        sections[index].header_off = off;
    }
    *count_out = count;
    *sec_off_out = sec_off;
}

int main(int argc, char **argv)
{
    uint8_t *elf;
    size_t elf_len = 0;
    Fixup *fixups;
    size_t nfix = 0;
    Buf blob;
    Buf pe;
    Section sections[96];
    uint16_t nsec = 0;
    size_t sec_off = 0;
    FILE *out;

    if (argc != 3)
        fail("usage: pe-reloc input.elf output.efi");
    elf = read_whole(argv[1], &elf_len);
    fixups = elf_relatives(elf, elf_len, &nfix);
    blob = reloc_blob(fixups, nfix);
    memset(&pe, 0, sizeof pe);
    pe.data = read_whole(argv[2], &pe.len);
    pe.cap = pe.len;
    load_sections(pe.data, pe.len, sections, &nsec, &sec_off);
    (void)sec_off;
    add_reloc(&pe, blob.data, blob.len, sections, nsec);
    check_addends(&pe, sections, nsec, fixups, nfix);

    out = fopen(argv[2], "wb");
    if (!out) {
        fprintf(stderr, "pe-reloc: cannot write %s\n", argv[2]);
        return 1;
    }
    if (fwrite(pe.data, 1, pe.len, out) != pe.len || fclose(out) != 0)
        fail("write of the EFI image failed");
    printf("%s: %zu base relocation(s), %zu byte table\n", argv[2], nfix, blob.len);
    free(elf);
    free(fixups);
    free(blob.data);
    free(pe.data);
    return 0;
}
