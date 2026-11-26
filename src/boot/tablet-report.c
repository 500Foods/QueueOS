/* HID short items for one absolute pointer.
 *
 * QEMU's usb-tablet is the report this boots against: three button bits,
 * a pad, absolute X and Y of 16 bits each in 0..32767, then a wheel.
 * A relative mouse has no absolute X, so it does not match.
 */

#include "tablet-report.h"

#include <string.h>

static int32_t hid_number(const uint8_t *p, int size)
{
    uint32_t raw = 0;
    int i;

    for (i = 0; i < size; i++)
        raw |= (uint32_t)p[i] << (8 * i);
    if (size == 1 && (raw & 0x80u) != 0)
        return (int32_t)raw - 0x100;
    if (size == 2 && (raw & 0x8000u) != 0)
        return (int32_t)raw - 0x10000;
    return (int32_t)raw;
}

static uint32_t bits_at(const uint8_t *report, size_t length,
                        unsigned bit, unsigned nbits)
{
    uint32_t value = 0;
    unsigned i;

    if (nbits > 32)
        nbits = 32;
    for (i = 0; i < nbits; i++) {
        unsigned pos = bit + i;
        unsigned byte = pos / 8u;

        if (byte >= length)
            break;
        if ((report[byte] & (uint8_t)(1u << (pos % 8u))) != 0)
            value |= 1u << i;
    }
    return value;
}

static int32_t field_number(const uint8_t *report, size_t length,
                            unsigned bit, unsigned nbits, int32_t minv)
{
    uint32_t raw = bits_at(report, length, bit, nbits);

    if (minv < 0 && nbits > 0 && nbits < 32
        && (raw & (1u << (nbits - 1u))) != 0)
        return (int32_t)raw - (int32_t)(1u << nbits);
    return (int32_t)raw;
}

typedef struct HidWalk {
    int32_t page;
    int32_t size;
    int32_t count;
    int32_t minv;
    int32_t maxv;
    int32_t usages[16];
    int usage_n;
    int32_t usage_min;
    int32_t usage_max;
    unsigned bit;
    int have_x;
    int have_y;
    QueueosTabletLayout *out;
} HidWalk;

static void clear_locals(HidWalk *walk)
{
    walk->usage_n = 0;
    walk->usage_min = 0;
    walk->usage_max = -1;
}

static int field_usage(const HidWalk *walk, int index, int32_t *usage)
{
    if (walk->usage_n > 0) {
        if (index < walk->usage_n)
            *usage = walk->usages[index];
        else
            *usage = walk->usages[walk->usage_n - 1];
        return 1;
    }
    if (walk->usage_max >= walk->usage_min) {
        int32_t value = walk->usage_min + index;

        if (value > walk->usage_max)
            value = walk->usage_max;
        *usage = value;
        return 1;
    }
    return 0;
}

static void take_input(HidWalk *walk, uint32_t flags)
{
    int constant = (flags & 1u) != 0;
    int relative = (flags & 4u) != 0;
    int count = walk->count;
    int size = walk->size;
    int i;

    if (count < 0)
        count = 0;
    if (count > 64)
        count = 64;
    if (size < 0)
        size = 0;
    if (size > 32)
        size = 32;
    for (i = 0; i < count; i++) {
        int32_t usage = 0;
        int have = field_usage(walk, i, &usage);

        if (!constant && !relative && have && walk->page == 0x01 && usage == 0x30
            && !walk->have_x) {
            walk->out->x_bit = walk->bit;
            walk->out->x_size = (unsigned)size;
            walk->out->x_min = walk->minv;
            walk->out->x_max = walk->maxv;
            walk->have_x = 1;
        }
        if (!constant && !relative && have && walk->page == 0x01 && usage == 0x31
            && !walk->have_y) {
            walk->out->y_bit = walk->bit;
            walk->out->y_size = (unsigned)size;
            walk->out->y_min = walk->minv;
            walk->out->y_max = walk->maxv;
            walk->have_y = 1;
        }
        if (!constant && have && walk->page == 0x09 && usage == 1
            && !walk->out->has_button && size >= 1) {
            walk->out->button_bit = walk->bit;
            walk->out->has_button = 1;
        }
        if (!constant && have && walk->page == 0x01 && usage == 0x38
            && !walk->out->has_wheel && size >= 1) {
            walk->out->wheel_bit = walk->bit;
            walk->out->wheel_size = (unsigned)size;
            walk->out->wheel_min = walk->minv;
            walk->out->has_wheel = 1;
        }
        walk->bit += (unsigned)size;
    }
}

