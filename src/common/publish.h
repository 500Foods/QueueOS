#ifndef QUEUEOS_PUBLISH_H
#define QUEUEOS_PUBLISH_H

#include "disk.h"

#include <stddef.h>
#include <stdint.h>

/* The image did not change. No slot could take the generation, or the
 * new bytes do not fit in the boot range. */
#define QUEUEOS_PUBLISH_OK 0
#define QUEUEOS_PUBLISH_REFUSED 1

typedef struct QueueosPayload {
    const char *role;
    const char *arch;
    const char *file_label;
    const uint8_t *bytes;
    size_t length;
} QueueosPayload;

typedef struct QueueosGeneration {
    const char *name;
    const char *version;
    const char *published;
    /* Seconds Boot may count. Boot treats under 10 as off and caps
     * over 30. Zero is written as 0. */
    int countdown;
    const QueueosPayload *payloads;
    size_t payload_count;
} QueueosGeneration;

/* One slot: the JSON object, a newline, then "sha256 " and 64 lowercase
 * hex digits of that JSON only, then a newline. booted_stamp, when not
 * NULL, adds one "booted STAMP" line. That line is not part of the hash.
 * The rest of the slot is zeros. Returns 0, or -1 when the text does not
 * fit or the stamp is not a single visible token. */
int queueos_slot_write(uint8_t slot[QUEUEOS_SLOT_BYTES],
                       const char *json, size_t json_len,
                       const char *booted_stamp);

/* Writes "booted STAMP" after the hash line. The JSON and the hash line
 * stay as they are. Returns 0 when the line was written, or when a booted
 * line was already there and was left unchanged. Returns -1, and leaves
 * the slot unchanged, when the stamp is bad, the hash does not match, the
 * line does not fit, or any byte after the hash line is not zero and is
 * not already a booted line. A completion still counts only for a
 * candidate. This function does not check that. */
int queueos_slot_mark_booted(uint8_t slot[QUEUEOS_SLOT_BYTES],
                             const char *stamp);

/* Zeros the whole image, then writes ten empty slots. An empty slot is
 * the two bytes {} and a hash that matches them. */
int queueos_disk_empty(uint8_t *image, size_t image_len);

/* Publishes one QueueOS generation into a raw database image.
 *
 * The slot is the lowest empty one. Otherwise the lowest unreadable,
 * foreign, or UNSAFE slot. Otherwise the oldest candidate, compared as
 * the published text; the same text yields the higher slot index. If
 * that candidate is the only slot that has completed a boot, it stays
 * and the oldest of the others is used. A completion is a booted line
 * on a candidate whose hash matches. An UNSAFE slot with that line is
 * still UNSAFE.
 *
 * An identical payload hash keeps one copy. Bytes a surviving slot
 * still names stay where they are. New bytes take the lowest free gap
 * in the boot range, which is the span after the last used byte when no
 * earlier gap is large enough. They do not enter the slots or the body.
 * Bytes nobody names after the new slot is written are cleared when
 * they sit in the boot range. The ones that remain are not moved.
 *
 * On success, returns QUEUEOS_PUBLISH_OK and, when slot_number is not
 * NULL, writes the chosen slot as 1..10. QUEUEOS_PUBLISH_REFUSED and a
 * negative return both leave the caller's image unchanged. Negative
 * means a bad argument or an allocation failure.
 */
int queueos_publish(uint8_t *image, size_t image_len,
                    const QueueosGeneration *gen, int *slot_number);

#endif
