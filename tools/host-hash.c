#include "host-hash.h"

#include "sha256.h"

#include <stdio.h>

uint32_t queueos_crc32(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    size_t i;

    for (i = 0; i < len; i++) {
        int bit;
        crc ^= data[i];
        for (bit = 0; bit < 8; bit++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

int queueos_sha256_buf(const uint8_t *data, size_t len, char hex[65])
{
    uint8_t out[32];

    queueos_sha256(data, len, out);
    queueos_sha256_hex(out, hex);
    return 0;
}

int queueos_sha256_file(const char *path, char hex[65])
{
    FILE *fp;
    QueueosSha256 s;
    uint8_t buf[8192];
    uint8_t out[32];
    size_t n;

    fp = fopen(path, "rb");
    if (!fp)
        return -1;
    queueos_sha256_init(&s);
    while ((n = fread(buf, 1, sizeof buf, fp)) > 0)
        queueos_sha256_update(&s, buf, n);
    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    queueos_sha256_final(&s, out);
    queueos_sha256_hex(out, hex);
    return 0;
}
