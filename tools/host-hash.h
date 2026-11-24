#ifndef QUEUEOS_HOST_HASH_H
#define QUEUEOS_HOST_HASH_H

#include <stddef.h>
#include <stdint.h>

/* crc32 is the same polynomial zlib uses. sha256 is FIPS 180-4.
 * The image builder needs both and does not link a crypto library.
 * hex receives 64 lowercase digits and a NUL. */

uint32_t queueos_crc32(const uint8_t *data, size_t len);

int queueos_sha256_buf(const uint8_t *data, size_t len, char hex[65]);

int queueos_sha256_file(const char *path, char hex[65]);

#endif
