/* Jansson classification for one database slot. The image is a buffer.
 * Nothing here opens images/.
 */

#include "disk.h"
#include "sha256.h"
#include "slot-json.h"

#include "unity.h"

#include <string.h>

static const char EMPTY_HASH[] =
    "44136fa355b3678a1146ad16f7e8649e94fb4fc21fe77e8310c060f61caaff8a";

static const char EMPTY_DIGEST[] =
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";

static void write_slot(uint8_t raw[QUEUEOS_SLOT_BYTES], const char *json, int good)
{
    char hex[65];
    uint8_t digest[32];
    size_t n = strlen(json);

    memset(raw, 0, QUEUEOS_SLOT_BYTES);
    memcpy(raw, json, n);
    raw[n] = '\n';
    queueos_sha256((const uint8_t *)json, n, digest);
    queueos_sha256_hex(digest, hex);
    if (!good)
        hex[0] = (char)(hex[0] == '0' ? '1' : '0');
    memcpy(raw + n + 1, "sha256 ", 7);
    memcpy(raw + n + 8, hex, 64);
    raw[n + 72] = '\n';
}

static const char *stub_json(void)
{
    return "{\"OS\":\"QueueOS\",\"name\":\"stub\",\"version\":\"1.0.1\","
           "\"published\":\"2026-11-25T00:00:00Z\",\"countdown\":10,"
           "\"binaries\":[{\"role\":\"base\",\"arch\":\"x86-64\","
           "\"offset\":40960,\"length\":4,"
           "\"sha256\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\","
           "\"file\":\"Base.efi\"}]}";
}

void test_empty_object_is_empty(void)
{
    uint8_t raw[QUEUEOS_SLOT_BYTES];
    QueueosGuestSlot slot;

    write_slot(raw, "{}", 1);
    queueos_guest_read_slot(raw, &slot);
    TEST_ASSERT_EQUAL_INT(QUEUEOS_SLOT_EMPTY, slot.kind);
    TEST_ASSERT_EQUAL_INT(0, slot.countdown);
    TEST_ASSERT_FALSE(slot.launchable);
}

void test_zeros_are_unreadable(void)
{
    uint8_t raw[QUEUEOS_SLOT_BYTES];
    QueueosGuestSlot slot;

    memset(raw, 0, sizeof raw);
    queueos_guest_read_slot(raw, &slot);
    TEST_ASSERT_EQUAL_INT(QUEUEOS_SLOT_UNREADABLE, slot.kind);
}

void test_stub_is_launchable(void)
{
    uint8_t raw[QUEUEOS_SLOT_BYTES];
    QueueosGuestSlot slot;

    write_slot(raw, stub_json(), 1);
    queueos_guest_read_slot(raw, &slot);
    TEST_ASSERT_EQUAL_INT(QUEUEOS_SLOT_CANDIDATE, slot.kind);
    TEST_ASSERT_TRUE(slot.launchable);
    TEST_ASSERT_EQUAL_STRING("stub", slot.name);
    TEST_ASSERT_EQUAL_STRING("1.0.1", slot.version);
    TEST_ASSERT_EQUAL_INT(10, slot.countdown);
    TEST_ASSERT_EQUAL_STRING("2026-11-25T00:00:00Z", slot.published);
    TEST_ASSERT_EQUAL_STRING("", slot.booted);
    TEST_ASSERT_EQUAL_STRING("", slot.tag);
    TEST_ASSERT_EQUAL_UINT64(40960, slot.offset);
    TEST_ASSERT_EQUAL_UINT64(4, slot.length);
}

void test_bad_hash_is_unsafe(void)
{
    uint8_t raw[QUEUEOS_SLOT_BYTES];
    QueueosGuestSlot slot;

    write_slot(raw, stub_json(), 0);
    queueos_guest_read_slot(raw, &slot);
    TEST_ASSERT_EQUAL_INT(QUEUEOS_SLOT_UNSAFE, slot.kind);
    TEST_ASSERT_FALSE(slot.launchable);
}

void test_other_os_is_foreign(void)
{
    uint8_t raw[QUEUEOS_SLOT_BYTES];
    QueueosGuestSlot slot;

    write_slot(raw, "{\"OS\":\"Other\",\"published\":\"2020-01-01T00:00:00Z\"}", 1);
    queueos_guest_read_slot(raw, &slot);
    TEST_ASSERT_EQUAL_INT(QUEUEOS_SLOT_FOREIGN, slot.kind);
    TEST_ASSERT_FALSE(slot.launchable);
}

