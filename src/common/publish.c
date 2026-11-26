#include "publish.h"

#include "sha256.h"
#include "slot-text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    MAX_INTERVALS = 192,
    MAX_PAYLOADS = 8,
    JSON_MAX = QUEUEOS_SLOT_BYTES - 73
};

_Static_assert(QUEUEOS_BODY_START > QUEUEOS_SLOT_REGION, "boot range");
_Static_assert(JSON_MAX == 4023, "json room in a slot");

typedef struct Interval {
    uint64_t start;
    uint64_t end;
} Interval;

typedef struct Placed {
    uint64_t offset;
    uint64_t length;
    char sha[65];
    const uint8_t *bytes;
} Placed;

static int oldest_candidate(const QueueosSlotView *slots, int skip)
{
    int best = -1;
    int i;

    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        int cmp;
        if (i == skip || slots[i].kind != QUEUEOS_SLOT_CANDIDATE)
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        cmp = strcmp(slots[i].published, slots[best].published);
        if (cmp < 0 || (cmp == 0 && i > best))
            best = i;
    }
    return best;
}

static int booted_candidates(const QueueosSlotView *slots)
{
    int n = 0;
    int i;

    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        if (slots[i].kind == QUEUEOS_SLOT_CANDIDATE && slots[i].booted)
            n++;
    }
    return n;
}

/* 1-based slot, or 0 when the rules have nowhere to write. */
static int choose_slot(const QueueosSlotView *slots)
{
    int i;
    int chosen;
    int booted;

    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        if (slots[i].kind == QUEUEOS_SLOT_EMPTY)
            return i + 1;
    }
    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        int kind = slots[i].kind;
        if (kind == QUEUEOS_SLOT_UNREADABLE || kind == QUEUEOS_SLOT_FOREIGN || kind == QUEUEOS_SLOT_UNSAFE)
            return i + 1;
    }
    chosen = oldest_candidate(slots, -1);
    if (chosen < 0)
        return 0;
    booted = booted_candidates(slots);
    if (booted == 1 && slots[chosen].booted) {
        chosen = oldest_candidate(slots, chosen);
        if (chosen < 0)
            return 0;
    }
    return chosen + 1;
}

static int text_ok(const char *s)
{
    const unsigned char *p;

    if (s == NULL || s[0] == '\0')
        return 0;
    for (p = (const unsigned char *)s; *p != '\0'; p++) {
        if (*p < 0x20 || *p == 0x7f)
            return 0;
    }
    return 1;
}

static int generation_ok(const QueueosGeneration *gen)
{
    size_t i;

    if (gen == NULL || !text_ok(gen->name) || !text_ok(gen->version) || !text_ok(gen->published))
        return 0;
    if (gen->payload_count > MAX_PAYLOADS)
        return 0;
    if (gen->payload_count > 0 && gen->payloads == NULL)
        return 0;
    for (i = 0; i < gen->payload_count; i++) {
        const QueueosPayload *p = &gen->payloads[i];
        if (!text_ok(p->role) || !text_ok(p->arch) || !text_ok(p->file_label))
            return 0;
        if (p->bytes == NULL || p->length < 1u)
            return 0;
    }
    return 1;
}

static void sort_intervals(Interval *iv, int n)
{
    int i;

    for (i = 1; i < n; i++) {
        Interval key = iv[i];
        int j = i;
        while (j > 0 && iv[j - 1].start > key.start) {
            iv[j] = iv[j - 1];
            j--;
        }
        iv[j] = key;
    }
}

static int add_interval(Interval *iv, int *n, uint64_t offset, uint64_t length, uint64_t limit)
{
    Interval clip;

    if (length == 0 || offset > UINT64_MAX - length)
        return 0;
    clip.end = offset + length;
    clip.start = offset;
    if (clip.end <= QUEUEOS_SLOT_REGION || clip.start >= limit)
        return 0;
    if (clip.start < QUEUEOS_SLOT_REGION)
        clip.start = QUEUEOS_SLOT_REGION;
    if (clip.end > limit)
        clip.end = limit;
    if (clip.end <= clip.start)
        return 0;
    if (*n >= MAX_INTERVALS)
        return -1;
    iv[(*n)++] = clip;
    return 0;
}

