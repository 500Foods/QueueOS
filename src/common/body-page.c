/* One body page. The ring is a JSON array, then the same sha256 line a
 * slot uses. The hash covers the JSON bytes and not the newline. The
 * tail stays zero so a torn or foreign page is not a ring.
 *
 * The writer does not call json_dumps. Jansson walks object keys in
 * hashtable order, and that order follows the hash seed, so a dump is
 * not a stable page. The canonical text is written here and parsed
 * with Jansson before it replaces the caller's bytes. The reader
 * accepts any array of the same objects whose bytes match the hash.
 */

#include "body-page.h"

#include "sha256.h"

#include <jansson.h>

#include <stddef.h>
#include <string.h>

/* Stack bound. A 4 KiB page of this schema holds fewer rows than this. */
#define QUEUEOS_BODY_ROWS 96

_Static_assert(QUEUEOS_BODY_LINE >= 36, "slot 10 stored line");
_Static_assert(QUEUEOS_BODY_ROWS >= 94, "one-digit ring");

typedef struct BodyRow {
    char stamp[21];
    int slot;
} BodyRow;

static int digits_between(const char *text, int width, int min, int max)
{
    int value = 0;
    int i;

    for (i = 0; i < width; i++) {
        if (text[i] < '0' || text[i] > '9')
            return 0;
        value = value * 10 + (text[i] - '0');
    }
    return value >= min && value <= max;
}

/* Twenty characters: YYYY-MM-DDTHH:MM:SSZ, with the ranges format_stamp uses. */
static int stamp_form(const char *stamp)
{
    int i;

    if (stamp == NULL)
        return 0;
    for (i = 0; i < 20; i++) {
        if (stamp[i] == '\0')
            return 0;
    }
    if (stamp[20] != '\0')
        return 0;
    if (stamp[4] != '-' || stamp[7] != '-' || stamp[10] != 'T')
        return 0;
    if (stamp[13] != ':' || stamp[16] != ':' || stamp[19] != 'Z')
        return 0;
    if (!digits_between(stamp, 4, 1, 9999))
        return 0;
    if (!digits_between(stamp + 5, 2, 1, 12))
        return 0;
    if (!digits_between(stamp + 8, 2, 1, 31))
        return 0;
    if (!digits_between(stamp + 11, 2, 0, 23))
        return 0;
    if (!digits_between(stamp + 14, 2, 0, 59))
        return 0;
    if (!digits_between(stamp + 17, 2, 0, 60))
        return 0;
    return 1;
}

static int lower_hex(const uint8_t *text, size_t length)
{
    size_t i;

    for (i = 0; i < length; i++) {
        char c = (char)text[i];
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))
            continue;
        return 0;
    }
    return 1;
}

static int append_bytes(char *dst, size_t cap, size_t *used, const char *text, size_t n)
{
    if (*used + n > cap)
        return 0;
    memcpy(dst + *used, text, n);
    *used += n;
    return 1;
}

/* 1 when a newline ends the JSON and no NUL sits inside it. */
static int json_span(const uint8_t page[QUEUEOS_BODY_PAGE], size_t *json_len)
{
    size_t i;

    for (i = 0; i < QUEUEOS_BODY_PAGE; i++) {
        if (page[i] == '\n') {
            *json_len = i;
            return i > 0;
        }
        if (page[i] == 0)
            return 0;
    }
    return 0;
}

static int hash_matches(const uint8_t page[QUEUEOS_BODY_PAGE], size_t json_len)
{
    size_t at = json_len + 1u;
    uint8_t digest[32];
    char hex[65];
    size_t i;

    if (at + 72u > QUEUEOS_BODY_PAGE)
        return 0;
    if (memcmp(page + at, "sha256 ", 7) != 0)
        return 0;
    if (!lower_hex(page + at + 7u, 64u))
        return 0;
    if (page[at + 71u] != '\n')
        return 0;
    for (i = at + 72u; i < QUEUEOS_BODY_PAGE; i++) {
        if (page[i] != 0)
            return 0;
    }
    queueos_sha256(page, json_len, digest);
    queueos_sha256_hex(digest, hex);
    return memcmp(hex, page + at + 7u, 64) == 0;
}

static int take_row(const json_t *element, BodyRow *row)
{
    const json_t *stored;
    const json_t *slotv;
    const char *text;
    json_int_t slot;

    if (!json_is_object(element) || json_object_size(element) != 2)
        return 0;
    stored = json_object_get(element, "stored");
    slotv = json_object_get(element, "slot");
    text = json_string_value(stored);
    if (!json_is_string(stored) || !stamp_form(text))
        return 0;
    if (!json_is_integer(slotv))
        return 0;
    slot = json_integer_value(slotv);
    if (slot < 1 || slot > 10)
        return 0;
    memcpy(row->stamp, text, 21);
    row->slot = (int)slot;
    return 1;
}

