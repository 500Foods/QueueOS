/* QueueOS-Boot stub.
 *
 * Scaffold: Base is loaded from \EFI\QueueOS\current\Base.efi on the same
 * device this image was loaded from. The database disk is not read. A later
 * plan replaces this with a fetch from that disk. There is no fs0: guess.
 *
 * The loading sentence is written first, then a 10-second countdown stays
 * on screen before StartImage. The harness matches that sentence and does
 * not wait out the countdown. A person starting the guest can see Boot
 * before Base clears the screen.
 */

#include "diagnostics.h"
#include "serial.h"
#include "text.h"
#include "types.h"

enum { QUEUEOS_PATH_MAX = 64 };

static const char QUEUEOS_BASE_PATH[] = "\\EFI\\QueueOS\\current\\Base.efi";

/* Loaded Image Protocol. Identifies the device Boot itself was loaded from. */
static const uint8_t QUEUEOS_LOADED_IMAGE_GUID[16] = {
    0xA1, 0x31, 0x1B, 0x5B, 0x62, 0x95, 0xD2, 0x11,
    0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B
};

/* Simple File System Protocol. The database disk has no FAT, so it will not answer. */
static const uint8_t QUEUEOS_SIMPLE_FILE_SYSTEM_GUID[16] = {
    0x22, 0x5B, 0x4E, 0x96, 0x59, 0x64, 0xD2, 0x11,
    0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B
};

static QUEUEOS_EFI_STATUS queueos_read_base(
    QUEUEOS_EFI_SYSTEM_TABLE *system,
    void *image,
    void **buffer,
    QUEUEOS_UINTN *size)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    QUEUEOS_EFI_LOADED_IMAGE *loaded;
    QUEUEOS_EFI_SIMPLE_FILE_SYSTEM *volume;
    QUEUEOS_EFI_FILE_PROTOCOL *root;
    QUEUEOS_EFI_FILE_PROTOCOL *file;
    QUEUEOS_CHAR16 path[QUEUEOS_PATH_MAX];
    QUEUEOS_UINTN i;
    uint64_t file_size;
    void *bytes;
    QUEUEOS_EFI_STATUS status;

    *buffer = 0;
    *size = 0;
    if (system == 0 || system->boot_services == 0) {
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    boot = system->boot_services;
    if (boot->handle_protocol == 0 || boot->allocate_pool == 0) {
        return QUEUEOS_EFI_LOAD_ERROR;
    }

    loaded = 0;
    status = boot->handle_protocol(
        image,
        QUEUEOS_LOADED_IMAGE_GUID,
        (void **)&loaded);
    if (status != QUEUEOS_EFI_SUCCESS || loaded == 0 || loaded->device_handle == 0) {
        return QUEUEOS_EFI_LOAD_ERROR;
    }

    volume = 0;
    status = boot->handle_protocol(
        loaded->device_handle,
        QUEUEOS_SIMPLE_FILE_SYSTEM_GUID,
        (void **)&volume);
    if (status != QUEUEOS_EFI_SUCCESS || volume == 0 || volume->open_volume == 0) {
        return QUEUEOS_EFI_LOAD_ERROR;
    }

    root = 0;
    status = volume->open_volume(volume, &root);
    if (status != QUEUEOS_EFI_SUCCESS || root == 0 || root->open == 0) {
        return QUEUEOS_EFI_LOAD_ERROR;
    }

    for (i = 0; i < QUEUEOS_PATH_MAX - 1 && QUEUEOS_BASE_PATH[i] != '\0'; i++) {
        path[i] = (QUEUEOS_CHAR16)(uint8_t)QUEUEOS_BASE_PATH[i];
    }
    path[i] = 0;

    file = 0;
    status = root->open(root, &file, path, QUEUEOS_EFI_FILE_MODE_READ, 0);
    if (root->close != 0) {
        root->close(root);
    }
    if (status != QUEUEOS_EFI_SUCCESS || file == 0
        || file->set_position == 0 || file->get_position == 0 || file->read == 0) {
        return QUEUEOS_EFI_LOAD_ERROR;
    }

    /* End of the file, then the position, is the length. No EFI_FILE_INFO layout. */
    status = file->set_position(file, ~0ULL);
    file_size = 0;
    if (status == QUEUEOS_EFI_SUCCESS) {
        status = file->get_position(file, &file_size);
    }
    if (status == QUEUEOS_EFI_SUCCESS) {
        status = file->set_position(file, 0);
    }
    if (status != QUEUEOS_EFI_SUCCESS || file_size == 0 || file_size > (500u * 1024u)) {
        if (file->close != 0) {
            file->close(file);
        }
        return QUEUEOS_EFI_LOAD_ERROR;
    }

    bytes = 0;
    status = boot->allocate_pool(QUEUEOS_EFI_LOADER_DATA, (QUEUEOS_UINTN)file_size, &bytes);
    if (status != QUEUEOS_EFI_SUCCESS || bytes == 0) {
        if (file->close != 0) {
            file->close(file);
        }
        return QUEUEOS_EFI_LOAD_ERROR;
    }

    {
        QUEUEOS_UINTN got = (QUEUEOS_UINTN)file_size;
        status = file->read(file, &got, bytes);
        if (file->close != 0) {
            file->close(file);
        }
        if (status != QUEUEOS_EFI_SUCCESS || got != (QUEUEOS_UINTN)file_size) {
            if (boot->free_pool != 0) {
                boot->free_pool(bytes);
            }
            return QUEUEOS_EFI_LOAD_ERROR;
        }
    }

    *buffer = bytes;
    *size = (QUEUEOS_UINTN)file_size;
    return QUEUEOS_EFI_SUCCESS;
}

