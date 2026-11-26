/* QueueOS-Boot.
 *
 * Base is read from the virtio-scsi disk whose product is Base. There is
 * no FAT on that disk and no fs0: guess. The ESP path is gone.
 *
 * The loading sentence is written first. The embedded Lua list then draws
 * the slots and waits about ten seconds before StartImage. The harness
 * matches that sentence and does not have to sit through the list.
 *
 * Load options are the ASCII text "slot N" and a trailing NUL. The size
 * counts that NUL. Base reads them when it writes the completion line.
 * They are not CHAR16.
 */

#include "base-disk.h"
#include "diagnostics.h"
#include "disk.h"
#include "pool.h"
#include "screen-run.h"
#include "serial.h"
#include "text.h"
#include "types.h"

#include <stdlib.h>
#include <string.h>

/* Loaded Image Protocol. The child image receives its slot name here. */
static const uint8_t QUEUEOS_LOADED_IMAGE_GUID[16] = {
    0xA1, 0x31, 0x1B, 0x5B, 0x62, 0x95, 0xD2, 0x11,
    0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B
};

static char *slot_text(int slot)
{
    char *text;
    int n = 0;

    if (slot < 1 || slot > QUEUEOS_SLOT_COUNT)
        return 0;
    text = malloc(16);
    if (text == 0)
        return 0;
    text[n++] = 's';
    text[n++] = 'l';
    text[n++] = 'o';
    text[n++] = 't';
    text[n++] = ' ';
    if (slot >= 10) {
        text[n++] = '1';
        text[n++] = '0';
    } else {
        text[n++] = (char)('0' + slot);
    }
    text[n] = '\0';
    return text;
}

static void release_slots(QueueosGuestSlot *slots, int count)
{
    int i;

    for (i = 0; i < count; i++)
        queueos_guest_release(&slots[i]);
}

static QUEUEOS_EFI_STATUS queueos_start_slot(
    QUEUEOS_EFI_SYSTEM_TABLE *system,
    void *image,
    const QueueosGuestSlot *slot,
    int slot_index)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    void *buffer = 0;
    QUEUEOS_UINTN size = 0;
    void *child;
    char *options;
    QUEUEOS_EFI_LOADED_IMAGE *loaded;
    QUEUEOS_EFI_STATUS status;

    status = queueos_read_slot_image(system, slot, &buffer, &size);
    if (status != QUEUEOS_EFI_SUCCESS)
        return status;
    boot = system->boot_services;
    if (boot == 0 || boot->load_image == 0 || boot->start_image == 0
        || boot->handle_protocol == 0) {
        free(buffer);
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    child = 0;
    status = boot->load_image(QUEUEOS_FALSE, image, 0, buffer, size, &child);
    free(buffer);
    if (status != QUEUEOS_EFI_SUCCESS || child == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    options = slot_text(slot_index);
    if (options == 0)
        return QUEUEOS_EFI_LOAD_ERROR;
    loaded = 0;
    status = boot->handle_protocol(child, QUEUEOS_LOADED_IMAGE_GUID, (void **)&loaded);
    if (status != QUEUEOS_EFI_SUCCESS || loaded == 0) {
        free(options);
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    loaded->load_options = options;
    loaded->load_options_size = (uint32_t)strlen(options) + 1u;
    status = boot->start_image(child, 0, 0);
    free(options);
    return status;
}

static void boot_failed(void)
{
    queueos_serial_init();
    queueos_serial_write_line(QUEUEOS_BOOT_FAIL_SENTENCE);
}

QUEUEOS_EFI_STATUS QUEUEOS_EFIAPI efi_main(void *image, QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    QueueosGuestSlot slots[QUEUEOS_SLOT_COUNT];
    QUEUEOS_EFI_STATUS status;
    int chosen;

    queueos_pool_bind(system);
    queueos_report(system, "QueueOS-Boot: entry", QUEUEOS_BOOT_SENTENCE);
    status = queueos_read_database_slots(system, slots, QUEUEOS_SLOT_COUNT);
    if (status != QUEUEOS_EFI_SUCCESS) {
        boot_failed();
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    chosen = queueos_screen_run(system, slots, QUEUEOS_SLOT_COUNT);
    status = QUEUEOS_EFI_LOAD_ERROR;
    if (chosen >= 1 && chosen <= QUEUEOS_SLOT_COUNT)
        status = queueos_start_slot(system, image, &slots[chosen - 1], chosen);
    release_slots(slots, QUEUEOS_SLOT_COUNT);
    if (status != QUEUEOS_EFI_SUCCESS) {
        boot_failed();
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    queueos_serial_init();
    queueos_serial_write_line("QueueOS-Boot: base started");
    /* Last ConOut change before firmware runs. A failed start leaves
     * the list on screen and writes the failure sentence to the serial. */
    queueos_screen_attribute(system, QUEUEOS_TEXT_RED_ON_RED);
    return QUEUEOS_EFI_SUCCESS;
}
