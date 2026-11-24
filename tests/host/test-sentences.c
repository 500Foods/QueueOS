/* Host checks for the three sentences in src/common/diagnostics.h.
 * Plan 001 names them C-001-001, C-001-002, and C-001-003.
 * The EFI programs include the same header and are not linked here.
 */

#include "diagnostics.h"

#include "unity.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_C_001_001_boot_sentence(void)
{
    TEST_ASSERT_EQUAL_STRING("QueueOS loading...", QUEUEOS_BOOT_SENTENCE);
}

void test_C_001_002_base_sentence(void)
{
    TEST_ASSERT_EQUAL_STRING("QueueOS Loaded", QUEUEOS_BASE_SENTENCE);
}

void test_C_001_003_boot_failure_sentence(void)
{
    TEST_ASSERT_EQUAL_STRING("QueueOS loading... failed to start Base",
        QUEUEOS_BOOT_FAIL_SENTENCE);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_C_001_001_boot_sentence);
    RUN_TEST(test_C_001_002_base_sentence);
    RUN_TEST(test_C_001_003_boot_failure_sentence);
    return UNITY_END();
}