int queueos_tablet_layout(const uint8_t *desc, size_t length,
                          QueueosTabletLayout *out)
{
    HidWalk walk;
    size_t i = 0;

    if (out == 0)
        return 0;
    memset(out, 0, sizeof *out);
    memset(&walk, 0, sizeof walk);
    walk.usage_max = -1;
    walk.out = out;
    if (desc == 0)
        return 0;
    while (i < length && walk.bit < 4096u) {
        uint8_t prefix = desc[i++];
        int size_code = prefix & 3;
        int size = size_code == 3 ? 4 : size_code;
        int type = (prefix >> 2) & 3;
        int tag = (prefix >> 4) & 15;
        int32_t value = 0;

        if (prefix == 0xFE) {
            if (i + 2 > length)
                break;
            i += 2u + desc[i];
            continue;
        }
        if (i + (size_t)size > length)
            break;
        if (size > 0)
            value = hid_number(desc + i, size);
        i += (size_t)size;
        if (type == 1 && tag == 0)
            walk.page = value;
        else if (type == 1 && tag == 1)
            walk.minv = value;
        else if (type == 1 && tag == 2)
            walk.maxv = value;
        else if (type == 1 && tag == 7)
            walk.size = value;
        else if (type == 1 && tag == 8 && walk.bit < 8u)
            walk.bit = 8;
        else if (type == 1 && tag == 9)
            walk.count = value;
        else if (type == 2 && tag == 0 && walk.usage_n < 16)
            walk.usages[walk.usage_n++] = value;
        else if (type == 2 && tag == 1)
            walk.usage_min = value;
        else if (type == 2 && tag == 2)
            walk.usage_max = value;
        else if (type == 0) {
            /* Collection is a main item. It clears the locals too. */
            if (tag == 8)
                take_input(&walk, (uint32_t)value);
            clear_locals(&walk);
        }
    }
    out->ok = walk.have_x && walk.have_y
        && out->x_size > 0 && out->y_size > 0
        && out->x_max >= out->x_min && out->y_max >= out->y_min;
    return out->ok;
}

int queueos_tablet_read(const QueueosTabletLayout *layout,
                        const uint8_t *report, size_t length,
                        int32_t *x, int32_t *y, int *button, int *wheel)
{
    unsigned need;

    if (layout == 0 || !layout->ok || report == 0 || x == 0 || y == 0 || button == 0)
        return 0;
    need = layout->x_bit + layout->x_size;
    if (layout->y_bit + layout->y_size > need)
        need = layout->y_bit + layout->y_size;
    if (layout->has_button && layout->button_bit + 1u > need)
        need = layout->button_bit + 1u;
    if (layout->has_wheel && layout->wheel_bit + layout->wheel_size > need)
        need = layout->wheel_bit + layout->wheel_size;
    if (length < (need + 7u) / 8u)
        return 0;
    *x = field_number(report, length, layout->x_bit, layout->x_size, layout->x_min);
    *y = field_number(report, length, layout->y_bit, layout->y_size, layout->y_min);
    *button = 0;
    if (layout->has_button)
        *button = bits_at(report, length, layout->button_bit, 1) != 0;
    if (wheel != 0)
        *wheel = 0;
    if (wheel != 0 && layout->has_wheel)
        *wheel = field_number(report, length, layout->wheel_bit,
                              layout->wheel_size, layout->wheel_min);
    return 1;
}

static uint64_t mul_div(uint64_t pos, uint64_t cells, uint64_t denom)
{
    uint64_t whole;
    uint64_t rem;

    if (denom == 0 || cells == 0)
        return 0;
    whole = pos / denom;
    rem = pos % denom;
    if (whole > UINT64_MAX / cells)
        return UINT64_MAX;
    if (rem > UINT64_MAX / cells)
        return whole * cells + (cells - 1u);
    return whole * cells + (rem * cells) / denom;
}

int queueos_axis_cell(uint64_t value, uint64_t minv, uint64_t maxv, int cells)
{
    uint64_t raw;

    if (cells < 1)
        return 1;
    if (value <= minv || maxv <= minv)
        return 1;
    if (value >= maxv)
        return cells;
    if (maxv - minv == UINT64_MAX)
        raw = mul_div(value - minv, (uint64_t)cells, maxv - minv);
    else
        raw = mul_div(value - minv, (uint64_t)cells, (maxv - minv) + 1u);
    if (raw >= (uint64_t)cells)
        raw = (uint64_t)cells - 1u;
    return (int)raw + 1;
}
