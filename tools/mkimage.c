/* mkimage.c
 *
 * Build the QueueOS disk images for this slice. No root and no loop
 * device: the GPT is written here, mkfs.fat formats the ESP, mtools
 * copies the tree, and qemu-img wraps qcow2.
 *
 * Two builds must match. mkfs.fat --invariant freezes the volume id.
 * mtools still stamps directory entries with the clock, so those fields
 * are cleared afterwards. Cluster size is 512 bytes because a 64 MiB
 * disk does not have enough 4 KiB clusters for FAT32, and the image is
 * specified to be FAT32.
 *
 * usage: mkimage --root DIR --build DIR --database-raw FILE
 *
 * A boot folder may include vm.conf. That names a local guest copy
 * to refresh after the images are written. Folders without the file
 * are left alone.
 */

#define _XOPEN_SOURCE 700

#include "host-hash.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

enum {
    SECTOR = 512,
    DISK_BYTES = 64 * 1024 * 1024,
    DISK_SECTORS = DISK_BYTES / SECTOR,
    ENTRY_COUNT = 128,
    ENTRY_SIZE = 128,
    ENTRY_BYTES = ENTRY_COUNT * ENTRY_SIZE,
    ENTRY_SECTORS = ENTRY_BYTES / SECTOR,
    FIRST_USABLE = 2048,
    BACKUP_LBA = DISK_SECTORS - 1,
    BACKUP_ENTRIES_LBA = BACKUP_LBA - ENTRY_SECTORS,
    LAST_USABLE = BACKUP_ENTRIES_LBA - 1,
    PART_START = FIRST_USABLE,
    PART_END = LAST_USABLE,
    QPATH = 4096,
    MAX_FOLDERS = 64
};

/* C12A7328-F81F-11D2-BA4B-00A0C93EC93B, stored in GPT mixed endian.
 * Disk and partition GUIDs are fixed so the GPT bytes do not move
 * between builds. They are slice constants, not architectural ids. */
static const uint8_t ESP_TYPE[16] = {
    0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
    0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B
};
static const uint8_t DISK_GUID[16] = {
    0x10, 0x00, 0x00, 0x00, 0x00, 0x10, 0x4E, 0x10,
    0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};
static const uint8_t PART_GUID[16] = {
    0x11, 0x00, 0x00, 0x00, 0x00, 0x10, 0x4E, 0x10,
    0x80, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

static void die(const char *msg)
{
    fprintf(stderr, "mkimage: %s\n", msg);
    exit(1);
}

static void die_errno(const char *what)
{
    fprintf(stderr, "mkimage: %s: %s\n", what, strerror(errno));
    exit(1);
}

static void path_join(char *dst, size_t dst_sz, const char *a, const char *b)
{
    int n = snprintf(dst, dst_sz, "%s/%s", a, b);
    if (n < 0 || (size_t)n >= dst_sz)
        die("path is too long");
}

static uint32_t rd16(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

static void wr64(uint8_t *p, uint64_t v)
{
    wr32(p, (uint32_t)v);
    wr32(p + 4, (uint32_t)(v >> 32));
}

static void run_cmd(char *const argv[])
{
    pid_t pid = fork();
    int status = 0;

    if (pid < 0)
        die_errno("fork");
    if (pid == 0) {
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }
    if (waitpid(pid, &status, 0) < 0)
        die_errno("waitpid");
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "mkimage: %s failed\n", argv[0]);
        exit(1);
    }
}

static uint8_t *read_file(const char *path, size_t *len_out)
{
    FILE *fp = fopen(path, "rb");
    long sz;
    uint8_t *data;
    size_t got;

    if (!fp)
        die_errno(path);
    if (fseek(fp, 0, SEEK_END) != 0)
        die_errno(path);
    sz = ftell(fp);
    if (sz < 0 || fseek(fp, 0, SEEK_SET) != 0)
        die_errno(path);
    data = malloc((size_t)sz + 1u);
    if (!data)
        die("out of memory");
    got = fread(data, 1, (size_t)sz, fp);
    fclose(fp);
    if (got != (size_t)sz)
        die(path);
    *len_out = (size_t)sz;
    return data;
}