/* 1 and a row count, including zero for an empty array. 0 when refused. */
static int parse_ring(const uint8_t page[QUEUEOS_BODY_PAGE],
                      BodyRow rows[QUEUEOS_BODY_ROWS], int *count)
{
    size_t json_len = 0;
    json_t *root;
    json_error_t error;
    size_t n;
    size_t i;

    *count = 0;
    if (!json_span(page, &json_len) || !hash_matches(page, json_len))
        return 0;
    root = json_loadb((const char *)page, json_len, JSON_REJECT_DUPLICATES, &error);
    if (!json_is_array(root)) {
        json_decref(root);
        return 0;
    }
    n = json_array_size(root);
    if (n > QUEUEOS_BODY_ROWS) {
        json_decref(root);
        return 0;
    }
    for (i = 0; i < n; i++) {
        if (!take_row(json_array_get(root, i), &rows[i])) {
            json_decref(root);
            return 0;
        }
    }
    json_decref(root);
    *count = (int)n;
    return 1;
}

static void sentence(const BodyRow *row, char line[QUEUEOS_BODY_LINE])
{
    size_t n = 0;

    memcpy(line, "stored ", 7);
    n = 7;
    memcpy(line + n, row->stamp, 20);
    n += 20;
    memcpy(line + n, " slot ", 6);
    n += 6;
    if (row->slot == 10) {
        line[n++] = '1';
        line[n++] = '0';
    } else {
        line[n++] = (char)('0' + row->slot);
    }
    line[n] = '\0';
}

/* Bytes of the canonical array, or 0 when the buffer cannot hold it. */
static size_t encode(char *dst, size_t cap, const BodyRow *rows, int count)
{
    size_t used = 0;
    int i;

    if (!append_bytes(dst, cap, &used, "[", 1))
        return 0;
    for (i = 0; i < count; i++) {
        char num[2];

        if (i > 0 && !append_bytes(dst, cap, &used, ",", 1))
            return 0;
        if (!append_bytes(dst, cap, &used, "{\"stored\":\"", 11))
            return 0;
        if (!append_bytes(dst, cap, &used, rows[i].stamp, 20))
            return 0;
        if (!append_bytes(dst, cap, &used, "\",\"slot\":", 9))
            return 0;
        if (rows[i].slot == 10) {
            num[0] = '1';
            num[1] = '0';
            if (!append_bytes(dst, cap, &used, num, 2))
                return 0;
        } else {
            num[0] = (char)('0' + rows[i].slot);
            if (!append_bytes(dst, cap, &used, num, 1))
                return 0;
        }
        if (!append_bytes(dst, cap, &used, "}", 1))
            return 0;
    }
    if (!append_bytes(dst, cap, &used, "]", 1))
        return 0;
    return used;
}

static int fits(const BodyRow *rows, int count)
{
    char json[QUEUEOS_BODY_PAGE];
    size_t n = encode(json, sizeof json, rows, count);

    return n > 0 && n + 73u <= QUEUEOS_BODY_PAGE;
}

static void drop_oldest(BodyRow *rows, int *count)
{
    if (*count <= 0)
        return;
    memmove(rows, rows + 1, (size_t)(*count - 1) * sizeof *rows);
    *count -= 1;
}

static int lay(uint8_t page[QUEUEOS_BODY_PAGE], const char *json, size_t json_len)
{
    uint8_t digest[32];
    char hex[65];

    if (json_len + 73u > QUEUEOS_BODY_PAGE)
        return 0;
    memset(page, 0, QUEUEOS_BODY_PAGE);
    memcpy(page, json, json_len);
    page[json_len] = '\n';
    queueos_sha256((const uint8_t *)json, json_len, digest);
    queueos_sha256_hex(digest, hex);
    memcpy(page + json_len + 1u, "sha256 ", 7);
    memcpy(page + json_len + 8u, hex, 64);
    page[json_len + 72u] = '\n';
    return 1;
}

int queueos_body_read(const uint8_t page[QUEUEOS_BODY_PAGE],
                      char line[QUEUEOS_BODY_LINE], int *slot)
{
    BodyRow rows[QUEUEOS_BODY_ROWS];
    int count = 0;

    if (page == NULL || line == NULL)
        return -1;
    if (!parse_ring(page, rows, &count) || count < 1)
        return 0;
    sentence(&rows[count - 1], line);
    if (slot != NULL)
        *slot = rows[count - 1].slot;
    return 1;
}

int queueos_body_write(uint8_t page[QUEUEOS_BODY_PAGE],
                       const char *stamp, int slot)
{
    BodyRow rows[QUEUEOS_BODY_ROWS];
    uint8_t built[QUEUEOS_BODY_PAGE];
    char json[QUEUEOS_BODY_PAGE];
    int count = 0;
    int check = 0;
    size_t n;

    if (page == NULL || !stamp_form(stamp) || slot < 1 || slot > 10)
        return -1;
    if (!parse_ring(page, rows, &count))
        count = 0;
    if (count >= QUEUEOS_BODY_ROWS)
        drop_oldest(rows, &count);
    memcpy(rows[count].stamp, stamp, 21);
    rows[count].slot = slot;
    count++;
    while (count > 0 && !fits(rows, count))
        drop_oldest(rows, &count);
    if (count < 1)
        return -1;
    n = encode(json, sizeof json, rows, count);
    if (n == 0 || !lay(built, json, n))
        return -1;
    if (!parse_ring(built, rows, &check) || check != count)
        return -1;
    memcpy(page, built, QUEUEOS_BODY_PAGE);
    return 0;
}
