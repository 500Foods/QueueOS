/* Minimal UEFI types for the x86-64 stubs.
 *
 * Firmware calls efi_main with the Microsoft x64 convention. Host gcc uses
 * System V. QUEUEOS_EFIAPI is the boundary: the entry point and every
 * firmware function pointer use it. Helpers firmware does not call stay
 * System V.
 *
 * CHAR16 is uint16_t. wchar_t is not used. -fshort-wchar is still set so a
 * slip does not become a 32-bit character.
 */
#ifndef QUEUEOS_EFI_TYPES_H
#define QUEUEOS_EFI_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define QUEUEOS_EFIAPI __attribute__((ms_abi))

typedef uint8_t QUEUEOS_BOOLEAN;
typedef uint16_t QUEUEOS_CHAR16;
typedef uint64_t QUEUEOS_UINTN;
typedef uint64_t QUEUEOS_EFI_STATUS;

#define QUEUEOS_EFI_SUCCESS 0
#define QUEUEOS_EFI_LOAD_ERROR ((QUEUEOS_EFI_STATUS)0x8000000000000001ULL)
#define QUEUEOS_TRUE 1
#define QUEUEOS_FALSE 0

/* White foreground, red background, in the UEFI text palette. */
#define QUEUEOS_TEXT_WHITE_ON_RED ((QUEUEOS_UINTN)(0x0Fu | 0x40u))

/* Same red for the glyph and the field. SetAttribute does not repaint
 * characters that are already on screen. Later firmware text that leaves
 * the attribute alone is drawn in this color and disappears into the field. */
#define QUEUEOS_TEXT_RED_ON_RED ((QUEUEOS_UINTN)(0x04u | 0x40u))

typedef struct QUEUEOS_EFI_TABLE_HEADER {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} QUEUEOS_EFI_TABLE_HEADER;

typedef struct QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_TEXT_STRING)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    QUEUEOS_CHAR16 *string);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_TEXT_SET_ATTRIBUTE)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    QUEUEOS_UINTN attribute);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_TEXT_CLEAR_SCREEN)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self);

struct QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    void *reset;
    QUEUEOS_EFI_TEXT_STRING output_string;
    void *test_string;
    void *query_mode;
    void *set_mode;
    QUEUEOS_EFI_TEXT_SET_ATTRIBUTE set_attribute;
    QUEUEOS_EFI_TEXT_CLEAR_SCREEN clear_screen;
    void *set_cursor_position;
    void *enable_cursor;
    void *mode;
};

typedef struct QUEUEOS_EFI_BOOT_SERVICES {
    QUEUEOS_EFI_TABLE_HEADER header;
    void *raise_tpl;
    void *restore_tpl;
    void *allocate_pages;
    void *free_pages;
    void *get_memory_map;
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *allocate_pool)(
        uint32_t pool_type,
        QUEUEOS_UINTN size,
        void **buffer);
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *free_pool)(void *buffer);
    void *create_event;
    void *set_timer;
    void *wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *handle_protocol)(
        void *handle,
        const uint8_t *protocol,
        void **interface);
    void *reserved;
    void *register_protocol_notify;
    void *locate_handle;
    void *locate_device_path;
    void *install_configuration_table;
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *load_image)(
        QUEUEOS_BOOLEAN boot_policy,
        void *parent_image,
        void *device_path,
        void *source_buffer,
        QUEUEOS_UINTN source_size,
        void **image);
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *start_image)(
        void *image,
        QUEUEOS_UINTN *exit_data_size,
        QUEUEOS_CHAR16 **exit_data);
    void *exit;
    void *unload_image;
    void *exit_boot_services;
    void *get_next_monotonic_count;
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *stall)(QUEUEOS_UINTN microseconds);
    void *set_watchdog_timer;
    void *connect_controller;
    void *disconnect_controller;
    void *open_protocol;
    void *close_protocol;
    void *open_protocol_information;
    void *protocols_per_handle;
    /* ByProtocol is 2. The handle buffer is firmware pool memory. */
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *locate_handle_buffer)(
        uint32_t search_type,
        const uint8_t *protocol,
        void *search_key,
        QUEUEOS_UINTN *no_handles,
        void ***buffer);
} QUEUEOS_EFI_BOOT_SERVICES;

typedef struct QUEUEOS_EFI_SYSTEM_TABLE {
    QUEUEOS_EFI_TABLE_HEADER header;
    QUEUEOS_CHAR16 *firmware_vendor;
    uint32_t firmware_revision;
    void *console_in_handle;
    void *con_in;
    void *console_out_handle;
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *con_out;
    void *standard_error_handle;
    void *std_err;
    void *runtime_services;
    QUEUEOS_EFI_BOOT_SERVICES *boot_services;
    QUEUEOS_UINTN number_of_table_entries;
    void *configuration_table;
} QUEUEOS_EFI_SYSTEM_TABLE;

/* EfiLoaderData. The read buffer for Base is not executable code. */
#define QUEUEOS_EFI_LOADER_DATA 2u

typedef struct QUEUEOS_EFI_LOADED_IMAGE {
    uint32_t revision;
    void *parent_handle;
    QUEUEOS_EFI_SYSTEM_TABLE *system;
    void *device_handle;
    void *file_path;
    void *reserved;
    uint32_t load_options_size;
    void *load_options;
    void *image_base;
    uint64_t image_size;
    uint32_t image_code_type;
    uint32_t image_data_type;
    void *unload;
} QUEUEOS_EFI_LOADED_IMAGE;

typedef struct QUEUEOS_EFI_FILE_PROTOCOL QUEUEOS_EFI_FILE_PROTOCOL;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_FILE_OPEN)(
    QUEUEOS_EFI_FILE_PROTOCOL *self,
    QUEUEOS_EFI_FILE_PROTOCOL **new_handle,
    QUEUEOS_CHAR16 *file_name,
    uint64_t open_mode,
    uint64_t attributes);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_FILE_CLOSE)(
    QUEUEOS_EFI_FILE_PROTOCOL *self);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_FILE_READ)(
    QUEUEOS_EFI_FILE_PROTOCOL *self,
    QUEUEOS_UINTN *buffer_size,
    void *buffer);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_FILE_GET_POSITION)(
    QUEUEOS_EFI_FILE_PROTOCOL *self,
    uint64_t *position);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_FILE_SET_POSITION)(
    QUEUEOS_EFI_FILE_PROTOCOL *self,
    uint64_t position);

struct QUEUEOS_EFI_FILE_PROTOCOL {
    uint64_t revision;
    QUEUEOS_EFI_FILE_OPEN open;
    QUEUEOS_EFI_FILE_CLOSE close;
    void *delete;
    QUEUEOS_EFI_FILE_READ read;
    void *write;
    QUEUEOS_EFI_FILE_GET_POSITION get_position;
    QUEUEOS_EFI_FILE_SET_POSITION set_position;
    void *get_info;
    void *set_info;
    void *flush;
};

typedef struct QUEUEOS_EFI_SIMPLE_FILE_SYSTEM QUEUEOS_EFI_SIMPLE_FILE_SYSTEM;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QUEUEOS_EFI_OPEN_VOLUME)(
    QUEUEOS_EFI_SIMPLE_FILE_SYSTEM *self,
    QUEUEOS_EFI_FILE_PROTOCOL **root);

struct QUEUEOS_EFI_SIMPLE_FILE_SYSTEM {
    uint64_t revision;
    QUEUEOS_EFI_OPEN_VOLUME open_volume;
};

#define QUEUEOS_EFI_FILE_MODE_READ 0x0000000000000001ULL

#endif
