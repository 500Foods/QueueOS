/* Host checks for the body ring. The image under test is a memory
 * buffer. Nothing here opens images/.
 */

#include "body-page.h"
#include "disk.h"
#include "publish.h"
#include "sha256.h"

#include "unity.h"

#include <stdlib.h>
#include <string.h>

static const char STAMP[] = "2026-11-25T00:00:00Z";
static const char STAMP_NEXT[] = "2026-11-25T00:00:01Z";
static const char SLOT1_LINE[] = "stored 2026-11-25T00:00:00Z slot 1";
static const char SLOT1_JSON[] = "[{\"stored\":\"2026-11-25T00:00:00Z\",\"slot\":1}]";
static const char SLOT1_HASH[] =
    "fe46aee0aa27eade88658f2037557cde0a585d52ef1291cec0805225d38b6570";
static const char SLOT10_LINE[] = "stored 2026-11-25T00:00:00Z slot 10";
static const char SLOT10_JSON[] = "[{\"stored\":\"2026-11-25T00:00:00Z\",\"slot\":10}]";
static const char SLOT10_HASH[] =
    "9aa471dae1ea3306b0deeae9cd8581c09f0fa59d7d63b1f1e051a2d9e114ced0";
static const char EMPTY_HASH[] =
    "4f53cda18c2baa0c0354bb5f9a3ecbe5ed12ab4d8e11ba873c2f11161202b945";
static const char OLD_LINE[] = "stored 2026-11-25T00:00:00Z slot 1";
static const char OLD_HASH[] =
    "b5584f1334084037dd6be0610f569914deac15d8651c367ec5b7f98e5edc59bf";

static void lay_json(uint8_t page[QUEUEOS_BODY_PAGE], const char *json)
{
    uint8_t digest[32];
    char hex[65];
    size_t n = strlen(json);

    memset(page, 0, QUEUEOS_BODY_PAGE);
    TEST_ASSERT_TRUE(n + 73u <= QUEUEOS_BODY_PAGE);
    memcpy(page, json, n);
    page[n] = '\n';
    queueos_sha256((const uint8_t *)json, n, digest);
    queueos_sha256_hex(digest, hex);
    memcpy(page + n + 1u, "sha256 ", 7);
    memcpy(page + n + 8u, hex, 64);
    page[n + 72u] = '\n';
}

static void expect_page(const uint8_t *page, const char *json, const char *hash)
{
    size_t n = strlen(json);
    char text[80];
    size_t i;

    TEST_ASSERT_EQUAL_MEMORY(json, page, n);
    TEST_ASSERT_EQUAL_UINT8('\n', page[n]);
    memcpy(text, "sha256 ", 7);
    memcpy(text + 7, hash, 64);
    text[71] = '\n';
    TEST_ASSERT_EQUAL_MEMORY(text, page + n + 1u, 72);
    for (i = n + 73u; i < QUEUEOS_BODY_PAGE; i++)
        TEST_ASSERT_EQUAL_UINT8(0, page[i]);
}

static int page_has(const uint8_t *page, const char *text)
{
    size_t n = strlen(text);
    size_t i;

    if (n == 0 || n > QUEUEOS_BODY_PAGE)
        return 0;
    for (i = 0; i + n <= QUEUEOS_BODY_PAGE; i++) {
        if (memcmp(page + i, text, n) == 0)
            return 1;
    }
    return 0;
}

static void stamp_at(char out[21], int n)
{
    int minute = (n / 60) % 60;
    int second = n % 60;

    memcpy(out, "2026-01-01T00:00:00Z", 21);
    out[14] = (char)('0' + minute / 10);
    out[15] = (char)('0' + minute % 10);
    out[17] = (char)('0' + second / 10);
    out[18] = (char)('0' + second % 10);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_zero_page_is_not_a_ring(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 7;

    memset(page, 0, sizeof page);
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_INT(7, slot);
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_read(NULL, line, &slot));
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_read(page, NULL, &slot));
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_write(NULL, STAMP, 1));
}