static QUEUEOS_EFI_STATUS queueos_start_base(
    QUEUEOS_EFI_SYSTEM_TABLE *system,
    void *image)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    void *buffer;
    QUEUEOS_UINTN size;
    void *child;
    QUEUEOS_EFI_STATUS status;

    status = queueos_read_base(system, image, &buffer, &size);
    if (status != QUEUEOS_EFI_SUCCESS) {
        return status;
    }
    boot = system->boot_services;
    child = 0;
    status = boot->load_image(QUEUEOS_FALSE, image, 0, buffer, size, &child);
    if (boot->free_pool != 0) {
        boot->free_pool(buffer);
    }
    if (status != QUEUEOS_EFI_SUCCESS || child == 0 || boot->start_image == 0) {
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    return boot->start_image(child, 0, 0);
}

static void queueos_boot_countdown(QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    int second;

    for (second = 10; second >= 1; second--) {
        char screen[48];
        char line[40];
        const char *prefix = "QueueOS-Boot: countdown ";
        int i = 0;
        int n = 0;

        while (QUEUEOS_BOOT_SENTENCE[n] != '\0' && i + 4 < (int)sizeof screen) {
            screen[i++] = QUEUEOS_BOOT_SENTENCE[n++];
        }
        screen[i++] = '\r';
        screen[i++] = '\n';
        if (second >= 10) {
            screen[i++] = '1';
            screen[i++] = '0';
        } else {
            screen[i++] = (char)('0' + second);
        }
        screen[i] = '\0';
        queueos_screen_show(system, screen);

        i = 0;
        while (prefix[i] != '\0' && i + 3 < (int)sizeof line) {
            line[i] = prefix[i];
            i++;
        }
        if (second >= 10) {
            line[i++] = '1';
            line[i++] = '0';
        } else {
            line[i++] = (char)('0' + second);
        }
        line[i] = '\0';
        queueos_serial_write_line(line);
        if (system != 0 && system->boot_services != 0 && system->boot_services->stall != 0) {
            system->boot_services->stall(1000u * 1000u);
        }
    }
}

QUEUEOS_EFI_STATUS QUEUEOS_EFIAPI efi_main(void *image, QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    QUEUEOS_EFI_STATUS status;

    queueos_report(system, "QueueOS-Boot: entry", QUEUEOS_BOOT_SENTENCE);
    queueos_boot_countdown(system);
    status = queueos_start_base(system, image);
    if (status != QUEUEOS_EFI_SUCCESS) {
        queueos_serial_init();
        queueos_serial_write_line(QUEUEOS_BOOT_FAIL_SENTENCE);
        return QUEUEOS_EFI_LOAD_ERROR;
    }
    queueos_serial_init();
    queueos_serial_write_line("QueueOS-Boot: base started");
    /* Last ConOut change before firmware runs. The failure return above
     * leaves the loading sentence visible. */
    queueos_screen_attribute(system, QUEUEOS_TEXT_RED_ON_RED);
    return QUEUEOS_EFI_SUCCESS;
}
