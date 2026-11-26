#include "slot-text.h"

#include "publish.h"
#include "sha256.h"

#include <string.h>

/* Reads and writes one generation slot. The hash covers the JSON bytes
 * only, so a later booted line does not change it. The publisher and the
 * image writer share this text. Firmware Boot parses with Jansson.
 */

typedef struct Scan {
    const char *p;
    const char *end;
} Scan;

static int stamp_ok(const char *stamp)
{
    const unsigned char *p;

    if (stamp == NULL || stamp[0] == '\0')
        return 0;
    for (p = (const unsigned char *)stamp; *p != '\0'; p++) {
        if (*p <= 0x20 || *p >= 0x7f)
            return 0;
    }
    return 1;
}

int queueos_slot_write(uint8_t slot[QUEUEOS_SLOT_BYTES],
                       const char *json, size_t json_len,
                       const char *booted_stamp)
{
    uint8_t digest[32];
    char hex[65];
    size_t used;
    size_t stamp_len = 0;

    if (slot == NULL || json == NULL || json_len == 0)
        return -1;
    if (memchr(json, '\n', json_len) != NULL || memchr(json, '\0', json_len) != NULL)
        return -1;
    if (booted_stamp != NULL) {
        if (!stamp_ok(booted_stamp))
            return -1;
        stamp_len = strlen(booted_stamp);
    }
    /* JSON, newline, "sha256 " + 64 hex, newline, and maybe the booted line. */
    used = json_len + 1u + 7u + 64u + 1u;
    if (stamp_len > 0)
        used += 7u + stamp_len + 1u;
    if (used > QUEUEOS_SLOT_BYTES)
        return -1;

    queueos_sha256((const uint8_t *)json, json_len, digest);
    queueos_sha256_hex(digest, hex);
    memset(slot, 0, QUEUEOS_SLOT_BYTES);
    memcpy(slot, json, json_len);
    used = json_len;
    slot[used++] = '\n';
    memcpy(slot + used, "sha256 ", 7);
    used += 7;
    memcpy(slot + used, hex, 64);
    used += 64;
    slot[used++] = '\n';
    if (stamp_len > 0) {
        memcpy(slot + used, "booted ", 7);
        used += 7;
        memcpy(slot + used, booted_stamp, stamp_len);
        used += stamp_len;
        slot[used++] = '\n';
    }
    return 0;
}

int queueos_disk_empty(uint8_t *image, size_t image_len)
{
    int i;

    if (image == NULL || image_len < QUEUEOS_SLOT_REGION)
        return -1;
    memset(image, 0, image_len);
    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        if (queueos_slot_write(image + (size_t)i * QUEUEOS_SLOT_BYTES, "{}", 2, NULL) != 0)
            return -1;
    }
    return 0;
}

static void skip_ws(Scan *s)
{
    while (s->p < s->end) {
        char c = *s->p;
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r')
            break;
        s->p++;
    }
}

static int peek(Scan *s)
{
    skip_ws(s);
    if (s->p >= s->end)
        return -1;
    return (unsigned char)*s->p;
}

static int take(Scan *s, char expect)
{
    if (peek(s) != (unsigned char)expect)
        return -1;
    s->p++;
    return 0;
}

