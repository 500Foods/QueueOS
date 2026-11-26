/* db-image.c
 *
 * Writes the first database disk: ten slots, two stub generations, and
 * an empty body. The published times are fixed text, so a second build
 * matches the first. The body start lives in disk.h. This file does
 * not copy it. Both generations name the same Base bytes.
 *
 * usage: db-image OUTPUT BASE.efi
 *        db-image --check RAW BASE.efi
 */

#define _XOPEN_SOURCE 700

#include "disk.h"
#include "publish.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { DISK_BYTES = 64 * 1024 * 1024 };

static void die(const char *msg)
{
    fprintf(stderr, "db-image: %s\n", msg);
    exit(1);
}

static void die_errno(const char *what)
{
    fprintf(stderr, "db-image: %s: %s\n", what, strerror(errno));
    exit(1);
}

static uint8_t *read_whole(const char *path, size_t *len)
{
    FILE *fp;
    long size;
    uint8_t *buf;

    fp = fopen(path, "rb");
    if (fp == NULL)
        return NULL;
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }
    if (fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    buf = malloc((size_t)size == 0 ? 1 : (size_t)size);
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }
    if (size > 0 && fread(buf, 1, (size_t)size, fp) != (size_t)size) {
        free(buf);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    *len = (size_t)size;
    return buf;
}

static int write_replace(const char *path, const uint8_t *data, size_t len)
{
    char tmp[4096];
    FILE *fp;
    int fd;

    if (snprintf(tmp, sizeof tmp, "%s.tmp", path) >= (int)sizeof tmp)
        return -1;
    fp = fopen(tmp, "wb");
    if (fp == NULL)
        return -1;
    if (fwrite(data, 1, len, fp) != len) {
        fclose(fp);
        unlink(tmp);
        return -1;
    }
    if (fflush(fp) != 0 || (fd = fileno(fp)) < 0 || fsync(fd) != 0) {
        fclose(fp);
        unlink(tmp);
        return -1;
    }
    if (fclose(fp) != 0) {
        unlink(tmp);
        return -1;
    }
    if (rename(tmp, path) != 0) {
        unlink(tmp);
        return -1;
    }
    return 0;
}

static void publish_stub(uint8_t *image, const uint8_t *base, size_t base_len,
                         const char *version, const char *published,
                         int countdown, int expect_slot)
{
    QueueosPayload payload;
    QueueosGeneration gen;
    int slot = 0;
    int rc;

    memset(&payload, 0, sizeof payload);
    payload.role = "base";
    payload.arch = "x86-64";
    payload.file_label = "Base.efi";
    payload.bytes = base;
    payload.length = base_len;
    memset(&gen, 0, sizeof gen);
    gen.name = "stub";
    gen.version = version;
    gen.published = published;
    gen.countdown = countdown;
    gen.payloads = &payload;
    gen.payload_count = 1;
    rc = queueos_publish(image, DISK_BYTES, &gen, &slot);
    if (rc == QUEUEOS_PUBLISH_REFUSED)
        die("Base.efi does not fit in the boot range");
    if (rc != QUEUEOS_PUBLISH_OK)
        die("could not publish the stub generation");
    if (slot != expect_slot)
        die("stub generation was not written to the expected slot");
}

/* Ten empty slots, then two stub generations. The newer stamp is slot 1
 * so the list shows it first. Base.efi is stored once. */
static uint8_t *build_image(const uint8_t *base, size_t base_len)
{
    uint8_t *image;

    image = calloc(1, DISK_BYTES);
    if (image == NULL)
        die("out of memory");
    if (queueos_disk_empty(image, DISK_BYTES) != 0)
        die("could not write empty slots");
    publish_stub(image, base, base_len, "1.0.1", "2026-11-25T00:00:01Z", 10, 1);
    publish_stub(image, base, base_len, "1.0.0", "2026-11-25T00:00:00Z", 30, 2);
    return image;
}

static void check_image(const char *raw_path, const char *base_path)
{
    size_t base_len = 0;
    size_t raw_len = 0;
    uint8_t *base;
    uint8_t *raw;
    uint8_t *expect;

    base = read_whole(base_path, &base_len);
    if (base == NULL || base_len < 1u)
        die_errno(base_path);
    raw = read_whole(raw_path, &raw_len);
    if (raw == NULL)
        die_errno(raw_path);
    expect = build_image(base, base_len);
    if (raw_len != DISK_BYTES || memcmp(raw, expect, DISK_BYTES) != 0)
        die("database image is not the two stub generations");
    free(expect);
    free(raw);
    free(base);
}

static void write_image(const char *out_path, const char *base_path)
{
    size_t base_len = 0;
    uint8_t *base;
    uint8_t *image;

    base = read_whole(base_path, &base_len);
    if (base == NULL || base_len < 1u)
        die_errno(base_path);
    image = build_image(base, base_len);
    if (write_replace(out_path, image, DISK_BYTES) != 0)
        die_errno(out_path);
    free(image);
    free(base);
}

int main(int argc, char **argv)
{
    if (argc == 4 && strcmp(argv[1], "--check") == 0) {
        check_image(argv[2], argv[3]);
        return 0;
    }
    if (argc == 3) {
        write_image(argv[1], argv[2]);
        return 0;
    }
    fprintf(stderr, "usage: db-image OUTPUT BASE.efi\n"
                    "       db-image --check RAW BASE.efi\n");
    return 1;
}
