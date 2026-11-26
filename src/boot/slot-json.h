/* One slot, parsed with Jansson. Kinds are the host list's kinds.
 * launchable matches the host list. The boot range is a separate check:
 * the list may name an offset Boot will not fetch.
 */
#ifndef QUEUEOS_SLOT_JSON_H
#define QUEUEOS_SLOT_JSON_H

#include "slot-text.h"

#include <stddef.h>
#include <stdint.h>

enum { QUEUEOS_GUEST_TEXT = 128 };

typedef struct QueueosGuestSlot {
    int kind;
    int launchable;
    char name[QUEUEOS_GUEST_TEXT];
    char version[QUEUEOS_GUEST_TEXT];
    char published[QUEUEOS_GUEST_TEXT];
    /* JSON number `countdown`. Zero when the field is absent. */
    int countdown;
    /* Stamp from the unhashed `booted` line. Empty when that line is absent. */
    char booted[QUEUEOS_GUEST_TEXT];
    /* JSON string `tag`, when the slot has one. Empty otherwise. */
    char tag[QUEUEOS_GUEST_TEXT];
    char file[QUEUEOS_GUEST_TEXT];
    uint64_t offset;
    uint64_t length;
    char sha[65];
    /* JSON bytes before the hash line. malloc memory. Null when unread. */
    char *json;
} QueueosGuestSlot;

void queueos_guest_read_slot(const uint8_t raw[QUEUEOS_SLOT_BYTES],
                             QueueosGuestSlot *slot);

void queueos_guest_release(QueueosGuestSlot *slot);

/* 1-based index of the newest launchable slot, or 0. An equal published
 * text keeps the lower index.
 */
int queueos_guest_choose(const QueueosGuestSlot *slots, int count);

int queueos_guest_range_ok(uint64_t offset, uint64_t length);
int queueos_guest_digest_ok(const uint8_t *bytes, size_t len, const char *hex);

#endif
