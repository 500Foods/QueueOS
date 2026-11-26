/* The virtio-scsi disk whose product is Base. No FAT and no fs0: guess. */
#ifndef QUEUEOS_DISK_IO_H
#define QUEUEOS_DISK_IO_H

#include "block.h"

/* Null when the disk is absent. The handle buffer from the search is freed. */
QueueosEfiBlockIo *queueos_disk_find_base(QUEUEOS_EFI_BOOT_SERVICES *boot);

/* A short read or write returns 0. Write refuses a read-only disk.
 * The bounce buffer is malloc memory, so the pool is bound first.
 */
int queueos_disk_read(QueueosEfiBlockIo *block, uint64_t offset,
                      uint64_t length, uint8_t *dest);
int queueos_disk_write(QueueosEfiBlockIo *block, uint64_t offset,
                       uint64_t length, const uint8_t *src);

#endif