static int find_gap(Interval *iv, int n, uint64_t need, uint64_t limit, uint64_t *out)
{
    uint64_t pos = QUEUEOS_SLOT_REGION;
    int i;

    sort_intervals(iv, n);
    for (i = 0; i < n; i++) {
        if (iv[i].end <= pos)
            continue;
        if (iv[i].start > pos && iv[i].start - pos >= need) {
            *out = pos;
            return 0;
        }
        if (iv[i].end > pos)
            pos = iv[i].end;
    }
    if (pos <= limit && limit - pos >= need) {
        *out = pos;
        return 0;
    }
    return -1;
}

static int in_boot(uint64_t offset, uint64_t length, uint64_t limit)
{
    if (length == 0 || offset < QUEUEOS_SLOT_REGION)
        return 0;
    if (offset > limit || length > limit - offset)
        return 0;
    return 1;
}

static int find_reuse(const QueueosSlotView *slots, const Placed *placed, int placed_n,
                      const uint8_t *image, uint64_t limit,
                      const char *sha, const uint8_t *data, size_t len,
                      uint64_t *offset)
{
    int found = 0;
    uint64_t best = 0;
    int i;
    int b;

    for (i = 0; i < placed_n; i++) {
        if (strcmp(placed[i].sha, sha) != 0 || placed[i].length != len)
            continue;
        if (!in_boot(placed[i].offset, placed[i].length, limit))
            continue;
        if (!found || placed[i].offset < best) {
            best = placed[i].offset;
            found = 1;
        }
    }
    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        for (b = 0; b < slots[i].blob_count; b++) {
            const QueueosSlotBlob *blob = &slots[i].blobs[b];
            if (strcmp(blob->sha, sha) != 0 || blob->length != len)
                continue;
            if (!in_boot(blob->offset, blob->length, limit))
                continue;
            if (memcmp(image + (size_t)blob->offset, data, len) != 0)
                continue;
            if (!found || blob->offset < best) {
                best = blob->offset;
                found = 1;
            }
        }
    }
    if (!found)
        return 0;
    *offset = best;
    return 1;
}

static int append_text(char *buf, size_t cap, size_t *used, const char *text)
{
    size_t n = strlen(text);

    if (*used + n >= cap)
        return -1;
    memcpy(buf + *used, text, n);
    *used += n;
    buf[*used] = '\0';
    return 0;
}

static int append_json(char *buf, size_t cap, size_t *used, const char *text)
{
    const unsigned char *p;

    if (append_text(buf, cap, used, "\"") != 0)
        return -1;
    for (p = (const unsigned char *)text; *p != '\0'; p++) {
        char esc[8];
        if (*p == '"' || *p == '\\') {
            esc[0] = '\\';
            esc[1] = (char)*p;
            esc[2] = '\0';
        } else if (*p < 0x20) {
            snprintf(esc, sizeof esc, "\\u%04x", *p);
        } else {
            esc[0] = (char)*p;
            esc[1] = '\0';
        }
        if (append_text(buf, cap, used, esc) != 0)
            return -1;
    }
    return append_text(buf, cap, used, "\"");
}

static int format_generation(char *buf, size_t cap, const QueueosGeneration *gen,
                             const Placed *placed)
{
    size_t used = 0;
    size_t i;

    buf[0] = '\0';
    if (append_text(buf, cap, &used, "{\"OS\":\"QueueOS\",\"name\":") != 0)
        return -1;
    if (append_json(buf, cap, &used, gen->name) != 0)
        return -1;
    if (append_text(buf, cap, &used, ",\"version\":") != 0)
        return -1;
    if (append_json(buf, cap, &used, gen->version) != 0)
        return -1;
    if (append_text(buf, cap, &used, ",\"published\":") != 0)
        return -1;
    if (append_json(buf, cap, &used, gen->published) != 0)
        return -1;
    if (append_text(buf, cap, &used, ",\"countdown\":") != 0)
        return -1;
    {
        char num[32];
        snprintf(num, sizeof num, "%d", gen->countdown);
        if (append_text(buf, cap, &used, num) != 0)
            return -1;
    }
    if (append_text(buf, cap, &used, ",\"binaries\":[") != 0)
        return -1;
    for (i = 0; i < gen->payload_count; i++) {
        char num[32];
        const QueueosPayload *payload = &gen->payloads[i];
        if (i > 0 && append_text(buf, cap, &used, ",") != 0)
            return -1;
        if (append_text(buf, cap, &used, "{\"role\":") != 0)
            return -1;
        if (append_json(buf, cap, &used, payload->role) != 0)
            return -1;
        if (append_text(buf, cap, &used, ",\"arch\":") != 0)
            return -1;
        if (append_json(buf, cap, &used, payload->arch) != 0)
            return -1;
        snprintf(num, sizeof num, "%llu", (unsigned long long)placed[i].offset);
        if (append_text(buf, cap, &used, ",\"offset\":") != 0)
            return -1;
        if (append_text(buf, cap, &used, num) != 0)
            return -1;
        snprintf(num, sizeof num, "%llu", (unsigned long long)placed[i].length);
        if (append_text(buf, cap, &used, ",\"length\":") != 0)
            return -1;
        if (append_text(buf, cap, &used, num) != 0)
            return -1;
        if (append_text(buf, cap, &used, ",\"sha256\":\"") != 0)
            return -1;
        if (append_text(buf, cap, &used, placed[i].sha) != 0)
            return -1;
        if (append_text(buf, cap, &used, "\",\"file\":") != 0)
            return -1;
        if (append_json(buf, cap, &used, payload->file_label) != 0)
            return -1;
        if (append_text(buf, cap, &used, "}") != 0)
            return -1;
    }
    if (append_text(buf, cap, &used, "]}") != 0)
        return -1;
    if (used > JSON_MAX)
        return -1;
    return (int)used;
}

