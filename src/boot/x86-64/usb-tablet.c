/* Read one absolute HID pointer with EFI_USB_IO_PROTOCOL.
 *
 * The keyboard is a different interface and is left to the firmware.
 * A boot mouse is relative, so the report walk rejects it.
 */

#include "usb-tablet.h"

#include "tablet-report.h"

#include <string.h>

/* 2B2F68D6-0CD2-44CF-8E8B-BBA20B1B5B75 */
static const uint8_t USB_IO_GUID[16] = {
    0xD6, 0x68, 0x2F, 0x2B, 0xD2, 0x0C, 0xCF, 0x44,
    0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75
};

typedef struct __attribute__((packed)) UsbRequest {
    uint8_t request_type;
    uint8_t request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
} UsbRequest;

typedef struct __attribute__((packed)) UsbInterface {
    uint8_t length;
    uint8_t type;
    uint8_t number;
    uint8_t alternate;
    uint8_t endpoints;
    uint8_t class_id;
    uint8_t subclass;
    uint8_t protocol;
    uint8_t interface_str;
} UsbInterface;

typedef struct __attribute__((packed)) UsbEndpoint {
    uint8_t length;
    uint8_t type;
    uint8_t address;
    uint8_t attributes;
    uint16_t max_packet;
    uint8_t interval;
} UsbEndpoint;

typedef struct __attribute__((packed)) UsbConfig {
    uint8_t length;
    uint8_t type;
    uint16_t total;
    uint8_t interfaces;
    uint8_t configuration;
    uint8_t configuration_str;
    uint8_t attributes;
    uint8_t max_power;
} UsbConfig;

typedef struct UsbIo UsbIo;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *UsbControlFn)(
    UsbIo *self,
    UsbRequest *request,
    uint32_t direction,
    uint32_t timeout,
    void *data,
    QUEUEOS_UINTN length,
    uint32_t *usb_status);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *UsbSyncFn)(
    UsbIo *self,
    uint8_t endpoint,
    void *data,
    QUEUEOS_UINTN *length,
    QUEUEOS_UINTN timeout,
    uint32_t *usb_status);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *UsbGetConfigFn)(
    UsbIo *self,
    UsbConfig *desc);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *UsbGetInterfaceFn)(
    UsbIo *self,
    UsbInterface *desc);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *UsbGetEndpointFn)(
    UsbIo *self,
    uint8_t index,
    UsbEndpoint *desc);

struct UsbIo {
    UsbControlFn control;
    void *bulk;
    void *async_interrupt;
    UsbSyncFn sync_interrupt;
    void *iso;
    void *async_iso;
    void *get_device;
    UsbGetConfigFn get_config;
    UsbGetInterfaceFn get_interface;
    UsbGetEndpointFn get_endpoint;
    void *get_string;
    void *get_languages;
    void *port_reset;
};

_Static_assert(sizeof(UsbRequest) == 8, "usb setup packet");
_Static_assert(sizeof(UsbInterface) == 9, "usb interface descriptor");
_Static_assert(sizeof(UsbEndpoint) == 7, "usb endpoint descriptor");
_Static_assert(sizeof(UsbConfig) == 9, "usb config descriptor");
_Static_assert(sizeof(UsbIo) == 13 * sizeof(void *), "usb io protocol");

static UsbIo *tablet_io;
static uint8_t tablet_ep;
static unsigned tablet_packet;
static QueueosTabletLayout tablet_layout;
static int tablet_ready;

static int control(UsbIo *io, UsbRequest *request, uint32_t direction,
                   void *data, QUEUEOS_UINTN length)
{
    uint32_t usb_status = 1;

    if (io == 0 || io->control == 0)
        return 0;
    if (io->control(io, request, direction, 100, data, length, &usb_status) != 0)
        return 0;
    return usb_status == 0;
}

static void class_out(UsbIo *io, uint8_t request, uint16_t value, uint16_t index)
{
    UsbRequest packet;

    memset(&packet, 0, sizeof packet);
    packet.request_type = 0x21;
    packet.request = request;
    packet.value = value;
    packet.index = index;
    control(io, &packet, 2, 0, 0);
}

static unsigned hid_length(const uint8_t *buf, unsigned n, uint8_t interface)
{
    unsigned p = 0;
    int current = -1;
    unsigned found = 0;

    while (p + 2u <= n) {
        unsigned len = buf[p];
        unsigned type = buf[p + 1u];

        if (len < 2u || p + len > n)
            break;
        if (type == 4u && len >= 9u)
            current = buf[p + 2u];
        if (type == 0x21u && len >= 9u && current == (int)interface
            && buf[p + 6u] == 0x22u)
            found = (unsigned)buf[p + 7u] | ((unsigned)buf[p + 8u] << 8);
        p += len;
    }
    return found;
}

