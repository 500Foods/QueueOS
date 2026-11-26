#include "pool.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

static QUEUEOS_EFI_BOOT_SERVICES *services;

void queueos_pool_bind(QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    services = 0;
    if (system != 0)
        services = system->boot_services;
}

/* Eight bytes in front of the caller pointer. AllocatePool is 8-aligned,
 * so the caller pointer stays aligned for a double. free reads the size
 * back from here because the firmware pool does not return it.
 */
struct pool_hdr {
    size_t size;
};

void *malloc(size_t size)
{
    struct pool_hdr *hdr;
    void *raw;
    size_t bytes;
    QUEUEOS_EFI_STATUS status;

    if (services == 0 || services->allocate_pool == 0)
        return 0;
    if (size > SIZE_MAX - sizeof *hdr)
        return 0;
    bytes = size + sizeof *hdr;
    raw = 0;
    status = services->allocate_pool(QUEUEOS_EFI_LOADER_DATA, bytes, &raw);
    if (status != QUEUEOS_EFI_SUCCESS || raw == 0)
        return 0;
    hdr = raw;
    hdr->size = size;
    return hdr + 1;
}

void free(void *ptr)
{
    struct pool_hdr *hdr;

    if (ptr == 0 || services == 0 || services->free_pool == 0)
        return;
    hdr = (struct pool_hdr *)ptr - 1;
    services->free_pool(hdr);
}

void *realloc(void *ptr, size_t size)
{
    struct pool_hdr *hdr;
    void *next;
    size_t copy;

    if (ptr == 0)
        return malloc(size);
    if (size == 0) {
        free(ptr);
        return 0;
    }
    hdr = (struct pool_hdr *)ptr - 1;
    next = malloc(size);
    if (next == 0)
        return 0;
    copy = hdr->size < size ? hdr->size : size;
    memcpy(next, ptr, copy);
    free(ptr);
    return next;
}