static void mark_window(unsigned char *map, size_t window, uint64_t offset, uint64_t length)
{
    uint64_t start;
    uint64_t end;
    uint64_t i;
    uint64_t base = QUEUEOS_SLOT_REGION;

    if (map == NULL || length == 0 || offset > UINT64_MAX - length)
        return;
    start = offset;
    end = offset + length;
    if (end <= base || start >= base + window)
        return;
    if (start < base)
        start = base;
    if (end > base + window)
        end = base + window;
    for (i = start; i < end; i++)
        map[(size_t)(i - base)] = 1;
}

static int publish_copy(uint8_t *image, size_t image_len, const QueueosGeneration *gen,
                        int *slot_number)
{
    QueueosSlotView *slots = NULL;
    Interval *intervals = NULL;
    unsigned char *keep = NULL;
    unsigned char *used = NULL;
    Placed placed[MAX_PAYLOADS];
    char sha[MAX_PAYLOADS][65];
    uint8_t digest[32];
    char json[QUEUEOS_SLOT_BYTES];
    int json_len;
    int interval_count = 0;
    int placed_n = 0;
    int target;
    int i;
    int b;
    int rc = -1;
    uint64_t limit;
    size_t window;

    if (!generation_ok(gen))
        return -1;
    if (image_len < QUEUEOS_SLOT_REGION)
        return -1;
    limit = QUEUEOS_BODY_START;
    if ((uint64_t)image_len < limit)
        limit = (uint64_t)image_len;
    window = (size_t)(limit - QUEUEOS_SLOT_REGION);

    slots = calloc(QUEUEOS_SLOT_COUNT, sizeof *slots);
    intervals = calloc(MAX_INTERVALS, sizeof *intervals);
    if (slots == NULL || intervals == NULL)
        goto out;
    if (window > 0) {
        keep = calloc(window, 1);
        used = calloc(window, 1);
        if (keep == NULL || used == NULL)
            goto out;
    }

    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++)
        queueos_slot_classify(image + (size_t)i * QUEUEOS_SLOT_BYTES, &slots[i]);
    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        if (slots[i].too_many)
            goto out;
    }

    target = choose_slot(slots);
    if (target == 0) {
        rc = QUEUEOS_PUBLISH_REFUSED;
        goto out;
    }

    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        if (i + 1 == target)
            continue;
        for (b = 0; b < slots[i].blob_count; b++) {
            if (add_interval(intervals, &interval_count, slots[i].blobs[b].offset,
                             slots[i].blobs[b].length, limit) != 0)
                goto out;
        }
    }

    for (i = 0; i < (int)gen->payload_count; i++) {
        queueos_sha256(gen->payloads[i].bytes, gen->payloads[i].length, digest);
        queueos_sha256_hex(digest, sha[i]);
    }

    /* Reserve every hash that already has a copy before looking for a gap,
     * so a new blob cannot land on bytes a later payload still needs. */
    for (i = 0; i < (int)gen->payload_count; i++) {
        uint64_t offset = 0;
        if (!find_reuse(slots, placed, placed_n, image, limit, sha[i],
                        gen->payloads[i].bytes, gen->payloads[i].length, &offset))
            continue;
        placed[placed_n].offset = offset;
        placed[placed_n].length = gen->payloads[i].length;
        placed[placed_n].bytes = gen->payloads[i].bytes;
        memcpy(placed[placed_n].sha, sha[i], 65);
        if (add_interval(intervals, &interval_count, offset, placed[placed_n].length, limit) != 0)
            goto out;
        placed_n++;
    }
    for (i = 0; i < (int)gen->payload_count; i++) {
        uint64_t offset = 0;
        int already = 0;
        int p;
        for (p = 0; p < placed_n; p++) {
            if (strcmp(placed[p].sha, sha[i]) == 0 &&
                placed[p].length == gen->payloads[i].length) {
                already = 1;
                break;
            }
        }
        if (already)
            continue;
        if (find_gap(intervals, interval_count, gen->payloads[i].length, limit, &offset) != 0) {
            rc = QUEUEOS_PUBLISH_REFUSED;
            goto out;
        }
        placed[placed_n].offset = offset;
        placed[placed_n].length = gen->payloads[i].length;
        placed[placed_n].bytes = gen->payloads[i].bytes;
        memcpy(placed[placed_n].sha, sha[i], 65);
        if (add_interval(intervals, &interval_count, offset, placed[placed_n].length, limit) != 0)
            goto out;
        placed_n++;
    }

    /* format_generation walks payloads in caller order. Rebuild placed
     * in that order. Reused and fresh entries are both in the list; a
     * repeated hash shares the first offset. */
    {
        Placed ordered[MAX_PAYLOADS];
        for (i = 0; i < (int)gen->payload_count; i++) {
            int p;
            int found = 0;
            for (p = 0; p < placed_n; p++) {
                if (strcmp(placed[p].sha, sha[i]) == 0 &&
                    placed[p].length == gen->payloads[i].length) {
                    ordered[i] = placed[p];
                    ordered[i].bytes = gen->payloads[i].bytes;
                    found = 1;
                    break;
                }
            }
            if (!found)
                goto out;
        }
        for (i = 0; i < (int)gen->payload_count; i++)
            placed[i] = ordered[i];
        placed_n = (int)gen->payload_count;
    }

    json_len = format_generation(json, sizeof json, gen, placed);
    if (json_len < 0)
        goto out;

    for (i = 0; i < QUEUEOS_SLOT_COUNT; i++) {
        for (b = 0; b < slots[i].blob_count; b++)
            mark_window(used, window, slots[i].blobs[b].offset, slots[i].blobs[b].length);
        if (i + 1 == target)
            continue;
        for (b = 0; b < slots[i].blob_count; b++)
            mark_window(keep, window, slots[i].blobs[b].offset, slots[i].blobs[b].length);
    }
    for (i = 0; i < placed_n; i++)
        mark_window(keep, window, placed[i].offset, placed[i].length);

    if (window > 0) {
        size_t n;
        for (n = 0; n < window; n++) {
            if (used[n] && !keep[n])
                image[QUEUEOS_SLOT_REGION + n] = 0;
        }
    }
    for (i = 0; i < placed_n; i++) {
        if (placed[i].offset > (uint64_t)image_len ||
            placed[i].length > (uint64_t)image_len - placed[i].offset)
            goto out;
        memcpy(image + (size_t)placed[i].offset, placed[i].bytes, (size_t)placed[i].length);
    }
    if (queueos_slot_write(image + (size_t)(target - 1) * QUEUEOS_SLOT_BYTES,
                           json, (size_t)json_len, NULL) != 0)
        goto out;

    if (slot_number != NULL)
        *slot_number = target;
    rc = QUEUEOS_PUBLISH_OK;

out:
    free(slots);
    free(intervals);
    free(keep);
    free(used);
    return rc;
}

int queueos_publish(uint8_t *image, size_t image_len, const QueueosGeneration *gen,
                    int *slot_number)
{
    uint8_t *copy;
    int slot = 0;
    int rc;

    if (image == NULL || !generation_ok(gen) || image_len < QUEUEOS_SLOT_REGION)
        return -1;
    copy = malloc(image_len);
    if (copy == NULL)
        return -1;
    memcpy(copy, image, image_len);
    rc = publish_copy(copy, image_len, gen, &slot);
    if (rc == QUEUEOS_PUBLISH_OK) {
        memcpy(image, copy, image_len);
        if (slot_number != NULL)
            *slot_number = slot;
    }
    free(copy);
    return rc;
}
