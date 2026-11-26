/* Find the database disk and move a byte range.
 * Product "Base" is the SCSI inquiry name QEMU was given.
 */

#include "disk-io.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* EFI_BLOCK_IO_PROTOCOL_GUID 964e5b21-6459-11d2-8e39-00a0c969723b */
static const uint8_t BLOCK_IO_GUID[16] = {
    0x21, 0x5b, 0x4e, 0x96, 0x59, 0x64, 0xd2, 0x11,
    0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b
};

/* EFI_DISK_INFO_PROTOCOL_GUID d432a67f-14dc-484b-b3bb-3f0291849327 */
static const uint8_t DISK_INFO_GUID[16] = {
    0x7f, 0xa6, 0x32, 0xd4, 0xdc, 0x14, 0x4b, 0x48,
    0xb3, 0xbb, 0x3f, 0x02, 0x91, 0x84, 0x93, 0x27
};

static int product_is_base(const uint8_t *inquiry, uint32_t size)
{
    int i;

    if (size < 32)
        return 0;
    if (inquiry[16] != 'B' || inquiry[17] != 'a'
        || inquiry[18] != 's' || inquiry[19] != 'e')
        return 0;
    /* QEMU pads a short product with NULs. The inquiry spec uses spaces. */
    for (i = 4; i < 16; i++) {
        if (inquiry[16 + i] != ' ' && inquiry[16 + i] != 0)
            return 0;
    }
    return 1;
}

static int power_of_two(uint32_t value)
{
    return value != 0 && (value & (value - 1u)) == 0;
}

QueueosEfiBlockIo *queueos_disk_find_base(QUEUEOS_EFI_BOOT_SERVICES *boot)
{
    void **handles = 0;
    QUEUEOS_UINTN count = 0;
    QUEUEOS_UINTN i;
    QueueosEfiBlockIo *found = 0;
    QUEUEOS_EFI_STATUS status;

    if (boot == 0 || boot->locate_handle_buffer == 0 || boot->handle_protocol == 0)
        return 0;
    status = boot->locate_handle_buffer(2, BLOCK_IO_GUID, 0, &count, &handles);
    if (status != QUEUEOS_EFI_SUCCESS || handles == 0)
        return 0;
    for (i = 0; i < count; i++) {
        QueueosEfiBlockIo *block = 0;
        QueueosEfiDiskInfo *info = 0;
        uint8_t inquiry[256];
        uint32_t inquiry_size = (uint32_t)sizeof inquiry;

        status = boot->handle_protocol(handles[i], BLOCK_IO_GUID, (void **)&block);
        if (status != QUEUEOS_EFI_SUCCESS || block == 0 || block->media == 0)
            continue;
        if (!block->media->present || block->read_blocks == 0)
            continue;
        status = boot->handle_protocol(handles[i], DISK_INFO_GUID, (void **)&info);
        if (status != QUEUEOS_EFI_SUCCESS || info == 0 || info->inquiry == 0)
            continue;
        status = info->inquiry(info, inquiry, &inquiry_size);
        if (status != QUEUEOS_EFI_SUCCESS)
            continue;
        if (!product_is_base(inquiry, inquiry_size))
            continue;
        found = block;
        break;
    }
    if (boot->free_pool != 0)
        boot->free_pool(handles);
    return found;
}

static int prepare(QueueosEfiBlockIo *block, uint64_t offset, uint64_t length,
                   uint32_t *block_size, uint64_t *start, uint64_t *bytes,
                   uint8_t **raw, uint8_t **aligned)
{
    QueueosEfiBlockMedia *media;
    uint32_t align;
    uint64_t disk_bytes;
    uint64_t end;
    uintptr_t addr;

    if (block == 0 || block->media == 0 || raw == 0 || aligned == 0)
        return 0;
    media = block->media;
    *block_size = media->block_size;
    if (!power_of_two(*block_size))
        return 0;
    if (length == 0 || offset + length < offset)
        return 0;
    if (media->last_block == UINT64_MAX)
        return 0;
    if ((media->last_block + 1ull) > UINT64_MAX / *block_size)
        return 0;
    disk_bytes = (media->last_block + 1ull) * *block_size;
    if (offset + length > disk_bytes)
        return 0;
    *start = offset & ~((uint64_t)*block_size - 1ull);
    end = (offset + length + *block_size - 1ull) & ~((uint64_t)*block_size - 1ull);
    *bytes = end - *start;
    if (*bytes > SIZE_MAX)
        return 0;
    align = media->io_align;
    if (align < 16u || !power_of_two(align))
        align = 16u;
    *raw = malloc((size_t)*bytes + align);
    if (*raw == 0)
        return 0;
    addr = ((uintptr_t)*raw + align - 1u) & ~((uintptr_t)align - 1u);
    *aligned = (uint8_t *)addr;
    return 1;
}

int queueos_disk_read(QueueosEfiBlockIo *block, uint64_t offset,
                      uint64_t length, uint8_t *dest)
{
    uint32_t block_size = 0;
    uint64_t start = 0;
    uint64_t bytes = 0;
    uint8_t *raw = 0;
    uint8_t *aligned = 0;
    QUEUEOS_EFI_STATUS status;

    if (block == 0 || dest == 0 || block->read_blocks == 0)
        return 0;
    if (!prepare(block, offset, length, &block_size, &start, &bytes, &raw, &aligned))
        return 0;
    status = block->read_blocks(
        block,
        block->media->media_id,
        start / block_size,
        (QUEUEOS_UINTN)bytes,
        aligned);
    if (status != QUEUEOS_EFI_SUCCESS) {
        free(raw);
        return 0;
    }
    memcpy(dest, aligned + (size_t)(offset - start), (size_t)length);
    free(raw);
    return 1;
}

int queueos_disk_write(QueueosEfiBlockIo *block, uint64_t offset,
                       uint64_t length, const uint8_t *src)
{
    uint32_t block_size = 0;
    uint64_t start = 0;
    uint64_t bytes = 0;
    uint8_t *raw = 0;
    uint8_t *aligned = 0;
    QUEUEOS_EFI_STATUS status;

    if (block == 0 || src == 0 || block->media == 0
        || block->read_blocks == 0 || block->write_blocks == 0)
        return 0;
    if (block->media->read_only)
        return 0;
    if (!prepare(block, offset, length, &block_size, &start, &bytes, &raw, &aligned))
        return 0;
    status = block->read_blocks(
        block,
        block->media->media_id,
        start / block_size,
        (QUEUEOS_UINTN)bytes,
        aligned);
    if (status != QUEUEOS_EFI_SUCCESS) {
        free(raw);
        return 0;
    }
    memcpy(aligned + (size_t)(offset - start), src, (size_t)length);
    status = block->write_blocks(
        block,
        block->media->media_id,
        start / block_size,
        (QUEUEOS_UINTN)bytes,
        aligned);
    if (status == QUEUEOS_EFI_SUCCESS && block->flush_blocks != 0)
        status = block->flush_blocks(block);
    free(raw);
    return status == QUEUEOS_EFI_SUCCESS;
}
