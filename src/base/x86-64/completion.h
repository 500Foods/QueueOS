/* The one completion line. Base calls this after QueueOS Loaded is on
 * the screen and before that sentence is written to the serial line.
 */
#ifndef QUEUEOS_COMPLETION_H
#define QUEUEOS_COMPLETION_H

#include "types.h"

/* Reads load options "slot N". When the firmware clock can be read,
 * writes "booted STAMP" into that slot after the hash line. A missing
 * slot name, a failed clock, a refused slot, or a failed write leaves
 * the disk as it was. This function does not print.
 */
void queueos_base_complete(QUEUEOS_EFI_SYSTEM_TABLE *system, void *image);

#endif