static void write_file(const char *path, const void *data, size_t len)
{
    FILE *fp = fopen(path, "wb");
    const uint8_t *p = data;

    if (!fp)
        die_errno(path);
    while (len > 0) {
        size_t n = fwrite(p, 1, len, fp);
        if (n == 0)
            die_errno(path);
        p += n;
        len -= n;
    }
    if (fclose(fp) != 0)
        die_errno(path);
}

static void write_zeros(const char *path, size_t len)
{
    FILE *fp = fopen(path, "wb");
    uint8_t buf[65536];

    if (!fp)
        die_errno(path);
    memset(buf, 0, sizeof buf);
    while (len > 0) {
        size_t n = len < sizeof buf ? len : sizeof buf;
        if (fwrite(buf, 1, n, fp) != n)
            die_errno(path);
        len -= n;
    }
    if (fclose(fp) != 0)
        die_errno(path);
}

static void copy_file(const char *src, const char *dst)
{
    size_t len = 0;
    uint8_t *data = read_file(src, &len);
    write_file(dst, data, len);
    free(data);
}

static void mkdir_p(const char *path)
{
    char buf[QPATH];
    size_t len = strlen(path);
    size_t i;

    if (len == 0 || len >= sizeof buf)
        die("path is too long");
    memcpy(buf, path, len + 1u);
    for (i = 1; i < len; i++) {
        if (buf[i] != '/')
            continue;
        buf[i] = '\0';
        if (mkdir(buf, 0755) != 0 && errno != EEXIST)
            die_errno(buf);
        buf[i] = '/';
    }
    if (mkdir(buf, 0755) != 0 && errno != EEXIST)
        die_errno(buf);
}

static int rm_entry(const char *fpath, const struct stat *sb, int typeflag,
                    struct FTW *ftw)
{
    (void)sb;
    (void)typeflag;
    (void)ftw;
    if (remove(fpath) != 0) {
        fprintf(stderr, "mkimage: remove %s: %s\n", fpath, strerror(errno));
        return -1;
    }
    return 0;
}

static void rm_rf(const char *path)
{
    struct stat st;

    if (lstat(path, &st) != 0) {
        if (errno == ENOENT)
            return;
        die_errno(path);
    }
    if (nftw(path, rm_entry, 16, FTW_DEPTH | FTW_PHYS) != 0)
        die("failed to remove the staged ESP tree");
}

static void hash_file(const char *path, char hex[65])
{
    if (queueos_sha256_file(path, hex) != 0)
        die_errno(path);
}

static void manifest_row(FILE *fp, const char *digest, const char *name)
{
    if (fprintf(fp, "%s  %s\n", digest, name) < 0)
        die("manifest write failed");
}

static void read_version(const char *path, char *out, size_t out_sz)
{
    FILE *fp = fopen(path, "r");
    size_t n;
    size_t i;

    if (!fp)
        die_errno(path);
    if (!fgets(out, (int)out_sz, fp))
        die("images/database/current has no version");
    fclose(fp);
    n = strlen(out);
    while (n > 0 && (out[n - 1] == '\n' || out[n - 1] == '\r' ||
                     out[n - 1] == ' ' || out[n - 1] == '\t'))
        out[--n] = '\0';
    i = 0;
    while (out[i] == ' ' || out[i] == '\t')
        i++;
    if (i > 0)
        memmove(out, out + i, strlen(out + i) + 1u);
    if (out[0] == '\0')
        die("images/database/current has no version");
    if (strchr(out, '/') != NULL || strchr(out, '\\') != NULL ||
        strcmp(out, ".") == 0 || strcmp(out, "..") == 0)
        die("database version is not a single path segment");
}

