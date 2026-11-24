#ifndef QUEUEOS_EFI_TEXT_H
#define QUEUEOS_EFI_TEXT_H

#include "types.h"

/* Serial diagnostic, then the sentence on ConOut when that protocol exists,
 * then the same sentence on the serial port.
 */
void queueos_report(QUEUEOS_EFI_SYSTEM_TABLE *system, const char *diagnostic, const char *sentence);

#endif
