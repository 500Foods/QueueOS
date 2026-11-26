/* Classify one slot the way src/boot/slot-read.lua does.
 * Jansson parses the object. The hash is the shared SHA-256 of the
 * JSON bytes only, which stops at the first newline.
 */

#include "slot-json.h"
#include "sha256.h"

#include <jansson.h>

#include <stdlib.h>
#include <string.h>

static void clear_slot(QueueosGuestSlot *slot)
{
    memset(slot, 0, sizeof *slot);
    slot->kind = QUEUEOS_SLOT_UNREADABLE;
}

static void copy_text(char *dst, size_t cap, const char *src)
{
    size_t i = 0;

    if (src == 0) {
        dst[0] = '\0';
        return;
    }
    while (src[i] != '\0' && i + 1 < cap) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static int hex64(const char *text)
{
    size_t i;

    if (text == 0)
        return 0;
    for (i = 0; i < 64; i++) {
        char c = text[i];
        int digit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        if (!digit)
            return 0;
    }
    return text[64] == '\0';
}

static const uint8_t *find_byte(const uint8_t *p, const uint8_t *end, uint8_t want)
{
    while (p < end) {
        if (*p == want)
            return p;
        p++;
    }
    return 0;
}

static int stored_hash(const uint8_t *line, size_t n, char out[65])
{
    size_t i;

    if (n != 7u + 64u)
        return 0;
    if (memcmp(line, "sha256 ", 7) != 0)
        return 0;
    for (i = 0; i < 64; i++) {
        char c = (char)line[7 + i];
        int digit = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        if (!digit)
            return 0;
        out[i] = c;
    }
    out[64] = '\0';
    return 1;
}

/* The line after the hash newline. A dirty tail is not a stamp. */
static void copy_booted(QueueosGuestSlot *slot, const uint8_t *tail, const uint8_t *end)
{
    const uint8_t *p;
    size_t n = 0;

    if (tail == 0 || (size_t)(end - tail) < 8u)
        return;
    if (memcmp(tail, "booted ", 7) != 0)
        return;
    p = tail + 7;
    while (p + n < end && p[n] != '\n') {
        if (p[n] <= 0x20 || p[n] >= 0x7f)
            return;
        n++;
    }
    if (n == 0 || p + n >= end || p[n] != '\n')
        return;
    if (n >= sizeof slot->booted)
        n = sizeof slot->booted - 1u;
    memcpy(slot->booted, p, n);
    slot->booted[n] = '\0';
}

static void take_base(QueueosGuestSlot *slot, json_t *root)
{
    json_t *bins;
    size_t i;
    size_t n;

    bins = json_object_get(root, "binaries");
    if (!json_is_array(bins))
        return;
    n = json_array_size(bins);
    for (i = 0; i < n; i++) {
        json_t *item = json_array_get(bins, i);
        const char *role;
        const char *arch;
        const char *sha;
        json_t *offv;
        json_t *lenv;
        json_int_t off;
        json_int_t len;

        if (!json_is_object(item))
            continue;
        role = json_string_value(json_object_get(item, "role"));
        arch = json_string_value(json_object_get(item, "arch"));
        sha = json_string_value(json_object_get(item, "sha256"));
        offv = json_object_get(item, "offset");
        lenv = json_object_get(item, "length");
        if (role == 0 || strcmp(role, "base") != 0)
            continue;
        if (arch == 0 || strcmp(arch, "x86-64") != 0)
            continue;
        if (!json_is_integer(offv) || !json_is_integer(lenv))
            continue;
        off = json_integer_value(offv);
        len = json_integer_value(lenv);
        if (off < 0 || len < 1)
            continue;
        if (!hex64(sha))
            continue;
        slot->offset = (uint64_t)off;
        slot->length = (uint64_t)len;
        memcpy(slot->sha, sha, 65);
        copy_text(slot->file, sizeof slot->file,
                  json_string_value(json_object_get(item, "file")));
        slot->launchable = 1;
        return;
    }
}

void queueos_guest_release(QueueosGuestSlot *slot)
{
    if (slot == 0)
        return;
    free(slot->json);
    slot->json = 0;
}

void queueos_guest_read_slot(const uint8_t raw[QUEUEOS_SLOT_BYTES],
                             QueueosGuestSlot *slot)
{
    const uint8_t *end = raw + QUEUEOS_SLOT_BYTES;
    const uint8_t *nl;
    const uint8_t *hash_at;
    const uint8_t *hash_end;
    size_t json_len;
    size_t hash_len;
    char stored[65];
    char actual[65];
    uint8_t digest[32];
    json_t *root;
    json_error_t error;
    const char *os_name;
    int matches;

    clear_slot(slot);
    nl = find_byte(raw, end, '\n');
    if (nl == 0 || nl == raw)
        return;
    json_len = (size_t)(nl - raw);
    hash_at = nl + 1;
    hash_end = find_byte(hash_at, end, '\n');
    if (hash_end == 0) {
        hash_end = hash_at;
        while (hash_end < end && *hash_end != 0)
            hash_end++;
    }
    hash_len = (size_t)(hash_end - hash_at);

    root = json_loadb((const char *)raw, json_len, 0, &error);
    if (!json_is_object(root)) {
        json_decref(root);
        return;
    }

    queueos_sha256(raw, json_len, digest);
    queueos_sha256_hex(digest, actual);
    matches = stored_hash(hash_at, hash_len, stored) && strcmp(stored, actual) == 0;

    copy_text(slot->name, sizeof slot->name,
              json_string_value(json_object_get(root, "name")));
    copy_text(slot->version, sizeof slot->version,
              json_string_value(json_object_get(root, "version")));
    copy_text(slot->published, sizeof slot->published,
              json_string_value(json_object_get(root, "published")));
    copy_text(slot->tag, sizeof slot->tag,
              json_string_value(json_object_get(root, "tag")));
    {
        json_t *count = json_object_get(root, "countdown");
        if (json_is_number(count)) {
            double n = json_number_value(count);
            if (n > 1000000.0)
                n = 1000000.0;
            if (n < -1000000.0)
                n = -1000000.0;
            slot->countdown = (int)n;
        }
    }
    if (hash_end < end && *hash_end == '\n')
        copy_booted(slot, hash_end + 1, end);
    if (json_len < SIZE_MAX) {
        slot->json = malloc(json_len + 1u);
        if (slot->json != 0) {
            memcpy(slot->json, raw, json_len);
            slot->json[json_len] = '\0';
        }
    }
    os_name = json_string_value(json_object_get(root, "OS"));

    if (!matches) {
        slot->kind = QUEUEOS_SLOT_UNSAFE;
    } else if (json_len == 2 && raw[0] == '{' && raw[1] == '}') {
        slot->kind = QUEUEOS_SLOT_EMPTY;
    } else if (os_name != 0 && strcmp(os_name, "QueueOS") == 0) {
        slot->kind = QUEUEOS_SLOT_CANDIDATE;
        take_base(slot, root);
    } else {
        slot->kind = QUEUEOS_SLOT_FOREIGN;
    }
    json_decref(root);
}

int queueos_guest_choose(const QueueosGuestSlot *slots, int count)
{
    int best = -1;
    int i;

    for (i = 0; i < count; i++) {
        if (!slots[i].launchable)
            continue;
        if (best < 0 || strcmp(slots[i].published, slots[best].published) > 0)
            best = i;
    }
    if (best < 0)
        return 0;
    return best + 1;
}

int queueos_guest_range_ok(uint64_t offset, uint64_t length)
{
    if (length < 1)
        return 0;
    if (offset < QUEUEOS_SLOT_REGION)
        return 0;
    if (offset >= QUEUEOS_BODY_START)
        return 0;
    if (length > (uint64_t)QUEUEOS_BODY_START - offset)
        return 0;
    return 1;
}

int queueos_guest_digest_ok(const uint8_t *bytes, size_t len, const char *hex)
{
    uint8_t digest[32];
    char got[65];

    if (bytes == 0 || hex == 0 || !hex64(hex))
        return 0;
    queueos_sha256(bytes, len, digest);
    queueos_sha256_hex(digest, got);
    return strcmp(got, hex) == 0;
}