void test_sample_page_reads_slot_1(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 0;

    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP, 1));
    expect_page(page, SLOT1_JSON, SLOT1_HASH);
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_STRING(SLOT1_LINE, line);
    TEST_ASSERT_EQUAL_INT(1, slot);
    slot = 0;
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, NULL));
    TEST_ASSERT_EQUAL_INT(0, slot);
}

void test_slot_10_round_trip(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 0;

    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP, 10));
    expect_page(page, SLOT10_JSON, SLOT10_HASH);
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_STRING(SLOT10_LINE, line);
    TEST_ASSERT_EQUAL_INT(10, slot);
}

void test_second_write_keeps_the_first_object(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 0;
    const char *both =
        "[{\"stored\":\"2026-11-25T00:00:00Z\",\"slot\":1},"
        "{\"stored\":\"2026-11-25T00:00:01Z\",\"slot\":2}]";

    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP, 1));
    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP_NEXT, 2));
    TEST_ASSERT_EQUAL_MEMORY(both, page, strlen(both));
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_STRING("stored 2026-11-25T00:00:01Z slot 2", line);
    TEST_ASSERT_EQUAL_INT(2, slot);
}

void test_empty_array_grows_one_object(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 9;

    lay_json(page, "[]");
    expect_page(page, "[]", EMPTY_HASH);
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_INT(9, slot);
    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP, 1));
    expect_page(page, SLOT1_JSON, SLOT1_HASH);
}

void test_reordered_object_is_rewritten(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 0;

    lay_json(page, "[{\"slot\":1,\"stored\":\"2026-11-25T00:00:00Z\"}]");
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_STRING(SLOT1_LINE, line);
    TEST_ASSERT_EQUAL_INT(1, slot);
    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP_NEXT, 2));
    TEST_ASSERT_EQUAL_MEMORY(
        "[{\"stored\":\"2026-11-25T00:00:00Z\",\"slot\":1},"
        "{\"stored\":\"2026-11-25T00:00:01Z\",\"slot\":2}]",
        page, 87);
}

void test_refused_pages_are_not_rings(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 4;
    size_t n;

    lay_json(page, SLOT1_JSON);
    page[strlen(SLOT1_JSON) + 8u] = 'c';
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));

    lay_json(page, SLOT1_JSON);
    n = strlen(SLOT1_JSON);
    page[n + 8u] = 'B';
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));

    lay_json(page, SLOT1_JSON);
    page[QUEUEOS_BODY_PAGE - 1u] = 1;
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_INT(4, slot);

    lay_json(page, "[{\"stored\":\"2026-11-25T00:00:00Z\",\"slot\":1,\"x\":1}]");
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    lay_json(page, "[{\"stored\":\"2026-11-25T00:00:00Z\",\"slot\":1.0}]");
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    lay_json(page, "[{\"stored\":\"2026-11-25T00:00:61Z\",\"slot\":1}]");
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    lay_json(page, "[{\"stored\": \"2026-11-25T00:00:00Z\", \"slot\": 1}]");
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));

    lay_json(page, OLD_LINE);
    memcpy(page + strlen(OLD_LINE) + 8u, OLD_HASH, 64);
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP_NEXT, 3));
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_STRING("stored 2026-11-25T00:00:01Z slot 3", line);
    TEST_ASSERT_EQUAL_INT(3, slot);

    memset(page, 0x11, sizeof page);
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_write(page, STAMP, 0));
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_write(page, STAMP, 11));
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_write(page, "2026-13-25T00:00:00Z", 1));
    TEST_ASSERT_EQUAL_UINT8(0x11, page[0]);
}