void test_newest_published_wins_and_ties_keep_the_lower_slot(void)
{
    QueueosGuestSlot slots[3];
    uint8_t raw[QUEUEOS_SLOT_BYTES];

    memset(slots, 0, sizeof slots);
    write_slot(raw,
               "{\"OS\":\"QueueOS\",\"published\":\"2026-11-24T00:00:00Z\","
               "\"binaries\":[{\"role\":\"base\",\"arch\":\"x86-64\","
               "\"offset\":40960,\"length\":1,"
               "\"sha256\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"}]}",
               1);
    queueos_guest_read_slot(raw, &slots[0]);
    write_slot(raw,
               "{\"OS\":\"QueueOS\",\"published\":\"2026-11-25T00:00:00Z\","
               "\"binaries\":[{\"role\":\"base\",\"arch\":\"x86-64\","
               "\"offset\":40960,\"length\":1,"
               "\"sha256\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"}]}",
               1);
    queueos_guest_read_slot(raw, &slots[1]);
    write_slot(raw,
               "{\"OS\":\"QueueOS\",\"published\":\"2026-11-25T00:00:00Z\","
               "\"binaries\":[{\"role\":\"base\",\"arch\":\"x86-64\","
               "\"offset\":40960,\"length\":1,"
               "\"sha256\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\"}]}",
               1);
    queueos_guest_read_slot(raw, &slots[2]);
    TEST_ASSERT_EQUAL_INT(2, queueos_guest_choose(slots, 3));
    memcpy(slots[0].published, slots[1].published, sizeof slots[0].published);
    slots[0].launchable = 1;
    TEST_ASSERT_EQUAL_INT(1, queueos_guest_choose(slots, 2));
}

void test_booted_line_and_tag_are_copied(void)
{
    uint8_t raw[QUEUEOS_SLOT_BYTES];
    QueueosGuestSlot slot;
    const char *json =
        "{\"OS\":\"QueueOS\",\"name\":\"stub\",\"version\":\"0\","
        "\"published\":\"2026-11-25T00:00:00Z\",\"tag\":\"lab\"}";
    const char *line = "booted 2026-11-25T15:38:39Z\n";
    size_t n;

    write_slot(raw, json, 1);
    n = strlen((const char *)raw);
    memcpy(raw + n, line, strlen(line));
    queueos_guest_read_slot(raw, &slot);
    TEST_ASSERT_EQUAL_STRING("lab", slot.tag);
    TEST_ASSERT_EQUAL_STRING("2026-11-25T15:38:39Z", slot.booted);
    TEST_ASSERT_EQUAL_INT(QUEUEOS_SLOT_CANDIDATE, slot.kind);
    queueos_guest_release(&slot);

    write_slot(raw, json, 1);
    memcpy(raw + strlen((const char *)raw), "not a boot line\n", 16);
    queueos_guest_read_slot(raw, &slot);
    TEST_ASSERT_EQUAL_STRING("", slot.booted);
    queueos_guest_release(&slot);
}

void test_boot_range_and_digest(void)
{
    uint8_t bytes[3] = { 'a', 'b', 'c' };

    TEST_ASSERT_TRUE(queueos_guest_range_ok(QUEUEOS_SLOT_REGION, 1));
    TEST_ASSERT_FALSE(queueos_guest_range_ok(0, 1));
    TEST_ASSERT_FALSE(queueos_guest_range_ok(QUEUEOS_BODY_START, 1));
    TEST_ASSERT_TRUE(queueos_guest_range_ok(QUEUEOS_BODY_START - 1u, 1));
    TEST_ASSERT_FALSE(queueos_guest_range_ok(QUEUEOS_BODY_START - 1u, 2));
    TEST_ASSERT_TRUE(queueos_guest_digest_ok((const uint8_t *)"", 0, EMPTY_DIGEST));
    TEST_ASSERT_FALSE(queueos_guest_digest_ok(bytes, 3, EMPTY_HASH));
    (void)bytes;
}

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_empty_object_is_empty);
    RUN_TEST(test_zeros_are_unreadable);
    RUN_TEST(test_stub_is_launchable);
    RUN_TEST(test_bad_hash_is_unsafe);
    RUN_TEST(test_other_os_is_foreign);
    RUN_TEST(test_newest_published_wins_and_ties_keep_the_lower_slot);
    RUN_TEST(test_booted_line_and_tag_are_copied);
    RUN_TEST(test_boot_range_and_digest);
    return UNITY_END();
}