static void gpt_header(uint8_t header[92], uint64_t current, uint64_t backup,
                       uint64_t entries_lba, const uint8_t *entries)
{
    memset(header, 0, 92);
    memcpy(header, "EFI PART", 8);
    wr32(header + 8, 0x00010000u);
    wr32(header + 12, 92);
    wr64(header + 24, current);
    wr64(header + 32, backup);
    wr64(header + 40, FIRST_USABLE);
    wr64(header + 48, LAST_USABLE);
    memcpy(header + 56, DISK_GUID, 16);
    wr64(header + 72, entries_lba);
    wr32(header + 80, ENTRY_COUNT);
    wr32(header + 84, ENTRY_SIZE);
    wr32(header + 88, queueos_crc32(entries, ENTRY_BYTES));
    wr32(header + 16, queueos_crc32(header, 92));
}

static void write_gpt(uint8_t *disk)
{
    uint8_t *entries = calloc(1, ENTRY_BYTES);
    uint8_t header[92];

    if (!entries)
        die("out of memory");
    memcpy(entries, ESP_TYPE, 16);
    memcpy(entries + 16, PART_GUID, 16);
    wr64(entries + 32, PART_START);
    wr64(entries + 40, PART_END);
    {
        static const char name[] = "QueueOS-ESP";
        size_t i;
        for (i = 0; name[i] != '\0'; i++) {
            entries[56 + i * 2] = (uint8_t)name[i];
            entries[57 + i * 2] = 0;
        }
    }

    memset(disk, 0, SECTOR);
    disk[446] = 0x00;
    disk[447] = 0x00;
    disk[448] = 0x02;
    disk[449] = 0x00;
    disk[450] = 0xEE;
    disk[451] = 0xFF;
    disk[452] = 0xFF;
    disk[453] = 0xFF;
    wr32(disk + 454, 1);
    wr32(disk + 458, DISK_SECTORS - 1);
    disk[510] = 0x55;
    disk[511] = 0xAA;

    gpt_header(header, 1, BACKUP_LBA, 2, entries);
    memcpy(disk + SECTOR, header, 92);
    memcpy(disk + 2 * SECTOR, entries, ENTRY_BYTES);
    memcpy(disk + (size_t)BACKUP_ENTRIES_LBA * SECTOR, entries, ENTRY_BYTES);
    gpt_header(header, BACKUP_LBA, 1, BACKUP_ENTRIES_LBA, entries);
    memcpy(disk + (size_t)BACKUP_LBA * SECTOR, header, 92);
    free(entries);
}

static void format_partition(const char *raw_path)
{
    size_t part_bytes = (size_t)(PART_END - PART_START + 1) * SECTOR;
    char part_path[QPATH];
    size_t raw_len = 0;
    uint8_t *disk;
    uint8_t *part;
    size_t part_len = 0;
    char *argv[10];
    const char *dot;

    dot = strrchr(raw_path, '.');
    if (!dot || strcmp(dot, ".raw") != 0)
        die("ESP raw image must end in .raw");
    if ((size_t)(dot - raw_path) + strlen(".part") + 1u >= sizeof part_path)
        die("path is too long");
    memcpy(part_path, raw_path, (size_t)(dot - raw_path));
    memcpy(part_path + (dot - raw_path), ".part", 6);

    write_zeros(part_path, part_bytes);
    argv[0] = "mkfs.fat";
    argv[1] = "-F";
    argv[2] = "32";
    argv[3] = "-s";
    argv[4] = "1";
    argv[5] = "-n";
    argv[6] = "QOS-ESP";
    argv[7] = "--invariant";
    argv[8] = part_path;
    argv[9] = NULL;
    run_cmd(argv);

    disk = read_file(raw_path, &raw_len);
    part = read_file(part_path, &part_len);
    if (raw_len < (size_t)PART_START * SECTOR + part_bytes || part_len != part_bytes)
        die("formatted ESP does not match the partition size");
    memcpy(disk + (size_t)PART_START * SECTOR, part, part_bytes);
    write_file(raw_path, disk, raw_len);
    free(disk);
    free(part);
    if (unlink(part_path) != 0)
        die_errno(part_path);
}

