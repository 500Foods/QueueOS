/* Absolute pointer report. The bytes are a HID report descriptor and one
 * report. This file does not talk to firmware.
 */
#ifndef QUEUEOS_TABLET_REPORT_H
#define QUEUEOS_TABLET_REPORT_H

#include <stddef.h>
#include <stdint.h>

typedef struct QueueosTabletLayout {
    int ok;
    unsigned x_bit;
    unsigned x_size;
    int32_t x_min;
    int32_t x_max;
    unsigned y_bit;
    unsigned y_size;
    int32_t y_min;
    int32_t y_max;
    unsigned button_bit;
    int has_button;
    unsigned wheel_bit;
    unsigned wheel_size;
    int32_t wheel_min;
    int has_wheel;
} QueueosTabletLayout;

/* 1 when desc has absolute X and absolute Y. */
int queueos_tablet_layout(const uint8_t *desc, size_t length,
                          QueueosTabletLayout *out);

/* 1 when report covers those fields. *button is the first button.
 * *wheel is the signed wheel when the report has one, else 0.
 * QEMU's tablet stores wheel-up as a negative byte.
 */
int queueos_tablet_read(const QueueosTabletLayout *layout,
                        const uint8_t *report, size_t length,
                        int32_t *x, int32_t *y, int *button, int *wheel);

/* Cell 1 is the low end of the axis. Cell `cells` is the high end. */
int queueos_axis_cell(uint64_t value, uint64_t minv, uint64_t maxv, int cells);

#endif
