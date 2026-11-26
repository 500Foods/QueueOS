/* Writes the completion line for the slot Boot named, then the body page.
 *
 * Load options are ASCII "slot N" and a trailing NUL. The size counts
 * that NUL. Anything else means this Base was not started that way, and
 * the disk is left alone.
 *
 * The stamp is one reading of the firmware clock, formatted
 * YYYY-MM-DDTHH:MM:SSZ. The Z is the stand-in from the plan. GetTime is
 * not asked to convert a timezone. A failed GetTime, or a clock field
 * that will not fit that text, writes nothing. The stamp is not invented.
 *
 * An existing booted line stays the first stamp. The page is still
 * written with this boot's stamp. When the page already holds a record,
 * that stored line is printed before the new page replaces it. A zero
 * or rejected page prints nothing.
 */

#include "completion.h"

#include "body-page.h"
#include "disk-io.h"
#include "disk.h"
#include "publish.h"
#include "serial.h"

#include <stddef.h>
#include <string.h>

/* EFI_LOADED_IMAGE_PROTOCOL_GUID 5B1B31A1-9562-11d2-8E3F-00A0C969723B */
static const uint8_t LOADED_IMAGE_GUID[16] = {
    0xA1, 0x31, 0x1B, 0x5B, 0x62, 0x95, 0xD2, 0x11,
    0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B
};

/* "RUNTSERV" */
#define QUEUEOS_EFI_RUNTIME_SIGNATURE 0x56524553544e5552ULL

typedef struct QueueosEfiTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t pad1;
    uint32_t nanosecond;
    int16_t timezone;
    uint8_t daylight;
    uint8_t pad2;
} QueueosEfiTime;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QueueosEfiGetTime)(
    QueueosEfiTime *time,
    void *capabilities);

typedef struct QueueosEfiRuntime {
    QUEUEOS_EFI_TABLE_HEADER header;
    QueueosEfiGetTime get_time;
} QueueosEfiRuntime;

_Static_assert(sizeof(QueueosEfiTime) == 16, "EFI_TIME size");
_Static_assert(offsetof(QueueosEfiTime, nanosecond) == 8, "nanosecond");
_Static_assert(offsetof(QueueosEfiTime, timezone) == 12, "timezone");

static void put_two(char *out, unsigned value)
{
    out[0] = (char)('0' + (value / 10u) % 10u);
    out[1] = (char)('0' + value % 10u);
}

static int format_stamp(const QueueosEfiTime *time, char out[21])
{
    unsigned year;

    if (time->year < 1 || time->year > 9999)
        return 0;
    if (time->month < 1 || time->month > 12)
        return 0;
    if (time->day < 1 || time->day > 31)
        return 0;
    if (time->hour > 23 || time->minute > 59 || time->second > 60)
        return 0;
    year = time->year;
    out[0] = (char)('0' + year / 1000u);
    out[1] = (char)('0' + (year / 100u) % 10u);
    out[2] = (char)('0' + (year / 10u) % 10u);
    out[3] = (char)('0' + year % 10u);
    out[4] = '-';
    put_two(out + 5, time->month);
    out[7] = '-';
    put_two(out + 8, time->day);
    out[10] = 'T';
    put_two(out + 11, time->hour);
    out[13] = ':';
    put_two(out + 14, time->minute);
    out[16] = ':';
    put_two(out + 17, time->second);
    out[19] = 'Z';
    out[20] = '\0';
    return 1;
}

static int firmware_stamp(QUEUEOS_EFI_SYSTEM_TABLE *system, char out[21])
{
    QueueosEfiRuntime *runtime;
    QueueosEfiTime time;

    if (system == 0 || system->runtime_services == 0)
        return 0;
    runtime = system->runtime_services;
    if (runtime->header.signature != QUEUEOS_EFI_RUNTIME_SIGNATURE)
        return 0;
    if (runtime->get_time == 0)
        return 0;
    memset(&time, 0, sizeof time);
    if (runtime->get_time(&time, 0) != QUEUEOS_EFI_SUCCESS)
        return 0;
    return format_stamp(&time, out);
}

/* 1..10, or 0 when the load options are not "slot N". */
static int slot_from_options(QUEUEOS_EFI_SYSTEM_TABLE *system, void *image)
{
    QUEUEOS_EFI_LOADED_IMAGE *loaded;
    const uint8_t *text;
    uint32_t size;
    QUEUEOS_EFI_STATUS status;

    if (image == 0 || system == 0 || system->boot_services == 0)
        return 0;
    if (system->boot_services->handle_protocol == 0)
        return 0;
    loaded = 0;
    status = system->boot_services->handle_protocol(
        image, LOADED_IMAGE_GUID, (void **)&loaded);
    if (status != QUEUEOS_EFI_SUCCESS || loaded == 0 || loaded->load_options == 0)
        return 0;
    size = loaded->load_options_size;
    text = loaded->load_options;
    if (size == 7
        && text[0] == 's' && text[1] == 'l' && text[2] == 'o' && text[3] == 't'
        && text[4] == ' ' && text[5] >= '1' && text[5] <= '9' && text[6] == 0)
        return text[5] - '0';
    if (size == 8
        && text[0] == 's' && text[1] == 'l' && text[2] == 'o' && text[3] == 't'
        && text[4] == ' ' && text[5] == '1' && text[6] == '0' && text[7] == 0)
        return 10;
    return 0;
}

/* The slot write has already been accepted. A failed page read leaves
 * the page alone. A record is printed from the bytes just read, and
 * then the buffer is replaced with this boot's page.
 */
static void store_body(QueueosEfiBlockIo *block, const char *stamp, int slot)
{
    uint8_t page[QUEUEOS_BODY_PAGE];
    char line[QUEUEOS_BODY_LINE];

    memset(page, 0, sizeof page);
    if (!queueos_disk_read(block, QUEUEOS_BODY_START, QUEUEOS_BODY_PAGE, page))
        return;
    if (queueos_body_read(page, line, 0) == 1)
        queueos_serial_write_line(line);
    if (queueos_body_write(page, stamp, slot) != 0)
        return;
    queueos_disk_write(block, QUEUEOS_BODY_START, QUEUEOS_BODY_PAGE, page);
}

void queueos_base_complete(QUEUEOS_EFI_SYSTEM_TABLE *system, void *image)
{
    uint8_t slot_bytes[QUEUEOS_SLOT_BYTES];
    uint8_t saved[QUEUEOS_SLOT_BYTES];
    char stamp[21];
    QueueosEfiBlockIo *block;
    int slot;
    uint64_t offset;

    slot = slot_from_options(system, image);
    if (slot < 1 || slot > QUEUEOS_SLOT_COUNT)
        return;
    if (!firmware_stamp(system, stamp))
        return;
    if (system == 0 || system->boot_services == 0)
        return;
    block = queueos_disk_find_base(system->boot_services);
    if (block == 0)
        return;
    offset = (uint64_t)(slot - 1) * QUEUEOS_SLOT_BYTES;
    if (!queueos_disk_read(block, offset, QUEUEOS_SLOT_BYTES, slot_bytes))
        return;
    memcpy(saved, slot_bytes, QUEUEOS_SLOT_BYTES);
    if (queueos_slot_mark_booted(slot_bytes, stamp) != 0)
        return;
    if (memcmp(saved, slot_bytes, QUEUEOS_SLOT_BYTES) != 0) {
        if (!queueos_disk_write(block, offset, QUEUEOS_SLOT_BYTES, slot_bytes))
            return;
    }
    store_body(block, stamp, slot);
}
