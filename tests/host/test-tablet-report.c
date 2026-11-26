/* The QEMU usb-tablet report, and a relative mouse that must not match. */

#include "tablet-report.h"

#include "unity.h"

/* QEMU 10.2 usb-tablet. Five buttons, then absolute X and Y. */
static const uint8_t TABLET[] = {
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x05, 0x15, 0x00, 0x25, 0x01,
    0x95, 0x05, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x03,
    0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x00,
    0x26, 0xFF, 0x7F, 0x35, 0x00, 0x46, 0xFF, 0x7F, 0x75, 0x10,
    0x95, 0x02, 0x81, 0x02, 0x05, 0x01, 0x09, 0x38, 0x15, 0x81,
    0x25, 0x7F, 0x35, 0x00, 0x45, 0x00, 0x75, 0x08, 0x95, 0x01,
    0x81, 0x06, 0xC0, 0xC0
};

static const uint8_t RELATIVE[] = {
    0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x09, 0x01, 0xA1, 0x00,
    0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01,
    0x95, 0x03, 0x75, 0x01, 0x81, 0x02, 0x95, 0x01, 0x75, 0x05,
    0x81, 0x01, 0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x15, 0x81,
    0x25, 0x7F, 0x75, 0x08, 0x95, 0x02, 0x81, 0x06, 0xC0, 0xC0
};

void test_qemu_tablet_layout_and_sample(void)
{
    QueueosTabletLayout layout;
    int32_t x = -1;
    int32_t y = -1;
    int button = 0;
    int wheel = 99;
    uint8_t report[6] = { 0x01, 0x00, 0x00, 0xFF, 0x7F, 0x00 };

    TEST_ASSERT_EQUAL_INT(74, (int)sizeof TABLET);
    TEST_ASSERT_TRUE(queueos_tablet_layout(TABLET, sizeof TABLET, &layout));
    TEST_ASSERT_EQUAL_UINT(8, layout.x_bit);
    TEST_ASSERT_EQUAL_UINT(16, layout.x_size);
    TEST_ASSERT_EQUAL_INT(0, layout.x_min);
    TEST_ASSERT_EQUAL_INT(32767, layout.x_max);
    TEST_ASSERT_EQUAL_UINT(24, layout.y_bit);
    TEST_ASSERT_EQUAL_UINT(16, layout.y_size);
    TEST_ASSERT_TRUE(layout.has_button);
    TEST_ASSERT_EQUAL_UINT(0, layout.button_bit);
    TEST_ASSERT_TRUE(layout.has_wheel);
    TEST_ASSERT_EQUAL_UINT(40, layout.wheel_bit);
    TEST_ASSERT_EQUAL_UINT(8, layout.wheel_size);
    TEST_ASSERT_EQUAL_INT(-127, layout.wheel_min);
    TEST_ASSERT_TRUE(queueos_tablet_read(&layout, report, sizeof report,
                                         &x, &y, &button, &wheel));
    TEST_ASSERT_EQUAL_INT(0, x);
    TEST_ASSERT_EQUAL_INT(32767, y);
    TEST_ASSERT_EQUAL_INT(1, button);
    TEST_ASSERT_EQUAL_INT(0, wheel);
    report[5] = 0xFF;
    TEST_ASSERT_TRUE(queueos_tablet_read(&layout, report, sizeof report,
                                         &x, &y, &button, &wheel));
    TEST_ASSERT_EQUAL_INT(-1, wheel);
    report[5] = 0x01;
    TEST_ASSERT_TRUE(queueos_tablet_read(&layout, report, sizeof report,
                                         &x, &y, &button, &wheel));
    TEST_ASSERT_EQUAL_INT(1, wheel);
    TEST_ASSERT_EQUAL_INT(1, queueos_axis_cell(0, 0, 32767, 80));
    TEST_ASSERT_EQUAL_INT(80, queueos_axis_cell(32767, 0, 32767, 80));
    TEST_ASSERT_EQUAL_INT(41, queueos_axis_cell(16384, 0, 32767, 80));
}

void test_relative_mouse_is_not_a_tablet(void)
{
    QueueosTabletLayout layout;

    TEST_ASSERT_FALSE(queueos_tablet_layout(RELATIVE, sizeof RELATIVE, &layout));
    TEST_ASSERT_FALSE(queueos_tablet_layout(0, 0, &layout));
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
    RUN_TEST(test_qemu_tablet_layout_and_sample);
    RUN_TEST(test_relative_mouse_is_not_a_tablet);
    return UNITY_END();
}