void test_full_page_drops_the_oldest(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    char first[21];
    char second[21];
    char kept[21];
    char newest[21];
    int slot = 0;
    int n;

    memset(page, 0, sizeof page);
    stamp_at(first, 0);
    stamp_at(second, 1);
    stamp_at(kept, 92);
    for (n = 0; n < 93; n++) {
        char stamp[21];

        stamp_at(stamp, n);
        TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, stamp, 1));
    }
    TEST_ASSERT_TRUE(page_has(page, first));
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_INT(1, slot);
    TEST_ASSERT_EQUAL_MEMORY(kept, line + 7, 20);

    stamp_at(newest, 93);
    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, newest, 1));
    TEST_ASSERT_FALSE(page_has(page, first));
    TEST_ASSERT_TRUE(page_has(page, second));
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_MEMORY(newest, line + 7, 20);
    TEST_ASSERT_EQUAL_INT(1, slot);
}

void test_write_leaves_bytes_after_the_page(void)
{
    uint8_t *buf = calloc(1, QUEUEOS_BODY_PAGE + 16u);
    char line[QUEUEOS_BODY_LINE];
    int slot = 0;
    int i;

    TEST_ASSERT_NOT_NULL(buf);
    memset(buf + QUEUEOS_BODY_PAGE, 0xAB, 16);
    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(buf, STAMP, 1));
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(buf, line, &slot));
    TEST_ASSERT_EQUAL_STRING(SLOT1_LINE, line);
    TEST_ASSERT_EQUAL_INT(1, slot);
    for (i = 0; i < 16; i++)
        TEST_ASSERT_EQUAL_UINT8(0xAB, buf[QUEUEOS_BODY_PAGE + i]);
    free(buf);
}

void test_publish_leaves_the_page(void)
{
    size_t bytes = (size_t)QUEUEOS_BODY_START + QUEUEOS_BODY_PAGE + 1u;
    uint8_t *image = calloc(1, bytes);
    uint8_t saved[QUEUEOS_BODY_PAGE];
    const char payload[] = "body-base";
    QueueosPayload item;
    QueueosGeneration gen;
    int slot = 0;
    uint8_t *page;

    TEST_ASSERT_NOT_NULL(image);
    page = image + QUEUEOS_BODY_START;
    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP, 1));
    memcpy(saved, page, QUEUEOS_BODY_PAGE);
    image[QUEUEOS_BODY_START + QUEUEOS_BODY_PAGE] = 0x5A;

    memset(&item, 0, sizeof item);
    item.role = "base";
    item.arch = "x86-64";
    item.file_label = "Base.efi";
    item.bytes = (const uint8_t *)payload;
    item.length = sizeof payload - 1u;
    memset(&gen, 0, sizeof gen);
    gen.name = "stub";
    gen.version = "0";
    gen.published = STAMP;
    gen.payloads = &item;
    gen.payload_count = 1;
    TEST_ASSERT_EQUAL_INT(QUEUEOS_PUBLISH_OK, queueos_publish(image, bytes, &gen, &slot));
    TEST_ASSERT_EQUAL_INT(1, slot);
    TEST_ASSERT_EQUAL_MEMORY(saved, page, QUEUEOS_BODY_PAGE);
    TEST_ASSERT_EQUAL_UINT8(0x5A, image[QUEUEOS_BODY_START + QUEUEOS_BODY_PAGE]);
    free(image);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_zero_page_is_not_a_ring);
    RUN_TEST(test_sample_page_reads_slot_1);
    RUN_TEST(test_slot_10_round_trip);
    RUN_TEST(test_second_write_keeps_the_first_object);
    RUN_TEST(test_empty_array_grows_one_object);
    RUN_TEST(test_reordered_object_is_rewritten);
    RUN_TEST(test_refused_pages_are_not_rings);
    RUN_TEST(test_full_page_drops_the_oldest);
    RUN_TEST(test_write_leaves_bytes_after_the_page);
    RUN_TEST(test_publish_leaves_the_page);
    return UNITY_END();
}