static void stage_tree(const char *stage, const char *boot_efi, const char *base_efi)
{
    char current[QPATH], previous[QPATH], rescue[QPATH], boot_dir[QPATH];
    char path[QPATH];

    rm_rf(stage);
    path_join(current, sizeof current, stage, "EFI/QueueOS/current");
    path_join(previous, sizeof previous, stage, "EFI/QueueOS/previous");
    path_join(rescue, sizeof rescue, stage, "EFI/QueueOS/rescue");
    path_join(boot_dir, sizeof boot_dir, stage, "EFI/BOOT");
    mkdir_p(current);
    mkdir_p(previous);
    mkdir_p(rescue);
    mkdir_p(boot_dir);

    path_join(path, sizeof path, boot_dir, "BOOTX64.EFI");
    copy_file(boot_efi, path);
    path_join(path, sizeof path, current, "Boot.efi");
    copy_file(boot_efi, path);
    path_join(path, sizeof path, current, "Base.efi");
    copy_file(base_efi, path);
    path_join(path, sizeof path, previous, ".keep");
    write_file(path, "", 0);
    path_join(path, sizeof path, rescue, "Boot.efi");
    copy_file(boot_efi, path);
    path_join(path, sizeof path, rescue, "RESCUE");
    write_file(path, "rescue\n", 7);
}

static void copy_tree(const char *raw, const char *stage)
{
    char spec[QPATH];
    static const char *dirs[] = {
        "::/EFI",
        "::/EFI/BOOT",
        "::/EFI/QueueOS",
        "::/EFI/QueueOS/current",
        "::/EFI/QueueOS/previous",
        "::/EFI/QueueOS/rescue"
    };
    static const char *files[][2] = {
        {"EFI/BOOT/BOOTX64.EFI", "::/EFI/BOOT/BOOTX64.EFI"},
        {"EFI/QueueOS/current/Boot.efi", "::/EFI/QueueOS/current/Boot.efi"},
        {"EFI/QueueOS/current/Base.efi", "::/EFI/QueueOS/current/Base.efi"},
        {"EFI/QueueOS/previous/.keep", "::/EFI/QueueOS/previous/.keep"},
        {"EFI/QueueOS/rescue/Boot.efi", "::/EFI/QueueOS/rescue/Boot.efi"},
        {"EFI/QueueOS/rescue/RESCUE", "::/EFI/QueueOS/rescue/RESCUE"}
    };
    size_t i;

    if (snprintf(spec, sizeof spec, "%s@@1M", raw) >= (int)sizeof spec)
        die("path is too long");
    for (i = 0; i < sizeof dirs / sizeof dirs[0]; i++) {
        char *argv[] = {"mmd", "-i", spec, (char *)dirs[i], NULL};
        run_cmd(argv);
    }
    for (i = 0; i < sizeof files / sizeof files[0]; i++) {
        char host[QPATH];
        char *argv[7];
        path_join(host, sizeof host, stage, files[i][0]);
        argv[0] = "mcopy";
        argv[1] = "-i";
        argv[2] = spec;
        argv[3] = "-m";
        argv[4] = host;
        argv[5] = (char *)files[i][1];
        argv[6] = NULL;
        run_cmd(argv);
    }
}

/* mtools writes the current clock into short directory entries. Zero the
 * timestamp bytes (13-19 and 22-25). Long-name entries and deleted
 * entries are left alone. A 0x00 marker ends that directory. */
