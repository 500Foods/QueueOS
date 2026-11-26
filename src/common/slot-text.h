#ifndef QUEUEOS_SLOT_TEXT_H
#define QUEUEOS_SLOT_TEXT_H

#include "disk.h"

#include <stddef.h>
#include <stdint.h>

/* Kinds match the host list. A completion is a booted line on a candidate
 * whose hash matches. The line is not part of the hash. */

enum {
    QUEUEOS_SLOT_UNREADABLE = 1,
    QUEUEOS_SLOT_EMPTY,
    QUEUEOS_SLOT_CANDIDATE,
    QUEUEOS_SLOT_UNSAFE,
    QUEUEOS_SLOT_FOREIGN
};

enum { QUEUEOS_SLOT_BLOBS = 16 };

typedef struct QueueosSlotBlob {
    uint64_t offset;
    uint64_t length;
    char sha[65];
} QueueosSlotBlob;

typedef struct QueueosSlotView {
    int kind;
    int booted;
    int os_queueos;
    int too_many;
    char published[QUEUEOS_SLOT_BYTES];
    QueueosSlotBlob blobs[QUEUEOS_SLOT_BLOBS];
    int blob_count;
} QueueosSlotView;

void queueos_slot_classify(const uint8_t raw[QUEUEOS_SLOT_BYTES],
                           QueueosSlotView *slot);

#endif