static int read_report_desc(UsbIo *io, uint8_t interface,
                            uint8_t *desc, unsigned cap, unsigned *out_len)
{
    uint8_t config[512];
    UsbConfig header;
    UsbRequest packet;
    unsigned total = 0;
    unsigned n;

    memset(config, 0, sizeof config);
    memset(&packet, 0, sizeof packet);
    memset(&header, 0, sizeof header);
    /* The firmware copies the 9-byte descriptor into the caller buffer. */
    if (io->get_config != 0 && io->get_config(io, &header) == 0)
        total = header.total;
    if (total < 9u || total > sizeof config)
        total = 256;
    packet.request_type = 0x80;
    packet.request = 0x06;
    packet.value = 0x0200;
    packet.index = 0;
    packet.length = (uint16_t)total;
    if (!control(io, &packet, 0, config, total))
        return 0;
    n = hid_length(config, total, interface);
    if (n == 0 || n > cap)
        n = cap < 96u ? cap : 96u;
    memset(&packet, 0, sizeof packet);
    memset(desc, 0, cap);
    packet.request_type = 0x81;
    packet.request = 0x06;
    packet.value = 0x2200;
    packet.index = interface;
    packet.length = (uint16_t)n;
    if (!control(io, &packet, 0, desc, n))
        return 0;
    *out_len = n;
    return 1;
}

static int consider(UsbIo *io)
{
    UsbInterface iface;
    uint8_t desc[256];
    unsigned desc_len = 0;
    uint8_t index;
    uint8_t address = 0;
    unsigned packet = 8;
    QueueosTabletLayout layout;

    if (io == 0 || io->get_interface == 0 || io->get_endpoint == 0)
        return 0;
    memset(&iface, 0, sizeof iface);
    if (io->get_interface(io, &iface) != 0)
        return 0;
    if (iface.class_id != 3 || iface.protocol == 1 || iface.endpoints == 0)
        return 0;
    for (index = 0; index < iface.endpoints; index++) {
        UsbEndpoint ep;

        memset(&ep, 0, sizeof ep);
        if (io->get_endpoint(io, index, &ep) != 0)
            continue;
        if ((ep.attributes & 3u) == 3u && (ep.address & 0x80u) != 0) {
            address = ep.address;
            packet = ep.max_packet & 0x07ffu;
            break;
        }
    }
    if (address == 0)
        return 0;
    if (packet == 0 || packet > 64u)
        packet = 8;
    if (!read_report_desc(io, iface.number, desc, sizeof desc, &desc_len))
        return 0;
    if (!queueos_tablet_layout(desc, desc_len, &layout))
        return 0;
    /* SET_PROTOCOL stalls on this tablet. SET_IDLE is what arms reports. */
    class_out(io, 0x0A, 0, iface.number);
    {
        UsbRequest halt;

        memset(&halt, 0, sizeof halt);
        halt.request_type = 0x02;
        halt.request = 0x01;
        halt.value = 0;
        halt.index = address;
        control(io, &halt, 2, 0, 0);
    }
    tablet_io = io;
    tablet_ep = address;
    tablet_packet = packet;
    tablet_layout = layout;
    tablet_ready = 1;
    return 1;
}

void queueos_tablet_open(QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    void **handles = 0;
    QUEUEOS_UINTN count = 0;
    QUEUEOS_UINTN i;

    tablet_io = 0;
    tablet_ep = 0;
    tablet_packet = 0;
    tablet_ready = 0;
    memset(&tablet_layout, 0, sizeof tablet_layout);
    if (system == 0 || system->boot_services == 0)
        return;
    boot = system->boot_services;
    if (boot->locate_handle_buffer == 0 || boot->handle_protocol == 0)
        return;
    if (boot->locate_handle_buffer(2, USB_IO_GUID, 0, &count, &handles) != 0
        || handles == 0)
        return;
    for (i = 0; i < count; i++) {
        void *proto = 0;

        if (boot->handle_protocol(handles[i], USB_IO_GUID, &proto) != 0)
            continue;
        if (consider((UsbIo *)proto))
            break;
    }
    if (boot->free_pool != 0)
        boot->free_pool(handles);
}

static int map_axis(int32_t raw, int32_t minv, int32_t maxv, int cells)
{
    if (raw < minv)
        raw = minv;
    if (raw > maxv)
        raw = maxv;
    return queueos_axis_cell((uint64_t)(raw - minv), 0,
                             (uint64_t)(maxv - minv), cells);
}

int queueos_tablet_sample(int cols, int rows, int *x, int *y, int *down,
                          int *wheel)
{
    uint8_t report[64];
    QUEUEOS_UINTN length;
    uint32_t usb_status = 1;
    int32_t raw_x = 0;
    int32_t raw_y = 0;
    int button = 0;
    int notch = 0;

    if (!tablet_ready || tablet_io == 0 || tablet_io->sync_interrupt == 0)
        return 0;
    if (x == 0 || y == 0 || down == 0 || wheel == 0)
        return 0;
    memset(report, 0, sizeof report);
    length = tablet_packet;
    if (length > sizeof report)
        length = sizeof report;
    if (tablet_io->sync_interrupt(tablet_io, tablet_ep, report, &length, 1,
                                  &usb_status) != 0
        || usb_status != 0 || length == 0)
        return 0;
    if (!queueos_tablet_read(&tablet_layout, report, length, &raw_x, &raw_y,
                             &button, &notch))
        return 0;
    *x = map_axis(raw_x, tablet_layout.x_min, tablet_layout.x_max, cols);
    *y = map_axis(raw_y, tablet_layout.y_min, tablet_layout.y_max, rows);
    *down = button;
    *wheel = notch;
    return 1;
}
