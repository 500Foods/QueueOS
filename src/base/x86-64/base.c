/* QueueOS-Base stub.
 *
 * After the loaded sentence, wait about five seconds so a person running
 * the run target can see the screen. The harness does not time this stall.
 * It quits QEMU once the serial line it is waiting for has arrived.
 */

#include "diagnostics.h"
#include "serial.h"
#include "text.h"
#include "types.h"

QUEUEOS_EFI_STATUS QUEUEOS_EFIAPI efi_main(void *image, QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    (void)image;
    queueos_report(system, "QueueOS-Base: entry", QUEUEOS_BASE_SENTENCE);
    queueos_serial_init();
    queueos_serial_write_line("QueueOS-Base: ready");
    if (system != 0 && system->boot_services != 0 && system->boot_services->stall != 0) {
        system->boot_services->stall(5u * 1000u * 1000u);
    }
    return QUEUEOS_EFI_SUCCESS;
}
