/* The database disk has no FAT. Product "Base" is the SCSI inquiry name
 * QEMU was given. Slot bytes and the one Base range are read with Block I/O.
 * Finding the disk and moving a span live in disk-io.c, so Base can write
 * a slot without linking this reader or Jansson.
 */

#include "base-disk.h"
#include "disk-io.h"
#include "disk.h"

#include <stdlib.h>

QUEUEOS_EFI_STATUS queueos_read_database_slots(
    QUEUEOS_EFI_SYSTEM_TABLE *system,
    QueueosGuestSlot *slots,
    int capacity)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    QueueosEfiBlockIo *block;
    uint8_t *region;
    int i;
    int n;

    if (slots == 0 || capacity < 1)
        return QUEUEOS_EFI_LOAD_ERROR;
    if (system == 0 || system->boot_services == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    boot = system->boot_services;
    block = queueos_disk_find_base(boot);
    if (block == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    region = malloc(QUEUEOS_SLOT_REGION);
    if (region == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    if (!queueos_disk_read(block, 0, QUEUEOS_SLOT_REGION, region)) {
        free(region);
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    n = capacity;
    if (n > QUEUEOS_SLOT_COUNT)
        n = QUEUEOS_SLOT_COUNT;
    for (i = 0; i < n; i++) {
        queueos_guest_read_slot(region + (size_t)i * QUEUEOS_SLOT_BYTES, &slots[i]);
    }
    free(region);
    return QUEUEOS_EFI_SUCCESS;
}

QUEUEOS_EFI_STATUS queueos_read_slot_image(
    QUEUEOS_EFI_SYSTEM_TABLE *system,
    const QueueosGuestSlot *slot,
    void **buffer,
    QUEUEOS_UINTN *size)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    QueueosEfiBlockIo *block;
    void *base;

    *buffer = 0;
    *size = 0;
    if (slot == 0 || system == 0 || system->boot_services == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    if (!queueos_guest_range_ok(slot->offset, slot->length))
        return QUEUEOS_EFI_LOAD_ERROR;
    if (slot->length > SIZE_MAX)
        return QUEUEOS_EFI_LOAD_ERROR;
    boot = system->boot_services;
    block = queueos_disk_find_base(boot);
    if (block == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    base = malloc((size_t)slot->length);
    if (base == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    if (!queueos_disk_read(block, slot->offset, slot->length, base)
        || !queueos_guest_digest_ok(base, (size_t)slot->length, slot->sha)) {
        free(base);
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    *buffer = base;
    *size = (QUEUEOS_UINTN)slot->length;
    return QUEUEOS_EFI_SUCCESS;
}
