/* gcc emits memcpy and memset even in freestanding code. libc is not linked.
 *
 * queueos_reloc_anchor is a pointer to itself. The preferred PE base is 0,
 * which firmware cannot use, so the image needs a base relocation. objcopy
 * turns this relative pointer into that table. Without it the PE is marked
 * relocations-stripped and the load is refused.
 */

#include <stddef.h>
#include <stdint.h>

void *queueos_reloc_anchor = &queueos_reloc_anchor;

void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *out = dst;
    const uint8_t *in = src;
    size_t i;

    for (i = 0; i < n; i++) {
        out[i] = in[i];
    }
    return dst;
}

void *memset(void *dst, int value, size_t n)
{
    uint8_t *out = dst;
    size_t i;

    for (i = 0; i < n; i++) {
        out[i] = (uint8_t)value;
    }
    return dst;
}
