/* malloc, free, and realloc over AllocatePool and FreePool.
 * efi_main binds the system table before Jansson runs. A call before
 * that bind returns NULL.
 */
#ifndef QUEUEOS_EFI_POOL_H
#define QUEUEOS_EFI_POOL_H

#include "types.h"

void queueos_pool_bind(QUEUEOS_EFI_SYSTEM_TABLE *system);

#endif
