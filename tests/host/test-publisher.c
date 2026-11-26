/* Host checks for the publisher. Plan 003, step 1.
 * The image under test is a memory buffer. Nothing here opens images/.
 */

#include "disk.h"
#include "publish.h"
#include "sha256.h"

#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char EMPTY_HASH[] =
    "44136fa355b3678a1146ad16f7e8649e94fb4fc21fe77e8310c060f61caaff8a";

static size_t image_bytes(void)
{
    return (size_t)QUEUEOS_BODY_START + 32u;
}

static uint8_t *slot_at(uint8_t *image, int slot)
{
    return image + (size_t)(slot - 1) * QUEUEOS_SLOT_BYTES;
}

static uint8_t *new_image(void)
{
    uint8_t *image = calloc(1, image_bytes());
    TEST_ASSERT_NOT_NULL(image);
    return image;
}

static void expect_tail_clear(const uint8_t *image)
{
    size_t i;

    for (i = QUEUEOS_BODY_START; i < image_bytes(); i++)
        TEST_ASSERT_EQUAL_UINT8(0, image[i]);
}

static void expect_hash_line(const uint8_t *slot)
{
    const uint8_t *nl = memchr(slot, '\n', QUEUEOS_SLOT_BYTES);
    uint8_t digest[32];
    char hex[65];
    char line[80];

    TEST_ASSERT_NOT_NULL(nl);
    queueos_sha256(slot, (size_t)(nl - slot), digest);
    queueos_sha256_hex(digest, hex);
    snprintf(line, sizeof line, "sha256 %s\n", hex);
    TEST_ASSERT_EQUAL_MEMORY(line, nl + 1, strlen(line));
}

static void expect_text(const uint8_t *slot, const char *text)
{
    char buf[QUEUEOS_SLOT_BYTES + 1];

    memcpy(buf, slot, QUEUEOS_SLOT_BYTES);
    buf[QUEUEOS_SLOT_BYTES] = '\0';
    TEST_ASSERT_NOT_NULL(strstr(buf, text));
}

static unsigned long long json_offset(const uint8_t *slot)
{
    char buf[QUEUEOS_SLOT_BYTES + 1];
    char *at;

    memcpy(buf, slot, QUEUEOS_SLOT_BYTES);
    buf[QUEUEOS_SLOT_BYTES] = '\0';
    at = strstr(buf, "\"offset\":");
    TEST_ASSERT_NOT_NULL(at);
    return strtoull(at + 9, NULL, 10);
}

static int count_bytes(const uint8_t *hay, size_t hay_len, const void *needle, size_t n)
{
    int count = 0;
    size_t i;

    if (n == 0 || hay_len < n)
        return 0;
    for (i = 0; i + n <= hay_len; i++) {
        if (memcmp(hay + i, needle, n) == 0)
            count++;
    }
    return count;
}

static int publish_one(uint8_t *image, const char *name, const char *published,
                       const void *bytes, size_t len, int *slot)
{
    QueueosPayload payload;
    QueueosGeneration gen;

    memset(&payload, 0, sizeof payload);
    payload.role = "base";
    payload.arch = "x86-64";
    payload.file_label = "Base.efi";
    payload.bytes = bytes;
    payload.length = len;
    memset(&gen, 0, sizeof gen);
    gen.name = name;
    gen.version = "0";
    gen.published = published;
    gen.payloads = &payload;
    gen.payload_count = 1;
    return queueos_publish(image, image_bytes(), &gen, slot);
}

static void add_booted(uint8_t *slot, const char *stamp)
{
    TEST_ASSERT_EQUAL_INT(0, queueos_slot_mark_booted(slot, stamp));
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_sha256_agrees_on_empty_object(void)
{
    uint8_t digest[32];
    char hex[65];
    uint8_t slot[QUEUEOS_SLOT_BYTES];
    char expect[80];
    size_t prefix;

    queueos_sha256((const uint8_t *)"{}", 2, digest);
    queueos_sha256_hex(digest, hex);
    TEST_ASSERT_EQUAL_STRING(EMPTY_HASH, hex);

    queueos_sha256((const uint8_t *)"", 0, digest);
    queueos_sha256_hex(digest, hex);
    TEST_ASSERT_EQUAL_STRING(
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", hex);

    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(slot, "{}", 2, NULL));
    prefix = 2u + 1u + 7u + 64u + 1u;
    snprintf(expect, sizeof expect, "{}\nsha256 %s\n", EMPTY_HASH);
    TEST_ASSERT_EQUAL_MEMORY(expect, slot, prefix);
    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(slot, "{}", 2, "2026-11-25T00:00:00Z"));
    expect_hash_line(slot);
    expect_text(slot, "booted 2026-11-25T00:00:00Z\n");
}