static void clear_directory(uint8_t *data, size_t len, size_t fat_off,
                            size_t data_off, uint32_t bps, uint32_t spc,
                            uint32_t cluster, int depth)
{
    uint32_t guard = 0;
    static const unsigned zeros[] = {13, 14, 15, 16, 17, 18, 19, 22, 23, 24, 25};
    static const char dot[11] = {'.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    static const char dotdot[11] = {'.', '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};

    if (cluster < 2)
        return;
    if (depth > 16)
        die("FAT directory nest is too deep");
    while (cluster >= 2) {
        size_t start;
        size_t span;
        size_t slot;
        size_t fat_at;
        uint32_t value;

        if (++guard > 1000000u)
            die("FAT cluster chain is cyclic");
        start = data_off + (size_t)(cluster - 2u) * spc * bps;
        span = (size_t)spc * bps;
        if (span > len || start > len - span)
            die("FAT directory is outside the disk");
        for (slot = 0; slot < span; slot += 32) {
            size_t entry = start + slot;
            uint8_t marker = data[entry];
            uint8_t attr;
            size_t z;

            if (marker == 0x00)
                return;
            if (marker == 0xE5)
                continue;
            attr = data[entry + 11];
            if ((attr & 0x0Fu) == 0x0Fu)
                continue;
            for (z = 0; z < sizeof zeros / sizeof zeros[0]; z++)
                data[entry + zeros[z]] = 0;
            if ((attr & 0x10u) != 0 &&
                memcmp(data + entry, dot, 11) != 0 &&
                memcmp(data + entry, dotdot, 11) != 0) {
                uint32_t child = rd16(data + entry + 26) |
                                 (rd16(data + entry + 20) << 16);
                clear_directory(data, len, fat_off, data_off, bps, spc, child,
                                depth + 1);
            }
        }
        fat_at = fat_off + (size_t)cluster * 4u;
        if (fat_at > len - 4u)
            die("FAT entry is outside the disk");
        value = rd32(data + fat_at) & 0x0FFFFFFFu;
        if (value < 2u || value >= 0x0FFFFFF8u)
            return;
        cluster = value;
    }
}

static void clear_fat_times(const char *raw_path)
{
    size_t len = 0;
    uint8_t *data = read_file(raw_path, &len);
    size_t base = (size_t)PART_START * SECTOR;
    uint32_t bps, spc, reserved, fats, spf, root;
    size_t fat_off, data_off;

    if (base > len || len - base < 512u)
        die("ESP boot sector is missing");
    bps = rd16(data + base + 11);
    spc = data[base + 13];
    reserved = rd16(data + base + 14);
    fats = data[base + 16];
    spf = rd32(data + base + 36);
    root = rd32(data + base + 44);
    if (bps == 0 || spc == 0 || fats == 0)
        die("ESP boot sector is not a FAT volume");
    fat_off = base + (size_t)reserved * bps;
    data_off = fat_off + (size_t)fats * spf * bps;
    clear_directory(data, len, fat_off, data_off, bps, spc, root, 0);
    write_file(raw_path, data, len);
    free(data);
}

/* raw_path is the database disk db-image already filled. This function
 * only wraps qcow2. It does not know the slot format. */
static void ensure_database(const char *db_dir, const char *raw_path)
{
    char image[QPATH];
    char manifest[QPATH];
    char digest[65];
    struct stat st;
    FILE *fp;
    char *argv[7];

    mkdir_p(db_dir);
    path_join(image, sizeof image, db_dir, "QueueOS-Database.qcow2");
    path_join(manifest, sizeof manifest, db_dir, "manifest.sha256");
    if (lstat(image, &st) == 0) {
        if (unlink(image) != 0)
            die_errno(image);
    } else if (errno != ENOENT) {
        die_errno(image);
    }
    argv[0] = "qemu-img";
    argv[1] = "convert";
    argv[2] = "-O";
    argv[3] = "qcow2";
    argv[4] = (char *)raw_path;
    argv[5] = image;
    argv[6] = NULL;
    run_cmd(argv);
    hash_file(image, digest);
    fp = fopen(manifest, "w");
    if (!fp)
        die_errno(manifest);
    manifest_row(fp, digest, "QueueOS-Database.qcow2");
    if (fclose(fp) != 0)
        die_errno(manifest);
}

static int cmp_names(const void *left, const void *right)
{
    return strcmp(*(char *const *)left, *(char *const *)right);
}

static size_t boot_folders(const char *images, char *names[MAX_FOLDERS])
{
    DIR *dir = opendir(images);
    struct dirent *ent;
    size_t count = 0;

    if (!dir)
        die_errno(images);
    while ((ent = readdir(dir)) != NULL) {
        char conf[QPATH];
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;
        path_join(conf, sizeof conf, images, ent->d_name);
        {
            char file[QPATH];
            path_join(file, sizeof file, conf, "image.conf");
            if (access(file, F_OK) != 0)
                continue;
        }
        if (count == MAX_FOLDERS)
            die("too many boot folders");
        names[count] = strdup(ent->d_name);
        if (!names[count])
            die("out of memory");
        count++;
    }
    closedir(dir);
    if (count == 0)
        die("no images/*/image.conf");
    qsort(names, count, sizeof names[0], cmp_names);
    return count;
}

