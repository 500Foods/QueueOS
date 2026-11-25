#ifndef QUEUEOS_EFI_TEXT_H
#define QUEUEOS_EFI_TEXT_H

#include "types.h"

/* Paint one sentence. Attribute, then clear, then the text. A missing
 * ConOut is a no-op so a serial-only boot still runs. */
void queueos_screen_show(QUEUEOS_EFI_SYSTEM_TABLE *system, const char *sentence);

/* Change only the attribute used for later text. Does not clear. */
void queueos_screen_attribute(QUEUEOS_EFI_SYSTEM_TABLE *system, QUEUEOS_UINTN attribute);

/* Serial diagnostic, then the sentence on ConOut when that protocol exists,
 * then the same sentence on the serial port.
 */
void queueos_report(QUEUEOS_EFI_SYSTEM_TABLE *system, const char *diagnostic, const char *sentence);

#endif