void test_first_publish_on_a_blank_image(void)
{
    uint8_t *image = new_image();
    const char payload[] = "blank-base";
    int slot = 0;
    int i;
    char needle[64];

    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
        publish_one(image, "stub", "2026-11-25T00:00:00Z", payload, sizeof payload - 1, &slot));
    TEST_ASSERT_EQUAL_INT(1, slot);
    expect_hash_line(slot_at(image, 1));
    expect_text(slot_at(image, 1), "\"OS\":\"QueueOS\"");
    expect_text(slot_at(image, 1), "\"name\":\"stub\"");
    snprintf(needle, sizeof needle, "\"offset\":%llu", (unsigned long long)QUEUEOS_SLOT_REGION);
    expect_text(slot_at(image, 1), needle);
    TEST_ASSERT_EQUAL_UINT64(QUEUEOS_SLOT_REGION, json_offset(slot_at(image, 1)));
    TEST_ASSERT_EQUAL_MEMORY(payload, image + QUEUEOS_SLOT_REGION, sizeof payload - 1);
    for (i = 2; i <= QUEUEOS_SLOT_COUNT; i++) {
        size_t n;
        for (n = 0; n < QUEUEOS_SLOT_BYTES; n++)
            TEST_ASSERT_EQUAL_UINT8(0, slot_at(image, i)[n]);
    }
    expect_tail_clear(image);
    free(image);
}

void test_first_publish_on_empty_slots(void)
{
    uint8_t *image = new_image();
    uint8_t empty[QUEUEOS_SLOT_BYTES];
    const char payload[] = "empty-base";
    int slot = 0;
    int i;

    TEST_ASSERT_EQUAL_INT(0, queueos_disk_empty(image, image_bytes()));
    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(empty, "{}", 2, NULL));
    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
        publish_one(image, "stub", "2026-11-25T00:00:00Z", payload, sizeof payload - 1, &slot));
    TEST_ASSERT_EQUAL_INT(1, slot);
    expect_text(slot_at(image, 1), "\"name\":\"stub\"");
    for (i = 2; i <= QUEUEOS_SLOT_COUNT; i++)
        TEST_ASSERT_EQUAL_MEMORY(empty, slot_at(image, i), QUEUEOS_SLOT_BYTES);
    TEST_ASSERT_EQUAL_MEMORY(payload, image + QUEUEOS_SLOT_REGION, sizeof payload - 1);
    expect_tail_clear(image);
    free(image);
}

void test_identical_sha256_stored_once(void)
{
    uint8_t *image = new_image();
    const char payload[] = "same-bytes";
    int slot = 0;

    TEST_ASSERT_EQUAL_INT(0, queueos_disk_empty(image, image_bytes()));
    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
        publish_one(image, "one", "2026-11-01T00:00:00Z", payload, sizeof payload - 1, &slot));
    TEST_ASSERT_EQUAL_INT(1, slot);
    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
        publish_one(image, "two", "2026-11-02T00:00:00Z", payload, sizeof payload - 1, &slot));
    TEST_ASSERT_EQUAL_INT(2, slot);
    TEST_ASSERT_EQUAL_UINT64(json_offset(slot_at(image, 1)), json_offset(slot_at(image, 2)));
    TEST_ASSERT_EQUAL_UINT64(QUEUEOS_SLOT_REGION, json_offset(slot_at(image, 2)));
    TEST_ASSERT_EQUAL_INT(1, count_bytes(image, image_bytes(), payload, sizeof payload - 1));
    expect_text(slot_at(image, 1), "\"name\":\"one\"");
    expect_text(slot_at(image, 2), "\"name\":\"two\"");
    expect_tail_clear(image);
    free(image);
}