static void write_boot_image(const char *folder, const char *name, const char *raw,
                             const char *stage, const char *version)
{
    char uefi[QPATH];
    char manifest_path[QPATH];
    char link[QPATH];
    char relative[QPATH];
    char digest[65];
    FILE *fp;
    struct stat st;
    static const char *payloads[] = {
        "EFI/BOOT/BOOTX64.EFI",
        "EFI/QueueOS/current/Boot.efi",
        "EFI/QueueOS/current/Base.efi",
        "EFI/QueueOS/rescue/Boot.efi"
    };
    size_t i;
    char *convert_argv[] = {
        "qemu-img", "convert", "-O", "qcow2", (char *)raw, uefi, NULL
    };

    path_join(uefi, sizeof uefi, folder, "QueueOS-UEFI.qcow2");
    run_cmd(convert_argv);
    path_join(manifest_path, sizeof manifest_path, folder, "manifest.sha256");
    fp = fopen(manifest_path, "w");
    if (!fp)
        die_errno(manifest_path);
    hash_file(uefi, digest);
    manifest_row(fp, digest, "QueueOS-UEFI.qcow2");
    for (i = 0; i < sizeof payloads / sizeof payloads[0]; i++) {
        char host[QPATH];
        path_join(host, sizeof host, stage, payloads[i]);
        hash_file(host, digest);
        manifest_row(fp, digest, payloads[i]);
    }
    if (fclose(fp) != 0)
        die_errno(manifest_path);

    if (snprintf(relative, sizeof relative, "../database/%s/QueueOS-Database.qcow2",
                 version) >= (int)sizeof relative)
        die("path is too long");
    path_join(link, sizeof link, folder, "QueueOS-Database.qcow2");
    if (lstat(link, &st) == 0) {
        if (unlink(link) != 0)
            die_errno(link);
    } else if (errno != ENOENT) {
        die_errno(link);
    }
    if (symlink(relative, link) != 0)
        die_errno(link);
    printf("%s: UEFI image, database -> %s\n", name, relative);
}

/* vm.conf sits beside image.conf. It names a local Virtual Machine
 * Manager guest whose disk copies should track this folder. The
 * builder does not define that guest. OVMF_VARS.fd in the directory
 * is firmware NVRAM, so only the two qcow2 files are copied, and only
 * when their bytes differ. The database source is the resolved
 * symlink target. Copying the symlink itself would store a path that
 * is relative to images/ and wrong under the guest directory.
 *
 * A missing vm.conf means this folder has no guest copy. A missing
 * directory is one line and not a build failure, so another checkout
 * still builds. A present file without name or directory is an error.
 * The copy runs when virsh reports "shut off", and when virsh cannot
 * see the domain. Any other state leaves the files alone.
 */

struct vm_conf {
    char name[256];
    char directory[QPATH];
};

static void trim_ends(char *s)
{
    char *end;
    size_t lead = 0;

    while (s[lead] == ' ' || s[lead] == '\t')
        lead++;
    if (lead > 0)
        memmove(s, s + lead, strlen(s + lead) + 1);
    end = s + strlen(s);
    while (end > s && (end[-1] == '\n' || end[-1] == '\r' ||
                       end[-1] == ' ' || end[-1] == '\t'))
        *--end = '\0';
}

static void copy_value(char *dst, size_t dst_sz, const char *src, const char *what)
{
    size_t n = strlen(src);

    if (n == 0 || n + 1 > dst_sz)
        die(what);
    memcpy(dst, src, n + 1);
}

