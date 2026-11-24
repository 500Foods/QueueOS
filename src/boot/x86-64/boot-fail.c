/* Second-batch Boot. It reports the loading sentence and returns without
 * calling LoadImage. The harness writes the annotation for that return.
 */

#include "diagnostics.h"
#include "text.h"
#include "types.h"

QUEUEOS_EFI_STATUS QUEUEOS_EFIAPI efi_main(void *image, QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    (void)image;
    queueos_report(system, "QueueOS-Boot: entry", QUEUEOS_BOOT_SENTENCE);
    return QUEUEOS_EFI_LOAD_ERROR;
}