void test_publish_that_does_not_fit_leaves_the_image(void)
{
    size_t span = (size_t)QUEUEOS_BODY_START - QUEUEOS_SLOT_REGION;
    uint8_t *image = new_image();
    uint8_t *payload = malloc(span);
    uint8_t *before = malloc(image_bytes());
    uint8_t digest[32];
    char hex[65];
    char json[320];
    const char extra[] = "x";
    int slot = 0;
    int n;

    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_NOT_NULL(before);
    memset(payload, 0xA5, span);
    TEST_ASSERT_EQUAL_INT(0, queueos_disk_empty(image, image_bytes()));
    memcpy(image + QUEUEOS_SLOT_REGION, payload, span);
    queueos_sha256(payload, span, digest);
    queueos_sha256_hex(digest, hex);
    n = snprintf(json, sizeof json,
        "{\"OS\":\"QueueOS\",\"name\":\"full\",\"version\":\"0\","
        "\"published\":\"2026-11-01T00:00:00Z\",\"binaries\":[{\"role\":\"base\","
        "\"arch\":\"x86-64\",\"offset\":%llu,\"length\":%llu,\"sha256\":\"%s\","
        "\"file\":\"Base.efi\"}]}",
        (unsigned long long)QUEUEOS_SLOT_REGION,
        (unsigned long long)span,
        hex);
    TEST_ASSERT_TRUE(n > 0 && (size_t)n < sizeof json);
    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(slot_at(image, 1), json, (size_t)n, NULL));
    memcpy(before, image, image_bytes());
    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_REFUSED,
        publish_one(image, "overflow", "2026-12-01T00:00:00Z", extra, 1, &slot));
    TEST_ASSERT_EQUAL_MEMORY(before, image, image_bytes());
    free(before);
    free(payload);
    free(image);
}

void test_only_completion_keeps_the_oldest_slot(void)
{
    uint8_t *image = new_image();
    uint8_t saved_oldest[QUEUEOS_SLOT_BYTES];
    uint8_t saved_rest[8][QUEUEOS_SLOT_BYTES];
    const char payload[] = "kept-base";
    const char newer[] = "newer-base";
    int slot = 0;
    int i;

    TEST_ASSERT_EQUAL_INT(0, queueos_disk_empty(image, image_bytes()));
    for (i = 1; i <= QUEUEOS_SLOT_COUNT; i++) {
        char stamp[32];
        char name[16];
        snprintf(stamp, sizeof stamp, "2026-11-%02dT00:00:00Z", i);
        snprintf(name, sizeof name, "gen-%02d", i);
        TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
            publish_one(image, name, stamp, payload, sizeof payload - 1, &slot));
        TEST_ASSERT_EQUAL_INT(i, slot);
    }
    add_booted(slot_at(image, 1), "2026-11-25T00:00:00Z");
    memcpy(saved_oldest, slot_at(image, 1), QUEUEOS_SLOT_BYTES);
    for (i = 0; i < 8; i++)
        memcpy(saved_rest[i], slot_at(image, i + 3), QUEUEOS_SLOT_BYTES);

    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
        publish_one(image, "newer", "2026-12-01T00:00:00Z", newer, sizeof newer - 1, &slot));
    TEST_ASSERT_EQUAL_INT(2, slot);
    TEST_ASSERT_EQUAL_MEMORY(saved_oldest, slot_at(image, 1), QUEUEOS_SLOT_BYTES);
    expect_text(slot_at(image, 1), "booted 2026-11-25T00:00:00Z\n");
    expect_text(slot_at(image, 2), "\"name\":\"newer\"");
    for (i = 0; i < 8; i++)
        TEST_ASSERT_EQUAL_MEMORY(saved_rest[i], slot_at(image, i + 3), QUEUEOS_SLOT_BYTES);
    TEST_ASSERT_EQUAL_INT(1, count_bytes(image, image_bytes(), payload, sizeof payload - 1));
    TEST_ASSERT_EQUAL_INT(1, count_bytes(image, image_bytes(), newer, sizeof newer - 1));
    expect_tail_clear(image);
    free(image);
}

