/* One body page. The hash covers the stored line and not its newline,
 * so the check is the same shape as a slot hash over JSON bytes only.
 * The tail stays zero so a torn or foreign page is not a record.
 */

#include "body-page.h"

#include "sha256.h"

#include <string.h>

_Static_assert(QUEUEOS_BODY_LINE >= 36, "slot 10 stored line");

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

/* 1 when the first line is "stored STAMP slot N". line_len excludes the newline. */
static int stored_prefix(const uint8_t *page, size_t *line_len, int *slot, char stamp[21])
{
    size_t nl;

    for (nl = 0; nl < 36u && nl < QUEUEOS_BODY_PAGE; nl++) {
        if (page[nl] == '\n')
            break;
    }
    if (nl != 34u && nl != 35u)
        return 0;
    if (page[nl] != '\n')
        return 0;
    if (memcmp(page, "stored ", 7) != 0)
        return 0;
    memcpy(stamp, page + 7, 20);
    stamp[20] = '\0';
    if (!stamp_form(stamp))
        return 0;
    if (memcmp(page + 27, " slot ", 6) != 0)
        return 0;
    if (nl == 34u) {
        if (page[33] < '1' || page[33] > '9')
            return 0;
        *slot = page[33] - '0';
    } else if (page[33] == '1' && page[34] == '0') {
        *slot = 10;
    } else {
        return 0;
    }
    *line_len = nl;
    return 1;
}

int queueos_body_read(const uint8_t page[QUEUEOS_BODY_PAGE],
                      char line[QUEUEOS_BODY_LINE], int *slot)
{
    char stamp[21];
    uint8_t digest[32];
    char hex[65];
    size_t line_len = 0;
    size_t hash_at;
    int parsed = 0;
    size_t i;

    if (page == NULL || line == NULL)
        return -1;
    if (!stored_prefix(page, &line_len, &parsed, stamp))
        return 0;
    hash_at = line_len + 1u;
    if (hash_at + 72u > QUEUEOS_BODY_PAGE)
        return 0;
    if (memcmp(page + hash_at, "sha256 ", 7) != 0)
        return 0;
    if (!lower_hex(page + hash_at + 7u, 64u))
        return 0;
    if (page[hash_at + 71u] != '\n')
        return 0;
    queueos_sha256(page, line_len, digest);
    queueos_sha256_hex(digest, hex);
    if (memcmp(hex, page + hash_at + 7u, 64) != 0)
        return 0;
    for (i = hash_at + 72u; i < QUEUEOS_BODY_PAGE; i++) {
        if (page[i] != 0)
            return 0;
    }
    if (line_len + 1u > QUEUEOS_BODY_LINE)
        return 0;
    memcpy(line, page, line_len);
    line[line_len] = '\0';
    if (slot != NULL)
        *slot = parsed;
    return 1;
}

int queueos_body_write(uint8_t page[QUEUEOS_BODY_PAGE],
                       const char *stamp, int slot)
{
    char stored[36];
    uint8_t digest[32];
    char hex[65];
    size_t used;

    if (page == NULL || !stamp_form(stamp) || slot < 1 || slot > 10)
        return -1;
    memcpy(stored, "stored ", 7);
    memcpy(stored + 7, stamp, 20);
    memcpy(stored + 27, " slot ", 6);
    used = 33;
    if (slot == 10) {
        stored[used++] = '1';
        stored[used++] = '0';
    } else {
        stored[used++] = (char)('0' + slot);
    }
    queueos_sha256((const uint8_t *)stored, used, digest);
    queueos_sha256_hex(digest, hex);
    memset(page, 0, QUEUEOS_BODY_PAGE);
    memcpy(page, stored, used);
    page[used] = '\n';
    memcpy(page + used + 1u, "sha256 ", 7);
    memcpy(page + used + 8u, hex, 64);
    page[used + 72u] = '\n';
    return 0;
}
