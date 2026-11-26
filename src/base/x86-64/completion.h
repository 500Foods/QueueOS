/* The one completion line. Base calls this after QueueOS Loaded is on
 * the screen and before that sentence is written to the serial line.
 */
#ifndef QUEUEOS_COMPLETION_H
#define QUEUEOS_COMPLETION_H

#include "types.h"

/* Reads load options "slot N". When the firmware clock can be read,
 * writes "booted STAMP" into that slot after the hash line, then writes
 * the body page with that same stamp. An existing booted line stays.
 * The page is still written. When the old page is a record, that stored
 * line is printed before the new page is written. A missing slot name,
 * a failed clock, a refused slot, or a failed slot write leaves the
 * page as it was and prints nothing. A failed page write leaves the
 * page as it was. This function does not print QueueOS Loaded.
 */
void queueos_base_complete(QUEUEOS_EFI_SYSTEM_TABLE *system, void *image);

#endif