/* 1 when the folder names a guest copy. 0 when vm.conf is absent. */
static int read_vm_conf(const char *folder, struct vm_conf *vm)
{
    char path[QPATH];
    FILE *fp;
    char line[QPATH];
    int saw_name = 0;
    int saw_dir = 0;

    path_join(path, sizeof path, folder, "vm.conf");
    fp = fopen(path, "r");
    if (!fp) {
        if (errno == ENOENT)
            return 0;
        die_errno(path);
    }
    memset(vm, 0, sizeof *vm);
    while (fgets(line, sizeof line, fp) != NULL) {
        char *eq;
        if (strchr(line, '\n') == NULL && !feof(fp))
            die("vm.conf line is too long");
        trim_ends(line);
        if (line[0] == '\0' || line[0] == '#')
            continue;
        eq = strchr(line, '=');
        if (!eq)
            die("vm.conf: expected name=value");
        *eq = '\0';
        trim_ends(line);
        trim_ends(eq + 1);
        if (strcmp(line, "name") == 0) {
            copy_value(vm->name, sizeof vm->name, eq + 1,
                       "vm.conf name is empty or too long");
            saw_name = 1;
        } else if (strcmp(line, "directory") == 0) {
            copy_value(vm->directory, sizeof vm->directory, eq + 1,
                       "vm.conf directory is empty or too long");
            saw_dir = 1;
        } else {
            die("vm.conf: unknown key");
        }
    }
    if (ferror(fp))
        die_errno(path);
    fclose(fp);
    if (!saw_name || !saw_dir)
        die("vm.conf needs name and directory");
    return 1;
}

static void read_all_text(int fd, char *out, size_t out_sz)
{
    size_t got = 0;
    char sink[256];

    if (out_sz == 0)
        die("vm state buffer");
    for (;;) {
        ssize_t n;
        if (got + 1 < out_sz)
            n = read(fd, out + got, out_sz - 1 - got);
        else
            n = read(fd, sink, sizeof sink);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            die_errno("virsh");
        }
        if (n == 0)
            break;
        if (got + 1 < out_sz)
            got += (size_t)n;
    }
    out[got] = '\0';
}

static void keep_first_line(char *s)
{
    char *nl = strpbrk(s, "\r\n");

    if (nl)
        *nl = '\0';
    trim_ends(s);
}

/* 1 when the copies may be replaced. virsh failure counts as allowed:
 * the domain is undefined here, or virsh is not installed. */
static int guest_allows_refresh(const char *domain, char *state, size_t state_sz)
{
    int fds[2];
    pid_t pid;
    int status;
    char *argv[] = {
        "virsh", "-c", "qemu:///system", "domstate", (char *)domain, NULL
    };

    state[0] = '\0';
    if (pipe(fds) != 0)
        die_errno("pipe");
    pid = fork();
    if (pid < 0)
        die_errno("fork");
    if (pid == 0) {
        int devnull;
        if (dup2(fds[1], STDOUT_FILENO) < 0)
            _exit(127);
        close(fds[0]);
        close(fds[1]);
        devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execvp(argv[0], argv);
        _exit(127);
    }
    close(fds[1]);
    read_all_text(fds[0], state, state_sz);
    close(fds[0]);
    if (waitpid(pid, &status, 0) < 0)
        die_errno("waitpid");
    keep_first_line(state);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return 1;
    return strcmp(state, "shut off") == 0;
}

static void refresh_disk(const char *folder_name, const char *src,
                         const char *dir, const char *file)
{
    char dest[QPATH];
    char tmp[QPATH];
    char src_hex[65];
    char dst_hex[65];

    path_join(dest, sizeof dest, dir, file);
    if (queueos_sha256_file(src, src_hex) != 0)
        die_errno(src);
    dst_hex[0] = '\0';
    if (queueos_sha256_file(dest, dst_hex) == 0) {
        if (strcmp(src_hex, dst_hex) == 0) {
            printf("%s: %s already matches\n", folder_name, file);
            return;
        }
    } else if (errno != ENOENT) {
        die_errno(dest);
    }
    if (snprintf(tmp, sizeof tmp, "%s.tmp", dest) >= (int)sizeof tmp)
        die("path is too long");
    copy_file(src, tmp);
    if (rename(tmp, dest) != 0) {
        int saved = errno;
        unlink(tmp);
        errno = saved;
        die_errno(dest);
    }
    printf("%s: %s refreshed\n", folder_name, file);
}

