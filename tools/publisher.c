/* publisher.c
 *
 * Publish one QueueOS generation into a raw database image.
 * The program rewrites that file. It does not open images/ itself.
 * A qcow2 is converted by the caller. On refusal the file is unchanged.
 *
 * usage: publisher --image FILE --name NAME --version VER --published STAMP
 *                  --add ROLE ARCH LABEL PATH [--add ...]
 */

#define _XOPEN_SOURCE 700

#include "publish.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { MAX_ADD = 8, PATH_CAP = 4096 };

typedef struct Add {
    const char *role;
    const char *arch;
    const char *label;
    const char *path;
    uint8_t *bytes;
    size_t length;
} Add;

static void usage(void)
{
    fprintf(stderr,
        "usage: publisher --image FILE --name NAME --version VER --published STAMP\n"
        "                 --add ROLE ARCH LABEL PATH [--add ...]\n");
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
    char tmp[PATH_CAP];
    FILE *fp;
    int fd;

    if (snprintf(tmp, sizeof tmp, "%s.tmp", path) >= (int)sizeof tmp)
        return -1;
    fp = fopen(tmp, "wb");
    if (fp == NULL)
        return -1;
    if (len > 0 && fwrite(data, 1, len, fp) != len) {
        fclose(fp);
        unlink(tmp);
        return -1;
    }
    if (fflush(fp) != 0) {
        fclose(fp);
        unlink(tmp);
        return -1;
    }
    fd = fileno(fp);
    if (fd < 0 || fsync(fd) != 0) {
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

static const char *need(int *index, int argc, char **argv)
{
    if (*index + 1 >= argc) {
        usage();
        return NULL;
    }
    *index += 1;
    return argv[*index];
}

int main(int argc, char **argv)
{
    const char *image_path = NULL;
    const char *name = NULL;
    const char *version = NULL;
    const char *published = NULL;
    Add adds[MAX_ADD];
    QueueosPayload payloads[MAX_ADD];
    QueueosGeneration gen;
    uint8_t *image = NULL;
    size_t image_len = 0;
    int add_count = 0;
    int slot = 0;
    int i;
    int rc;

    memset(adds, 0, sizeof adds);
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--image") == 0) {
            image_path = need(&i, argc, argv);
        } else if (strcmp(argv[i], "--name") == 0) {
            name = need(&i, argc, argv);
        } else if (strcmp(argv[i], "--version") == 0) {
            version = need(&i, argc, argv);
        } else if (strcmp(argv[i], "--published") == 0) {
            published = need(&i, argc, argv);
        } else if (strcmp(argv[i], "--add") == 0) {
            if (add_count >= MAX_ADD) {
                fprintf(stderr, "publisher: too many payloads\n");
                return 2;
            }
            adds[add_count].role = need(&i, argc, argv);
            adds[add_count].arch = need(&i, argc, argv);
            adds[add_count].label = need(&i, argc, argv);
            adds[add_count].path = need(&i, argc, argv);
            if (adds[add_count].path == NULL)
                return 2;
            add_count++;
        } else {
            usage();
            return 2;
        }
    }
    if (image_path == NULL || name == NULL || version == NULL || published == NULL ||
        add_count < 1) {
        usage();
        return 2;
    }

    image = read_whole(image_path, &image_len);
    if (image == NULL) {
        fprintf(stderr, "publisher: %s: %s\n", image_path, strerror(errno));
        return 2;
    }
    for (i = 0; i < add_count; i++) {
        adds[i].bytes = read_whole(adds[i].path, &adds[i].length);
        if (adds[i].bytes == NULL || adds[i].length < 1u) {
            fprintf(stderr, "publisher: %s: %s\n", adds[i].path, strerror(errno));
            rc = 2;
            goto out;
        }
        payloads[i].role = adds[i].role;
        payloads[i].arch = adds[i].arch;
        payloads[i].file_label = adds[i].label;
        payloads[i].bytes = adds[i].bytes;
        payloads[i].length = adds[i].length;
    }

    memset(&gen, 0, sizeof gen);
    gen.name = name;
    gen.version = version;
    gen.published = published;
    gen.payloads = payloads;
    gen.payload_count = (size_t)add_count;
    rc = queueos_publish(image, image_len, &gen, &slot);
    if (rc == QUEUEOS_PUBLISH_REFUSED) {
        fprintf(stderr, "publisher: refused, image unchanged\n");
        rc = 1;
        goto out;
    }
    if (rc != QUEUEOS_PUBLISH_OK) {
        fprintf(stderr, "publisher: bad generation or image\n");
        rc = 2;
        goto out;
    }
    if (write_replace(image_path, image, image_len) != 0) {
        fprintf(stderr, "publisher: %s: %s\n", image_path, strerror(errno));
        rc = 2;
        goto out;
    }
    printf("publisher: slot %d\n", slot);
    rc = 0;

out:
    for (i = 0; i < add_count; i++)
        free(adds[i].bytes);
    free(image);
    return rc;
}
