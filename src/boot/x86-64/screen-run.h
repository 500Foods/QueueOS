/* The Boot list. Lua draws it. This returns the slot to start. */
#ifndef QUEUEOS_SCREEN_RUN_H
#define QUEUEOS_SCREEN_RUN_H

#include "slot-json.h"
#include "types.h"

/* 1-based slot index, or 0 when the list does not launch. */
int queueos_screen_run(QUEUEOS_EFI_SYSTEM_TABLE *system,
                       const QueueosGuestSlot *slots,
                       int count);

#endif