static void refresh_vm_copy(const char *folder, const char *folder_name)
{
    struct vm_conf vm;
    struct stat st;
    char state[128];
    char link[QPATH];
    char resolved[QPATH];
    char uefi[QPATH];

    if (!read_vm_conf(folder, &vm))
        return;
    if (stat(vm.directory, &st) != 0) {
        if (errno == ENOENT || errno == ENOTDIR) {
            printf("%s: vm directory is not on this machine\n", folder_name);
            return;
        }
        die_errno(vm.directory);
    }
    if (!S_ISDIR(st.st_mode))
        die("vm.conf directory is not a directory");
    if (!guest_allows_refresh(vm.name, state, sizeof state)) {
        printf("%s: guest \"%s\" is %s; left the copies\n",
               folder_name, vm.name, state[0] ? state : "not shut off");
        return;
    }
    path_join(uefi, sizeof uefi, folder, "QueueOS-UEFI.qcow2");
    refresh_disk(folder_name, uefi, vm.directory, "QueueOS-UEFI.qcow2");
    path_join(link, sizeof link, folder, "QueueOS-Database.qcow2");
    if (realpath(link, resolved) == NULL)
        die_errno(link);
    refresh_disk(folder_name, resolved, vm.directory, "QueueOS-Database.qcow2");
}

int main(int argc, char **argv)
{
    const char *root = NULL;
    const char *build = NULL;
    const char *database_raw = NULL;
    char images[QPATH];
    char current_path[QPATH];
    char version[128];
    char db_dir[QPATH];
    char boot_efi[QPATH];
    char base_efi[QPATH];
    char stage[QPATH];
    char raw[QPATH];
    char *folders[MAX_FOLDERS];
    size_t count;
    size_t i;
    uint8_t *disk;
    struct stat st;
    int arg;

    for (arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "--root") == 0 && arg + 1 < argc)
            root = argv[++arg];
        else if (strcmp(argv[arg], "--build") == 0 && arg + 1 < argc)
            build = argv[++arg];
        else if (strcmp(argv[arg], "--database-raw") == 0 && arg + 1 < argc)
            database_raw = argv[++arg];
        else
            die("usage: mkimage --root DIR --build DIR --database-raw FILE");
    }
    if (!root || !build || !database_raw)
        die("usage: mkimage --root DIR --build DIR --database-raw FILE");

    path_join(images, sizeof images, root, "images");
    path_join(current_path, sizeof current_path, images, "database/current");
    read_version(current_path, version, sizeof version);
    path_join(db_dir, sizeof db_dir, images, "database");
    {
        char version_dir[QPATH];
        path_join(version_dir, sizeof version_dir, db_dir, version);
        ensure_database(version_dir, database_raw);
    }

    path_join(boot_efi, sizeof boot_efi, build, "efi/Boot.efi");
    path_join(base_efi, sizeof base_efi, build, "efi/Base.efi");
    if (stat(boot_efi, &st) != 0 || stat(base_efi, &st) != 0)
        die("build the EFI stubs before the images");

    path_join(stage, sizeof stage, build, "esp-stage");
    stage_tree(stage, boot_efi, base_efi);
    path_join(raw, sizeof raw, build, "esp.raw");
    disk = calloc(1, DISK_BYTES);
    if (!disk)
        die("out of memory");
    write_gpt(disk);
    write_file(raw, disk, DISK_BYTES);
    free(disk);
    format_partition(raw);
    copy_tree(raw, stage);
    clear_fat_times(raw);

    count = boot_folders(images, folders);
    for (i = 0; i < count; i++) {
        char folder[QPATH];
        path_join(folder, sizeof folder, images, folders[i]);
        write_boot_image(folder, folders[i], raw, stage, version);
        refresh_vm_copy(folder, folders[i]);
        free(folders[i]);
    }
    return 0;
}
