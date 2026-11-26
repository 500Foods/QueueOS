#ifndef QUEUEOS_SHA256_H
#define QUEUEOS_SHA256_H

#include <stddef.h>
#include <stdint.h>

/* FIPS 180-4 SHA-256. One implementation for the publisher and for
 * firmware Boot. No allocation and no FILE, so the guest can link it.
 * hex receives 64 lowercase digits and a NUL.
 */

typedef struct QueueosSha256 {
    uint32_t state[8];
    uint64_t bits;
    uint8_t block[64];
    size_t used;
} QueueosSha256;

void queueos_sha256_init(QueueosSha256 *s);
void queueos_sha256_update(QueueosSha256 *s, const uint8_t *data, size_t len);
void queueos_sha256_final(QueueosSha256 *s, uint8_t out[32]);

void queueos_sha256(const uint8_t *data, size_t len, uint8_t out[32]);
void queueos_sha256_hex(const uint8_t digest[32], char hex[65]);

#endif
