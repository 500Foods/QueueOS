/* Host checks for the one body page. The image under test is a memory
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
static const char SLOT1_LINE[] = "stored 2026-11-25T00:00:00Z slot 1";
static const char SLOT1_HASH[] =
    "b5584f1334084037dd6be0610f569914deac15d8651c367ec5b7f98e5edc59bf";
static const char SLOT10_LINE[] = "stored 2026-11-25T00:00:00Z slot 10";
static const char SLOT10_HASH[] =
    "6217a9917cf2f265febf9e1001ea9ac6ea04095701a603be149e996375bf0c12";

static void lay_line(uint8_t page[QUEUEOS_BODY_PAGE], const char *line)
{
    uint8_t digest[32];
    char hex[65];
    size_t n = strlen(line);

    memset(page, 0, QUEUEOS_BODY_PAGE);
    TEST_ASSERT_TRUE(n + 72u < QUEUEOS_BODY_PAGE);
    memcpy(page, line, n);
    page[n] = '\n';
    queueos_sha256((const uint8_t *)line, n, digest);
    queueos_sha256_hex(digest, hex);
    memcpy(page + n + 1u, "sha256 ", 7);
    memcpy(page + n + 8u, hex, 64);
    page[n + 72u] = '\n';
}

static void expect_hash_line(const uint8_t *page, const char *line, const char *hash)
{
    size_t n = strlen(line);
    char text[80];

    TEST_ASSERT_EQUAL_MEMORY(line, page, n);
    TEST_ASSERT_EQUAL_UINT8('\n', page[n]);
    memcpy(text, "sha256 ", 7);
    memcpy(text + 7, hash, 64);
    text[71] = '\n';
    TEST_ASSERT_EQUAL_MEMORY(text, page + n + 1u, 72);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void test_zero_page_is_not_a_record(void)
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
    expect_hash_line(page, SLOT1_LINE, SLOT1_HASH);
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
    expect_hash_line(page, SLOT10_LINE, SLOT10_HASH);
    TEST_ASSERT_EQUAL_INT(1, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_STRING(SLOT10_LINE, line);
    TEST_ASSERT_EQUAL_INT(10, slot);
}

void test_refused_pages_are_not_records(void)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];
    int slot = 4;
    size_t n;

    lay_line(page, SLOT1_LINE);
    page[strlen(SLOT1_LINE) + 8u] = 'c';
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));

    lay_line(page, "stored 2026-11-25T00:00:61Z slot 1");
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));

    lay_line(page, SLOT1_LINE);
    n = strlen(SLOT1_LINE);
    page[n + 8u] = 'B';
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));

    lay_line(page, "stored 2026-11-25T00:00:00Z slot 01");
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    lay_line(page, "stored 2026-11-25T00:00:00Z slot 0");
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    lay_line(page, "stored 2026-11-25T00:00:00Z slot 11");
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));

    TEST_ASSERT_EQUAL_INT(0, queueos_body_write(page, STAMP, 1));
    page[QUEUEOS_BODY_PAGE - 1u] = 1;
    TEST_ASSERT_EQUAL_INT(0, queueos_body_read(page, line, &slot));
    TEST_ASSERT_EQUAL_INT(4, slot);

    memset(page, 0x11, sizeof page);
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_write(page, STAMP, 0));
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_write(page, STAMP, 11));
    TEST_ASSERT_EQUAL_INT(-1, queueos_body_write(page, "2026-13-25T00:00:00Z", 1));
    TEST_ASSERT_EQUAL_UINT8(0x11, page[0]);
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
    RUN_TEST(test_zero_page_is_not_a_record);
    RUN_TEST(test_sample_page_reads_slot_1);
    RUN_TEST(test_slot_10_round_trip);
    RUN_TEST(test_refused_pages_are_not_records);
    RUN_TEST(test_write_leaves_bytes_after_the_page);
    RUN_TEST(test_publish_leaves_the_page);
    return UNITY_END();
}