void test_unsafe_booted_line_is_replaced(void)
{
    uint8_t *image = new_image();
    uint8_t saved[QUEUEOS_SLOT_BYTES];
    const char payload[] = "live-base";
    const char json[] =
        "{\"OS\":\"QueueOS\",\"name\":\"bad\",\"version\":\"0\","
        "\"published\":\"2026-01-01T00:00:00Z\"}";
    int slot = 0;
    int i;
    uint8_t *hash_digit;

    TEST_ASSERT_EQUAL_INT(0, queueos_disk_empty(image, image_bytes()));
    memset(slot_at(image, 1), 0, QUEUEOS_SLOT_BYTES);
    for (i = 2; i <= QUEUEOS_SLOT_COUNT; i++) {
        char stamp[32];
        char name[16];
        snprintf(stamp, sizeof stamp, "2026-11-%02dT00:00:00Z", i);
        snprintf(name, sizeof name, "gen-%02d", i);
        TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
            publish_one(image, name, stamp, payload, sizeof payload - 1, &slot));
        TEST_ASSERT_EQUAL_INT(i, slot);
    }
    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(slot_at(image, 1), json, strlen(json),
        "2026-11-25T00:00:00Z"));
    hash_digit = memchr(slot_at(image, 1), '\n', QUEUEOS_SLOT_BYTES);
    TEST_ASSERT_NOT_NULL(hash_digit);
    hash_digit += 8;
    *hash_digit = (uint8_t)(*hash_digit == '0' ? '1' : '0');
    memcpy(saved, slot_at(image, 2), QUEUEOS_SLOT_BYTES);

    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK,
        publish_one(image, "fixed", "2026-12-01T00:00:00Z", "Z", 1, &slot));
    TEST_ASSERT_EQUAL_INT(1, slot);
    expect_text(slot_at(image, 1), "\"name\":\"fixed\"");
    TEST_ASSERT_EQUAL_MEMORY(saved, slot_at(image, 2), QUEUEOS_SLOT_BYTES);
    free(image);
}

void test_mark_writes_after_the_hash(void)
{
    uint8_t slot[QUEUEOS_SLOT_BYTES];
    uint8_t saved[QUEUEOS_SLOT_BYTES];
    const char json[] = "{\"OS\":\"QueueOS\",\"name\":\"stub\"}";

    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(slot, json, strlen(json), NULL));
    TEST_ASSERT_EQUAL_INT(0, queueos_slot_mark_booted(slot, "2026-11-25T12:00:00Z"));
    expect_hash_line(slot);
    expect_text(slot, "booted 2026-11-25T12:00:00Z\n");
    memcpy(saved, slot, sizeof saved);
    TEST_ASSERT_EQUAL_INT(0, queueos_slot_mark_booted(slot, "2026-12-01T00:00:00Z"));
    TEST_ASSERT_EQUAL_MEMORY(saved, slot, sizeof saved);
}

void test_mark_refuses_a_bad_hash_and_a_dirty_tail(void)
{
    uint8_t slot[QUEUEOS_SLOT_BYTES];
    uint8_t saved[QUEUEOS_SLOT_BYTES];
    const char json[] = "{\"OS\":\"QueueOS\",\"name\":\"bad\"}";
    uint8_t *digit;
    uint8_t *nl2;

    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(slot, json, strlen(json), NULL));
    TEST_ASSERT_EQUAL_INT(-1, queueos_slot_mark_booted(slot, "bad stamp"));
    expect_hash_line(slot);

    digit = memchr(slot, '\n', QUEUEOS_SLOT_BYTES);
    TEST_ASSERT_NOT_NULL(digit);
    digit += 8;
    *digit = (uint8_t)(*digit == '0' ? '1' : '0');
    memcpy(saved, slot, sizeof saved);
    TEST_ASSERT_EQUAL_INT(-1, queueos_slot_mark_booted(slot, "2026-11-25T00:00:00Z"));
    TEST_ASSERT_EQUAL_MEMORY(saved, slot, sizeof saved);

    TEST_ASSERT_EQUAL_INT(0, queueos_slot_write(slot, json, strlen(json), NULL));
    nl2 = memchr(slot, '\n', QUEUEOS_SLOT_BYTES);
    TEST_ASSERT_NOT_NULL(nl2);
    nl2 = memchr(nl2 + 1, '\n', QUEUEOS_SLOT_BYTES - (size_t)(nl2 + 1 - slot));
    TEST_ASSERT_NOT_NULL(nl2);
    nl2[1] = 'x';
    memcpy(saved, slot, sizeof saved);
    TEST_ASSERT_EQUAL_INT(-1, queueos_slot_mark_booted(slot, "2026-11-25T00:00:00Z"));
    TEST_ASSERT_EQUAL_MEMORY(saved, slot, sizeof saved);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sha256_agrees_on_empty_object);
    RUN_TEST(test_first_publish_on_a_blank_image);
    RUN_TEST(test_first_publish_on_empty_slots);
    RUN_TEST(test_identical_sha256_stored_once);
    RUN_TEST(test_publish_that_does_not_fit_leaves_the_image);
    RUN_TEST(test_only_completion_keeps_the_oldest_slot);
    RUN_TEST(test_unsafe_booted_line_is_replaced);
    RUN_TEST(test_mark_writes_after_the_hash);
    RUN_TEST(test_mark_refuses_a_bad_hash_and_a_dirty_tail);
    return UNITY_END();
}
