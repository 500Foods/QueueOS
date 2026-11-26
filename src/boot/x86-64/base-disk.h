/* Read the launchable Base from the virtio-scsi disk whose product is Base.
 * The buffer is malloc memory. The caller frees it.
 */
#ifndef QUEUEOS_BASE_DISK_H
#define QUEUEOS_BASE_DISK_H

#include "slot-json.h"
#include "types.h"

/* Fills capacity slots, at most the ten on the disk. JSON text is malloc
 * memory. The caller releases every slot, including when this fails after
 * a partial fill. On failure before a slot is written, the array is
 * untouched.
 */
QUEUEOS_EFI_STATUS queueos_read_database_slots(
    QUEUEOS_EFI_SYSTEM_TABLE *system,
    QueueosGuestSlot *slots,
    int capacity);

/* Reads the Base range named by one already-classified slot. The buffer
 * is malloc memory. The caller frees it. A bad range or a bad digest
 * returns an error and leaves the buffer null.
 */
QUEUEOS_EFI_STATUS queueos_read_slot_image(
    QUEUEOS_EFI_SYSTEM_TABLE *system,
    const QueueosGuestSlot *slot,
    void **buffer,
    QUEUEOS_UINTN *size);

#endif
