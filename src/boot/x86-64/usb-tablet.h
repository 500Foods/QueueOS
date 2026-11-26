/* QEMU usb-tablet through EFI USB I/O.
 *
 * This OVMF build does not publish that device as an Absolute Pointer.
 * The tablet is still on the USB bus. virt-manager delivers the grabbed
 * pointer there, not to the PS/2 mouse.
 */
#ifndef QUEUEOS_USB_TABLET_H
#define QUEUEOS_USB_TABLET_H

#include "types.h"

void queueos_tablet_open(QUEUEOS_EFI_SYSTEM_TABLE *system);

/* 1 when a report set *x and *y. *down is the left button.
 * *wheel is the signed notch count from that report. Wheel-up is negative.
 */
int queueos_tablet_sample(int cols, int rows, int *x, int *y, int *down,
                          int *wheel);

#endif
