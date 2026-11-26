/* Block I/O and Disk Info, only the fields Boot calls.
 * The media layout is the natural alignment from the UEFI spec.
 * block_size is at byte 12 and last_block is at byte 24.
 */
#ifndef QUEUEOS_EFI_BLOCK_H
#define QUEUEOS_EFI_BLOCK_H

#include "types.h"

#include <stddef.h>
#include <stdint.h>

typedef struct QueueosEfiBlockMedia {
    uint32_t media_id;
    uint8_t removable;
    uint8_t present;
    uint8_t logical;
    uint8_t read_only;
    uint8_t write_caching;
    uint32_t block_size;
    uint32_t io_align;
    uint64_t last_block;
} QueueosEfiBlockMedia;

typedef struct QueueosEfiBlockIo QueueosEfiBlockIo;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QueueosEfiReadBlocks)(
    QueueosEfiBlockIo *self,
    uint32_t media_id,
    uint64_t lba,
    QUEUEOS_UINTN buffer_size,
    void *buffer);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QueueosEfiWriteBlocks)(
    QueueosEfiBlockIo *self,
    uint32_t media_id,
    uint64_t lba,
    QUEUEOS_UINTN buffer_size,
    void *buffer);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QueueosEfiFlushBlocks)(
    QueueosEfiBlockIo *self);

struct QueueosEfiBlockIo {
    uint64_t revision;
    QueueosEfiBlockMedia *media;
    void *reset;
    QueueosEfiReadBlocks read_blocks;
    QueueosEfiWriteBlocks write_blocks;
    QueueosEfiFlushBlocks flush_blocks;
};

typedef struct QueueosEfiDiskInfo QueueosEfiDiskInfo;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QueueosEfiInquiry)(
    QueueosEfiDiskInfo *self,
    void *data,
    uint32_t *size);

/* Interface is an EFI_GUID. Inquiry is the second field. */
struct QueueosEfiDiskInfo {
    uint8_t interface_guid[16];
    QueueosEfiInquiry inquiry;
    void *identify;
    void *sense;
    void *which_ide;
};

_Static_assert(offsetof(QueueosEfiBlockMedia, block_size) == 12,
               "block size offset");
_Static_assert(offsetof(QueueosEfiBlockMedia, last_block) == 24,
               "last block offset");

#endif
