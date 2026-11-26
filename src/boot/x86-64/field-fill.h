/* Interior field shade. Columns 1 and the last column stay the clear
 * color. The last cell of the last row is not written.
 */
#ifndef QUEUEOS_FIELD_FILL_H
#define QUEUEOS_FIELD_FILL_H

#include "types.h"

void queueos_field_fill(QUEUEOS_EFI_SYSTEM_TABLE *system,
                        QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out,
                        int rows,
                        int cols);

#endif