static int hex_nibble(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int store_byte(char *out, size_t cap, size_t *n, unsigned char b, int *fit)
{
    if (out == NULL || *n + 1u >= cap) {
        if (fit != NULL)
            *fit = 0;
        return 0;
    }
    out[(*n)++] = (char)b;
    out[*n] = '\0';
    return 0;
}

static int store_cp(char *out, size_t cap, size_t *n, unsigned cp, int *fit)
{
    unsigned char tmp[3];
    int len;
    int i;

    if (cp < 0x80u) {
        tmp[0] = (unsigned char)cp;
        len = 1;
    } else if (cp < 0x800u) {
        tmp[0] = (unsigned char)(0xC0u | (cp >> 6));
        tmp[1] = (unsigned char)(0x80u | (cp & 0x3Fu));
        len = 2;
    } else {
        tmp[0] = (unsigned char)(0xE0u | (cp >> 12));
        tmp[1] = (unsigned char)(0x80u | ((cp >> 6) & 0x3Fu));
        tmp[2] = (unsigned char)(0x80u | (cp & 0x3Fu));
        len = 3;
    }
    for (i = 0; i < len; i++) {
        if (store_byte(out, cap, n, tmp[i], fit) != 0)
            return -1;
    }
    return 0;
}

/* Decodes one JSON string. out may be NULL to discard it. *fit is 0
 * when the decoded text did not fit in out. The string is still consumed. */
static int parse_string(Scan *s, char *out, size_t cap, int *fit)
{
    size_t n = 0;
    int local_fit = 1;

    if (take(s, '"') != 0)
        return -1;
    if (out != NULL && cap > 0)
        out[0] = '\0';
    while (s->p < s->end) {
        unsigned char c = (unsigned char)*s->p++;
        if (c == '"') {
            if (fit != NULL)
                *fit = local_fit;
            return 0;
        }
        if (c == '\\') {
            unsigned char e;
            if (s->p >= s->end)
                return -1;
            e = (unsigned char)*s->p++;
            if (e == '"' || e == '\\' || e == '/') {
                c = e;
            } else if (e == 'b') {
                c = '\b';
            } else if (e == 'f') {
                c = '\f';
            } else if (e == 'n') {
                c = '\n';
            } else if (e == 'r') {
                c = '\r';
            } else if (e == 't') {
                c = '\t';
            } else if (e == 'u') {
                unsigned cp = 0;
                int k;
                for (k = 0; k < 4; k++) {
                    int nib;
                    if (s->p >= s->end)
                        return -1;
                    nib = hex_nibble((unsigned char)*s->p++);
                    if (nib < 0)
                        return -1;
                    cp = (cp << 4) | (unsigned)nib;
                }
                /* A surrogate pair is not a QueueOS generation. The slot
                 * is unreadable rather than a candidate we might start. */
                if (cp >= 0xD800u && cp <= 0xDFFFu)
                    return -1;
                if (store_cp(out, cap, &n, cp, &local_fit) != 0)
                    return -1;
                continue;
            } else {
                return -1;
            }
        } else if (c < 0x20u) {
            return -1;
        }
        if (store_byte(out, cap, &n, c, &local_fit) != 0)
            return -1;
    }
    return -1;
}

static int skip_value(Scan *s, int depth);

static int skip_number(Scan *s)
{
    skip_ws(s);
    if (s->p < s->end && *s->p == '-')
        s->p++;
    if (s->p >= s->end || *s->p < '0' || *s->p > '9')
        return -1;
    if (*s->p == '0') {
        s->p++;
        if (s->p < s->end && *s->p >= '0' && *s->p <= '9')
            return -1;
    } else {
        while (s->p < s->end && *s->p >= '0' && *s->p <= '9')
            s->p++;
    }
    if (s->p < s->end && *s->p == '.') {
        s->p++;
        if (s->p >= s->end || *s->p < '0' || *s->p > '9')
            return -1;
        while (s->p < s->end && *s->p >= '0' && *s->p <= '9')
            s->p++;
    }
    if (s->p < s->end && (*s->p == 'e' || *s->p == 'E')) {
        s->p++;
        if (s->p < s->end && (*s->p == '+' || *s->p == '-'))
            s->p++;
        if (s->p >= s->end || *s->p < '0' || *s->p > '9')
            return -1;
        while (s->p < s->end && *s->p >= '0' && *s->p <= '9')
            s->p++;
    }
    return 0;
}

/* Whole number, no sign, no fraction. *ok is 0 when the token was a
 * JSON number but not a byte offset we can keep. */
static int parse_u64(Scan *s, int *ok, uint64_t *value)
{
    const char *begin;
    const char *q;
    uint64_t v = 0;
    int c;

    *ok = 0;
    skip_ws(s);
    begin = s->p;
    c = peek(s);
    if (c < '0' || c > '9') {
        if (skip_number(s) != 0)
            return -1;
        return 0;
    }
    if (skip_number(s) != 0)
        return -1;
    if (begin < s->p && *begin == '0' && (s->p - begin) != 1)
        return 0;
    for (q = begin; q < s->p; q++) {
        if (*q < '0' || *q > '9')
            return 0;
    }
    for (q = begin; q < s->p; q++) {
        uint64_t d = (uint64_t)(*q - '0');
        if (v > (UINT64_MAX - d) / 10u)
            return 0;
        v = v * 10u + d;
    }
    *ok = 1;
    *value = v;
    return 0;
}

static int take_lit(Scan *s, const char *lit)
{
    size_t n = strlen(lit);

    skip_ws(s);
    if ((size_t)(s->end - s->p) < n || memcmp(s->p, lit, n) != 0)
        return -1;
    s->p += n;
    return 0;
}

static int skip_container(Scan *s, int depth, char open, char close)
{
    if (depth > 32)
        return -1;
    if (take(s, open) != 0)
        return -1;
    if (peek(s) == (unsigned char)close) {
        s->p++;
        return 0;
    }
    for (;;) {
        if (open == '{') {
            if (parse_string(s, NULL, 0, NULL) != 0)
                return -1;
            if (take(s, ':') != 0)
                return -1;
        }
        if (skip_value(s, depth + 1) != 0)
            return -1;
        if (peek(s) == (unsigned char)close) {
            s->p++;
            return 0;
        }
        if (peek(s) != ',')
            return -1;
        s->p++;
    }
}

static int skip_value(Scan *s, int depth)
{
    int c;

    if (depth > 32)
        return -1;
    c = peek(s);
    if (c == '"')
        return parse_string(s, NULL, 0, NULL);
    if (c == '{')
        return skip_container(s, depth, '{', '}');
    if (c == '[')
        return skip_container(s, depth, '[', ']');
    if (c == 't')
        return take_lit(s, "true");
    if (c == 'f')
        return take_lit(s, "false");
    if (c == 'n')
        return take_lit(s, "null");
    if (c == '-' || (c >= '0' && c <= '9'))
        return skip_number(s);
    return -1;
}

static int hex_span(const uint8_t *text, size_t n)
{
    size_t i;

    if (n != 64u)
        return 0;
    for (i = 0; i < n; i++) {
        unsigned char c = text[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return 0;
    }
    return 1;
}

static int hex64(const char *text)
{
    if (text == NULL || text[64] != '\0')
        return 0;
    return hex_span((const uint8_t *)text, 64);
}

static int parse_one_binary(Scan *s, QueueosSlotBlob *blob, int *valid, int depth)
{
    int have_off = 0;
    int have_len = 0;
    int have_sha = 0;
    uint64_t offset = 0;
    uint64_t length = 0;
    char sha[80];

    *valid = 0;
    if (depth > 32)
        return -1;
    if (take(s, '{') != 0)
        return -1;
    if (peek(s) == '}') {
        s->p++;
        return 0;
    }
    for (;;) {
        char key[64];
        int fit = 0;
        int c;

        if (parse_string(s, key, sizeof key, &fit) != 0)
            return -1;
        if (!fit)
            key[0] = '\0';
        if (take(s, ':') != 0)
            return -1;
        if (strcmp(key, "offset") == 0) {
            int ok = 0;
            if (parse_u64(s, &ok, &offset) != 0)
                return -1;
            have_off = ok;
        } else if (strcmp(key, "length") == 0) {
            int ok = 0;
            if (parse_u64(s, &ok, &length) != 0)
                return -1;
            have_len = ok && length >= 1u;
        } else if (strcmp(key, "sha256") == 0) {
            int sha_fit = 0;
            if (peek(s) != '"') {
                have_sha = 0;
                if (skip_value(s, depth + 1) != 0)
                    return -1;
            } else if (parse_string(s, sha, sizeof sha, &sha_fit) != 0) {
                return -1;
            } else {
                have_sha = sha_fit && hex64(sha);
            }
        } else if (skip_value(s, depth + 1) != 0) {
            return -1;
        }
        c = peek(s);
        if (c == '}') {
            s->p++;
            break;
        }
        if (c != ',')
            return -1;
        s->p++;
    }
    if (have_off && have_len && have_sha && length <= (uint64_t)SIZE_MAX &&
        offset <= UINT64_MAX - length) {
        blob->offset = offset;
        blob->length = length;
        memcpy(blob->sha, sha, 65);
        *valid = 1;
    }
    return 0;
}

static int parse_binaries(Scan *s, QueueosSlotView *slot, int depth)
{
    if (take(s, '[') != 0)
        return -1;
    if (peek(s) == ']') {
        s->p++;
        return 0;
    }
    for (;;) {
        int c;
        if (peek(s) == '{') {
            QueueosSlotBlob blob;
            int valid = 0;
            memset(&blob, 0, sizeof blob);
            if (parse_one_binary(s, &blob, &valid, depth + 1) != 0)
                return -1;
            if (valid) {
                if (slot->blob_count >= QUEUEOS_SLOT_BLOBS)
                    slot->too_many = 1;
                else
                    slot->blobs[slot->blob_count++] = blob;
            }
        } else if (skip_value(s, depth + 1) != 0) {
            return -1;
        }
        c = peek(s);
        if (c == ']') {
            s->p++;
            return 0;
        }
        if (c != ',')
            return -1;
        s->p++;
    }
}

static int parse_generation(Scan *s, QueueosSlotView *slot)
{
    if (take(s, '{') != 0)
        return -1;
    if (peek(s) == '}') {
        s->p++;
        return 0;
    }
    for (;;) {
        char key[64];
        int fit = 0;
        int c;

        if (parse_string(s, key, sizeof key, &fit) != 0)
            return -1;
        if (!fit)
            key[0] = '\0';
        if (take(s, ':') != 0)
            return -1;
        if (strcmp(key, "OS") == 0) {
            if (peek(s) == '"') {
                char text[32];
                int text_fit = 0;
                if (parse_string(s, text, sizeof text, &text_fit) != 0)
                    return -1;
                slot->os_queueos = text_fit && strcmp(text, "QueueOS") == 0;
            } else {
                slot->os_queueos = 0;
                if (skip_value(s, 1) != 0)
                    return -1;
            }
        } else if (strcmp(key, "published") == 0) {
            if (peek(s) == '"') {
                int text_fit = 0;
                if (parse_string(s, slot->published, sizeof slot->published, &text_fit) != 0)
                    return -1;
                if (!text_fit)
                    return -1;
            } else {
                slot->published[0] = '\0';
                if (skip_value(s, 1) != 0)
                    return -1;
            }
        } else if (strcmp(key, "binaries") == 0) {
            slot->blob_count = 0;
            slot->too_many = 0;
            if (peek(s) == '[') {
                if (parse_binaries(s, slot, 1) != 0)
                    return -1;
            } else if (skip_value(s, 1) != 0) {
                return -1;
            }
        } else if (skip_value(s, 1) != 0) {
            return -1;
        }
        c = peek(s);
        if (c == '}') {
            s->p++;
            return 0;
        }
        if (c != ',')
            return -1;
        s->p++;
    }
}

static int hash_matches(const uint8_t *json, size_t json_len,
                        const uint8_t *hash_line, size_t hash_len)
{
    uint8_t digest[32];
    char actual[65];

    if (hash_len != 7u + 64u)
        return 0;
    if (memcmp(hash_line, "sha256 ", 7) != 0)
        return 0;
    /* The hash line is not NUL-terminated. The next byte is the newline. */
    if (!hex_span(hash_line + 7, 64))
        return 0;
    queueos_sha256(json, json_len, digest);
    queueos_sha256_hex(digest, actual);
    return memcmp(actual, hash_line + 7, 64) == 0;
}

static int line_is_booted(const uint8_t *p, const uint8_t *end)
{
    if ((size_t)(end - p) < 8u)
        return 0;
    if (memcmp(p, "booted ", 7) != 0)
        return 0;
    p += 7;
    if (p >= end || *p == '\n' || *p == '\0')
        return 0;
    while (p < end && *p != '\n') {
        if (*p <= 0x20 || *p >= 0x7f)
            return 0;
        p++;
    }
    return p < end && *p == '\n';
}

int queueos_slot_mark_booted(uint8_t slot[QUEUEOS_SLOT_BYTES], const char *stamp)
{
    uint8_t *end;
    uint8_t *nl;
    uint8_t *hash_line;
    uint8_t *nl2;
    uint8_t *tail;
    size_t json_len;
    size_t hash_len;
    size_t stamp_len;
    size_t need;
    size_t i;

    if (slot == NULL || !stamp_ok(stamp))
        return -1;
    end = slot + QUEUEOS_SLOT_BYTES;
    nl = memchr(slot, '\n', QUEUEOS_SLOT_BYTES);
    if (nl == NULL || nl == slot)
        return -1;
    json_len = (size_t)(nl - slot);
    hash_line = nl + 1;
    nl2 = memchr(hash_line, '\n', (size_t)(end - hash_line));
    if (nl2 == NULL)
        return -1;
    hash_len = (size_t)(nl2 - hash_line);
    if (!hash_matches(slot, json_len, hash_line, hash_len))
        return -1;
    tail = nl2 + 1;
    if (line_is_booted(tail, end))
        return 0;
    for (i = 0; tail + i < end; i++) {
        if (tail[i] != 0)
            return -1;
    }
    stamp_len = strlen(stamp);
    need = 7u + stamp_len + 1u;
    if (need > (size_t)(end - tail))
        return -1;
    memcpy(tail, "booted ", 7);
    memcpy(tail + 7, stamp, stamp_len);
    tail[7u + stamp_len] = '\n';
    return 0;
}

void queueos_slot_classify(const uint8_t raw[QUEUEOS_SLOT_BYTES], QueueosSlotView *slot)
{
    const uint8_t *end = raw + QUEUEOS_SLOT_BYTES;
    const uint8_t *nl;
    const uint8_t *hash_line;
    const uint8_t *nl2;
    const uint8_t *hash_end;
    size_t json_len;
    size_t hash_len;
    int matches;
    int parsed;
    Scan scan;

    memset(slot, 0, sizeof *slot);
    slot->kind = QUEUEOS_SLOT_UNREADABLE;
    nl = memchr(raw, '\n', QUEUEOS_SLOT_BYTES);
    if (nl == NULL || nl == raw)
        return;
    json_len = (size_t)(nl - raw);
    hash_line = nl + 1;
    nl2 = memchr(hash_line, '\n', (size_t)(end - hash_line));
    if (nl2 != NULL) {
        hash_end = nl2;
    } else {
        hash_end = end;
        while (hash_end > hash_line && hash_end[-1] == '\0')
            hash_end--;
    }
    hash_len = (size_t)(hash_end - hash_line);

    scan.p = (const char *)raw;
    scan.end = (const char *)raw + json_len;
    parsed = parse_generation(&scan, slot) == 0;
    if (parsed)
        skip_ws(&scan);
    if (!parsed || scan.p != scan.end) {
        memset(slot, 0, sizeof *slot);
        slot->kind = QUEUEOS_SLOT_UNREADABLE;
        return;
    }

    matches = hash_matches(raw, json_len, hash_line, hash_len);
    if (!matches) {
        slot->kind = QUEUEOS_SLOT_UNSAFE;
        return;
    }
    if (json_len == 2 && raw[0] == '{' && raw[1] == '}') {
        slot->kind = QUEUEOS_SLOT_EMPTY;
        return;
    }
    if (slot->os_queueos)
        slot->kind = QUEUEOS_SLOT_CANDIDATE;
    else
        slot->kind = QUEUEOS_SLOT_FOREIGN;
    if (slot->kind == QUEUEOS_SLOT_CANDIDATE && nl2 != NULL && line_is_booted(nl2 + 1, end))
        slot->booted = 1;
}
