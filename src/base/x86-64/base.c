/* QueueOS-Base stub.
 *
 * The Loaded sentence is drawn first. The completion path then writes
 * the slot line and the body page. An older stored line, when the page
 * has one, is printed there. The serial Loaded sentence comes after
 * that. The harness quits QEMU once the line it is waiting for has
 * arrived, so both writes have to finish before that line is printed.
 * A failed write still prints the sentence.
 *
 * Then count down from 10 so a person running the run target can see
 * this stage. The harness does not sit through the countdown.
 *
 * The countdown ends by leaving the console attribute red on red. The
 * Loaded sentence stays as it was drawn. Firmware lines that reuse the
 * attribute then match the field.
 */

#include "completion.h"
#include "diagnostics.h"
#include "pool.h"
#include "serial.h"
#include "text.h"
#include "types.h"

static void queueos_base_countdown(QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    int second;

    for (second = 10; second >= 1; second--) {
        char screen[48];
        char line[40];
        const char *prefix = "QueueOS-Base: countdown ";
        int i = 0;
        int n = 0;

        while (QUEUEOS_BASE_SENTENCE[n] != '\0' && i + 4 < (int)sizeof screen) {
            screen[i++] = QUEUEOS_BASE_SENTENCE[n++];
        }
        screen[i++] = '\r';
        screen[i++] = '\n';
        if (second >= 10) {
            screen[i++] = '1';
            screen[i++] = '0';
        } else {
            screen[i++] = (char)('0' + second);
        }
        screen[i] = '\0';
        queueos_screen_show(system, screen);

        i = 0;
        while (prefix[i] != '\0' && i + 3 < (int)sizeof line) {
            line[i] = prefix[i];
            i++;
        }
        if (second >= 10) {
            line[i++] = '1';
            line[i++] = '0';
        } else {
            line[i++] = (char)('0' + second);
        }
        line[i] = '\0';
        queueos_serial_write_line(line);
        if (system != 0 && system->boot_services != 0 && system->boot_services->stall != 0) {
            system->boot_services->stall(1000u * 1000u);
        }
    }
}

QUEUEOS_EFI_STATUS QUEUEOS_EFIAPI efi_main(void *image, QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    queueos_pool_bind(system);
    queueos_serial_init();
    queueos_serial_write_line("QueueOS-Base: entry");
    queueos_screen_show(system, QUEUEOS_BASE_SENTENCE);
    queueos_base_complete(system, image);
    queueos_serial_write_line(QUEUEOS_BASE_SENTENCE);
    queueos_serial_write_line("QueueOS-Base: ready");
    queueos_base_countdown(system);
    queueos_screen_attribute(system, QUEUEOS_TEXT_RED_ON_RED);
    return QUEUEOS_EFI_SUCCESS;
}
