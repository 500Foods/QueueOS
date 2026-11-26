/* Firmware side of guest-screen.lua.
 *
 * UEFI text console, pointers, and the timer clock. U+2592 is drawn as
 * U+2591. Hover is yellow on red. List rows are black. The selected row
 * and a scrollbar thumb are white on red. Scrollbar arrows are bright red
 * on black. The QEMU tablet wheel is a signed byte, wheel-up negative.
 * The wait is WaitForEvent. Stall does not run pointer notifies.
 */

#include "screen-run.h"

#include "field-fill.h"
#include "guest-scripts.h"
#include "tablet-report.h"
#include "usb-tablet.h"
#include "serial.h"

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define QUEUEOS_FIELD ((QUEUEOS_UINTN)(0x07u | 0x40u))
#define QUEUEOS_HOVER ((QUEUEOS_UINTN)(0x0Eu | 0x40u))
#define QUEUEOS_LIST ((QUEUEOS_UINTN)0x0Fu)
#define QUEUEOS_LIST_HOVER ((QUEUEOS_UINTN)0x0Eu)
#define QUEUEOS_POINTER ((QUEUEOS_UINTN)0x0Eu)
#define QUEUEOS_ARROW ((QUEUEOS_UINTN)0x0Cu)

#define QUEUEOS_EFI_NOT_READY ((QUEUEOS_EFI_STATUS)0x8000000000000006ULL)

enum { WIDE_CAP = 160 };

/* EFI_ABSOLUTE_POINTER_PROTOCOL_GUID
 * 8d59d32b-c655-4ae9-9b15-f25904992a43
 */
static const uint8_t ABSOLUTE_GUID[16] = {
    0x2B, 0xD3, 0x59, 0x8D, 0x55, 0xC6, 0xE9, 0x4A,
    0x9B, 0x15, 0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43
};

/* EFI_SIMPLE_POINTER_PROTOCOL_GUID
 * 31878c87-0b75-11d5-9a4f-0090273fc14d
 */
static const uint8_t SIMPLE_GUID[16] = {
    0x87, 0x8C, 0x87, 0x31, 0x75, 0x0B, 0xD5, 0x11,
    0x9A, 0x4F, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D
};

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QueryModeFn)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    QUEUEOS_UINTN mode_number,
    QUEUEOS_UINTN *columns,
    QUEUEOS_UINTN *rows);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *SetCursorFn)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    QUEUEOS_UINTN column,
    QUEUEOS_UINTN row);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *EnableCursorFn)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    QUEUEOS_BOOLEAN visible);

typedef struct TextMode {
    int32_t max_mode;
    int32_t mode;
    int32_t attribute;
    int32_t cursor_column;
    int32_t cursor_row;
    uint8_t cursor_visible;
} TextMode;

typedef struct QueueosEfiTextInput QueueosEfiTextInput;
typedef struct QueueosEfiInputKey {
    uint16_t scan;
    uint16_t unicode;
} QueueosEfiInputKey;
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *QueueosEfiReadKey)(
    QueueosEfiTextInput *self,
    QueueosEfiInputKey *key);
struct QueueosEfiTextInput {
    void *reset;
    QueueosEfiReadKey read_key;
    void *wait_for_key;
};

typedef struct AbsMode {
    uint64_t min_x;
    uint64_t min_y;
    uint64_t min_z;
    uint64_t max_x;
    uint64_t max_y;
    uint64_t max_z;
    uint32_t attributes;
} AbsMode;

typedef struct AbsState {
    uint64_t x;
    uint64_t y;
    uint64_t z;
    uint32_t buttons;
} AbsState;

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *PointerResetFn)(
    void *self,
    QUEUEOS_BOOLEAN extended);

typedef struct AbsProto AbsProto;
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *AbsGetState)(
    AbsProto *self,
    AbsState *state);
struct AbsProto {
    PointerResetFn reset;
    AbsGetState get_state;
    void *wait_for_input;
    AbsMode *mode;
};

typedef struct SimpleMode {
    uint64_t res_x;
    uint64_t res_y;
    uint64_t res_z;
    uint8_t has_left;
    uint8_t has_right;
} SimpleMode;

typedef struct SimpleState {
    int32_t dx;
    int32_t dy;
    int32_t dz;
    uint8_t left;
    uint8_t right;
} SimpleState;

typedef struct SimpleProto SimpleProto;
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *SimpleGetState)(
    SimpleProto *self,
    SimpleState *state);
struct SimpleProto {
    PointerResetFn reset;
    SimpleGetState get_state;
    void *wait_for_input;
    SimpleMode *mode;
};

enum { POINTER_CAP = 4 };

static QUEUEOS_EFI_SYSTEM_TABLE *fw_system;
static uint64_t fw_us;
static int fw_cols = 80;
static int fw_rows = 25;
static AbsProto *fw_abs[POINTER_CAP];
static SimpleProto *fw_rel[POINTER_CAP];
static int fw_abs_n;
static int fw_rel_n;
static uint8_t fw_abs_down[POINTER_CAP];
static uint8_t fw_rel_down[POINTER_CAP];
static uint8_t fw_abs_have[POINTER_CAP];
static uint8_t fw_rel_seen[POINTER_CAP];
static int fw_abs_x[POINTER_CAP];
static int fw_abs_y[POINTER_CAP];
static int fw_cell_x = 1;
static int fw_cell_y = 1;
static int fw_placed;
static uint8_t fw_usb_down;
static int fw_wheel;
static int64_t fw_acc_x;
static int64_t fw_acc_y;

_Static_assert(offsetof(SimpleState, left) == 12, "simple pointer left button");
_Static_assert(offsetof(AbsState, buttons) == 24, "absolute pointer buttons");

static void note_failed(void)
{
    queueos_serial_init();
    queueos_serial_write_line("QueueOS-Boot: screen failed");
}

static const char *kind_name(int kind)
{
    if (kind == QUEUEOS_SLOT_EMPTY)
        return "empty";
    if (kind == QUEUEOS_SLOT_CANDIDATE)
        return "candidate";
    if (kind == QUEUEOS_SLOT_UNSAFE)
        return "unsafe";
    if (kind == QUEUEOS_SLOT_FOREIGN)
        return "foreign";
    return "unreadable";
}

static void read_geometry(void)
{
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out;
    QueryModeFn query;
    TextMode *mode;
    QUEUEOS_UINTN cols = 80;
    QUEUEOS_UINTN rows = 25;

    fw_cols = 80;
    fw_rows = 25;
    if (fw_system == 0 || fw_system->con_out == 0)
        return;
    out = fw_system->con_out;
    query = (QueryModeFn)out->query_mode;
    mode = (TextMode *)out->mode;
    if (query == 0 || mode == 0 || mode->mode < 0)
        return;
    if (query(out, (QUEUEOS_UINTN)mode->mode, &cols, &rows) != QUEUEOS_EFI_SUCCESS)
        return;
    if (cols < 1 || rows < 1 || cols > 256 || rows > 256)
        return;
    fw_cols = (int)cols;
    fw_rows = (int)rows;
}

static QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *console(void)
{
    if (fw_system == 0)
        return 0;
    return fw_system->con_out;
}

static void hide_cursor(QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out)
{
    EnableCursorFn enable;

    if (out == 0)
        return;
    enable = (EnableCursorFn)out->enable_cursor;
    if (enable != 0)
        enable(out, QUEUEOS_FALSE);
}

static int fw_rows_fn(lua_State *L)
{
    lua_pushinteger(L, fw_rows);
    return 1;
}

static int fw_cols_fn(lua_State *L)
{
    lua_pushinteger(L, fw_cols);
    return 1;
}

static int fw_field(lua_State *L)
{
    (void)L;
    queueos_field_fill(fw_system, console(), fw_rows, fw_cols);
    return 0;
}

static int fw_clear(lua_State *L)
{
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out = console();

    (void)L;
    if (out == 0 || out->set_attribute == 0 || out->clear_screen == 0)
        return 0;
    out->set_attribute(out, QUEUEOS_FIELD);
    out->clear_screen(out);
    hide_cursor(out);
    return 0;
}

/* One UTF-8 scalar, or a single byte when the sequence is not well formed.
 * The return value is how many bytes were consumed.
 */
static size_t decode_utf8(const unsigned char *s, size_t n, uint32_t *cp)
{
    size_t need;
    size_t i;
    uint32_t value;

    if (n == 0)
        return 0;
    if (s[0] < 0x80) {
        *cp = s[0];
        return 1;
    }
    if ((s[0] & 0xE0) == 0xC0)
        need = 2;
    else if ((s[0] & 0xF0) == 0xE0)
        need = 3;
    else if ((s[0] & 0xF8) == 0xF0)
        need = 4;
    else {
        *cp = (uint32_t)'?';
        return 1;
    }
    if (n < need) {
        *cp = (uint32_t)'?';
        return 1;
    }
    for (i = 1; i < need; i++) {
        if ((s[i] & 0xC0) != 0x80) {
            *cp = (uint32_t)'?';
            return 1;
        }
    }
    if (need == 2)
        value = ((uint32_t)(s[0] & 0x1Fu) << 6) | (s[1] & 0x3Fu);
    else if (need == 3)
        value = ((uint32_t)(s[0] & 0x0Fu) << 12)
            | ((uint32_t)(s[1] & 0x3Fu) << 6) | (s[2] & 0x3Fu);
    else
        value = ((uint32_t)(s[0] & 0x07u) << 18)
            | ((uint32_t)(s[1] & 0x3Fu) << 12)
            | ((uint32_t)(s[2] & 0x3Fu) << 6) | (s[3] & 0x3Fu);
    if ((need == 2 && value < 0x80u) || (need == 3 && value < 0x800u)
        || (need == 4 && value < 0x10000u))
        value = (uint32_t)'?';
    if (value >= 0xD800u && value <= 0xDFFFu)
        value = (uint32_t)'?';
    if (value == 0x2592u)
        value = 0x2591u;
    if (value == 0 || value > 0xFFFFu)
        value = (uint32_t)'?';
    *cp = value;
    return need;
}

static void to_char16(const char *text, size_t length, QUEUEOS_CHAR16 *out)
{
    size_t i = 0;
    int n = 0;

    while (i < length && n + 1 < WIDE_CAP) {
        uint32_t cp = (uint32_t)'?';
        size_t used = decode_utf8((const unsigned char *)text + i, length - i, &cp);

        if (used == 0)
            break;
        out[n++] = (QUEUEOS_CHAR16)cp;
        i += used;
    }
    out[n] = 0;
}

static QUEUEOS_UINTN attr_of(const char *name)
{
    if (name != 0 && strcmp(name, "white") == 0)
        return QUEUEOS_TEXT_WHITE_ON_RED;
    if (name != 0 && strcmp(name, "hover") == 0)
        return QUEUEOS_HOVER;
    if (name != 0 && strcmp(name, "bar") == 0)
        return QUEUEOS_TEXT_WHITE_ON_RED;
    if (name != 0 && strcmp(name, "list") == 0)
        return QUEUEOS_LIST;
    if (name != 0 && strcmp(name, "list-hover") == 0)
        return QUEUEOS_LIST_HOVER;
    if (name != 0 && strcmp(name, "pointer") == 0)
        return QUEUEOS_POINTER;
    if (name != 0 && strcmp(name, "arrow") == 0)
        return QUEUEOS_ARROW;
    return QUEUEOS_FIELD;
}

static int fw_write(lua_State *L)
{
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out = console();
    lua_Integer row;
    lua_Integer col;
    size_t length = 0;
    const char *text;
    const char *attr;
    QUEUEOS_CHAR16 wide[WIDE_CAP];
    SetCursorFn move;
    int is_row = 0;
    int is_col = 0;

    row = lua_tointegerx(L, 1, &is_row);
    col = lua_tointegerx(L, 2, &is_col);
    text = lua_tolstring(L, 3, &length);
    attr = lua_tostring(L, 4);
    if (!is_row || !is_col || text == 0 || out == 0 || out->output_string == 0)
        return 0;
    if (row < 1 || col < 1 || row > fw_rows || col > fw_cols)
        return 0;
    move = (SetCursorFn)out->set_cursor_position;
    if (out->set_attribute != 0)
        out->set_attribute(out, attr_of(attr));
    if (move != 0)
        move(out, (QUEUEOS_UINTN)(col - 1), (QUEUEOS_UINTN)(row - 1));
    to_char16(text, length, wide);
    /* A glyph in the last cell of the last row advances the cursor and
     * scrolls the console. Stop one cell short on that row.
     */
    {
        int glyphs = 0;
        int room;

        while (wide[glyphs] != 0)
            glyphs++;
        if ((int)row == fw_rows) {
            room = fw_cols - (int)col;
            if (room < 0)
                room = 0;
            if (glyphs > room)
                glyphs = room;
            wide[glyphs] = 0;
        }
    }
    if (wide[0] != 0)
        out->output_string(out, wide);
    return 0;
}

static const char *key_name(uint16_t scan, uint16_t uni)
{
    if (scan == 0x01)
        return "up";
    if (scan == 0x02)
        return "down";
    if (scan == 0x05)
        return "home";
    if (scan == 0x06)
        return "end";
    if (scan == 0x09)
        return "pageup";
    if (scan == 0x0A)
        return "pagedown";
    if (scan == 0x17 || uni == 0x1B)
        return "esc";
    if (uni == 0x0D || uni == 0x0A)
        return "enter";
    if (uni == 'q' || uni == 'Q')
        return "q";
    if (uni == 'e' || uni == 'E')
        return "e";
    if (uni == 'b' || uni == 'B')
        return "b";
    if (uni == 'j' || uni == 'J')
        return "j";
    if (uni == 'k' || uni == 'K')
        return "k";
    if (uni == ' ')
        return "space";
    return 0;
}

static const char *read_key(void)
{
    QueueosEfiTextInput *in;
    QueueosEfiInputKey key;
    QUEUEOS_EFI_STATUS status;

    if (fw_system == 0 || fw_system->con_in == 0)
        return 0;
    in = fw_system->con_in;
    if (in->read_key == 0)
        return 0;
    status = in->read_key(in, &key);
    if (status != QUEUEOS_EFI_SUCCESS)
        return 0;
    return key_name(key.scan, key.unicode);
}

static void show_cursor_at(int row, int col, int visible)
{
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out = console();
    SetCursorFn move;
    EnableCursorFn enable;

    if (out == 0)
        return;
    enable = (EnableCursorFn)out->enable_cursor;
    if (!visible) {
        if (enable != 0)
            enable(out, QUEUEOS_FALSE);
        return;
    }
    if (row < 1 || col < 1 || row > fw_rows || col > fw_cols)
        return;
    move = (SetCursorFn)out->set_cursor_position;
    if (move != 0)
        move(out, (QUEUEOS_UINTN)(col - 1), (QUEUEOS_UINTN)(row - 1));
    if (enable != 0)
        enable(out, QUEUEOS_TRUE);
}

static int fw_cursor(lua_State *L)
{
    int row = (int)lua_tointeger(L, 1);
    int col = (int)lua_tointeger(L, 2);
    int on = lua_toboolean(L, 3);

    show_cursor_at(row, col, on);
    return 0;
}

static void reset_pointer(PointerResetFn reset, void *proto)
{
    if (reset != 0 && proto != 0)
        reset(proto, QUEUEOS_FALSE);
}

static int take_protocols(const uint8_t *guid, void **slot, int cap)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    void **handles = 0;
    QUEUEOS_UINTN count = 0;
    QUEUEOS_UINTN i;
    int n = 0;
    QUEUEOS_EFI_STATUS status;

    if (fw_system == 0 || fw_system->boot_services == 0 || cap < 1)
        return 0;
    boot = fw_system->boot_services;
    if (boot->locate_handle_buffer == 0 || boot->handle_protocol == 0)
        return 0;
    status = boot->locate_handle_buffer(2, guid, 0, &count, &handles);
    if (status != QUEUEOS_EFI_SUCCESS || handles == 0 || count == 0) {
        if (handles != 0 && boot->free_pool != 0)
            boot->free_pool(handles);
        return 0;
    }
    for (i = 0; i < count && n < cap; i++) {
        void *proto = 0;

        status = boot->handle_protocol(handles[i], guid, &proto);
        if (status == QUEUEOS_EFI_SUCCESS && proto != 0)
            slot[n++] = proto;
    }
    if (boot->free_pool != 0)
        boot->free_pool(handles);
    return n;
}

static void bind_pointer(void)
{
    int i;

    fw_abs_n = 0;
    fw_rel_n = 0;
    fw_placed = 0;
    fw_usb_down = 0;
    fw_wheel = 0;
    fw_acc_x = 0;
    fw_acc_y = 0;
    fw_cell_x = 1;
    fw_cell_y = 1;
    for (i = 0; i < POINTER_CAP; i++) {
        fw_abs[i] = 0;
        fw_rel[i] = 0;
        fw_abs_down[i] = 0;
        fw_rel_down[i] = 0;
        fw_abs_have[i] = 0;
        fw_rel_seen[i] = 0;
        fw_abs_x[i] = 0;
        fw_abs_y[i] = 0;
    }
    fw_abs_n = take_protocols(ABSOLUTE_GUID, (void **)fw_abs, POINTER_CAP);
    fw_rel_n = take_protocols(SIMPLE_GUID, (void **)fw_rel, POINTER_CAP);
    for (i = 0; i < fw_abs_n; i++) {
        if (fw_abs[i] == 0 || fw_abs[i]->get_state == 0 || fw_abs[i]->mode == 0) {
            fw_abs[i] = 0;
            continue;
        }
        reset_pointer(fw_abs[i]->reset, fw_abs[i]);
    }
    for (i = 0; i < fw_rel_n; i++) {
        if (fw_rel[i] == 0 || fw_rel[i]->get_state == 0) {
            fw_rel[i] = 0;
            continue;
        }
        reset_pointer(fw_rel[i]->reset, fw_rel[i]);
    }
    queueos_tablet_open(fw_system);
}

static int buttons_down(void)
{
    int i;

    if (fw_usb_down)
        return 1;
    for (i = 0; i < fw_abs_n; i++) {
        if (fw_abs_down[i])
            return 1;
    }
    for (i = 0; i < fw_rel_n; i++) {
        if (fw_rel_down[i])
            return 1;
    }
    return 0;
}

/* Resolution is counts per millimetre. One cell is about one millimetre.
 * A device that reports no resolution moves one cell per eight counts.
 */
static int64_t counts_per_cell(uint64_t resolution)
{
    if (resolution == 0 || resolution > 1000000u)
        return 8;
    return (int64_t)resolution;
}

static void slide(int *cell, int64_t *acc, int64_t delta, int64_t step, int limit)
{
    int64_t next;

    if (step < 1)
        step = 1;
    *acc += delta;
    next = *acc / step;
    *acc -= next * step;
    next += *cell;
    if (next < 1) {
        next = 1;
        *acc = 0;
    } else if (next > limit) {
        next = limit;
        *acc = 0;
    }
    *cell = (int)next;
}

static int poll_pointer(int *x, int *y, int *click)
{
    int was = buttons_down();
    int moved = 0;
    int i;
    int old_x = fw_cell_x;
    int old_y = fw_cell_y;
    int was_placed = fw_placed;

    *click = 0;
    fw_wheel = 0;
    for (i = 0; i < fw_abs_n; i++) {
        AbsState state;
        AbsMode *mode;
        int cx;
        int cy;
        QUEUEOS_EFI_STATUS status;

        if (fw_abs[i] == 0)
            continue;
        memset(&state, 0, sizeof state);
        status = fw_abs[i]->get_state(fw_abs[i], &state);
        if (status != QUEUEOS_EFI_SUCCESS)
            continue;
        mode = fw_abs[i]->mode;
        cx = queueos_axis_cell(state.x, mode->min_x, mode->max_x, fw_cols);
        cy = queueos_axis_cell(state.y, mode->min_y, mode->max_y, fw_rows);
        fw_abs_down[i] = (state.buttons & 1u) != 0;
        if (!fw_abs_have[i] || cx != fw_abs_x[i] || cy != fw_abs_y[i]) {
            fw_abs_have[i] = 1;
            fw_abs_x[i] = cx;
            fw_abs_y[i] = cy;
            fw_cell_x = cx;
            fw_cell_y = cy;
            fw_acc_x = 0;
            fw_acc_y = 0;
            fw_placed = 1;
        }
    }
    for (i = 0; i < fw_rel_n; i++) {
        SimpleState state;
        uint64_t res_x = 0;
        uint64_t res_y = 0;
        QUEUEOS_EFI_STATUS status;

        if (fw_rel[i] == 0)
            continue;
        memset(&state, 0, sizeof state);
        status = fw_rel[i]->get_state(fw_rel[i], &state);
        if (status != QUEUEOS_EFI_SUCCESS)
            continue;
        if (fw_rel[i]->mode != 0) {
            res_x = fw_rel[i]->mode->res_x;
            res_y = fw_rel[i]->mode->res_y;
        }
        fw_rel_down[i] = state.left != 0;
        if (!fw_rel_seen[i]) {
            fw_rel_seen[i] = 1;
            if (!fw_placed) {
                fw_cell_x = fw_cols / 2;
                fw_cell_y = fw_rows / 2;
                if (fw_cell_x < 1)
                    fw_cell_x = 1;
                if (fw_cell_y < 1)
                    fw_cell_y = 1;
                fw_placed = 1;
            }
            continue;
        }
        /* Positive device Y is up. The text row grows downward. */
        slide(&fw_cell_x, &fw_acc_x, state.dx, counts_per_cell(res_x), fw_cols);
        slide(&fw_cell_y, &fw_acc_y, -(int64_t)state.dy, counts_per_cell(res_y), fw_rows);
        fw_placed = 1;
    }
    {
        int tx = fw_cell_x;
        int ty = fw_cell_y;
        int tdown = fw_usb_down;
        int twheel = 0;

        if (queueos_tablet_sample(fw_cols, fw_rows, &tx, &ty, &tdown, &twheel)) {
            fw_cell_x = tx;
            fw_cell_y = ty;
            fw_usb_down = (uint8_t)tdown;
            fw_wheel = twheel;
            fw_acc_x = 0;
            fw_acc_y = 0;
            fw_placed = 1;
        }
    }
    if (!fw_placed && (fw_abs_n > 0 || fw_rel_n > 0)) {
        int live = 0;

        for (i = 0; i < fw_abs_n; i++)
            live = live || fw_abs[i] != 0;
        for (i = 0; i < fw_rel_n; i++)
            live = live || fw_rel[i] != 0;
        if (live) {
            fw_cell_x = fw_cols / 2;
            fw_cell_y = fw_rows / 2;
            if (fw_cell_x < 1)
                fw_cell_x = 1;
            if (fw_cell_y < 1)
                fw_cell_y = 1;
            fw_placed = 1;
        }
    }
    if (!fw_placed)
        return 0;
    if (!was_placed || fw_cell_x != old_x || fw_cell_y != old_y)
        moved = 1;
    *click = was && !buttons_down();
    if (!moved && !*click && fw_wheel == 0)
        return 0;
    *x = fw_cell_x;
    *y = fw_cell_y;
    return 1;
}

static void *fw_tick;

/* Pointer drivers arm EVT_TIMER notifies. Those run when this wait drops
 * to TPL_APPLICATION. Stall leaves them queued. TimerRelative is 2.
 */
static void wait_slice(unsigned microseconds)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *create_event)(
        uint32_t type, QUEUEOS_UINTN tpl, void *notify, void *context, void **event);
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *set_timer)(void *event, uint32_t type, uint64_t trigger);
    QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *wait_for)(
        QUEUEOS_UINTN number, void **event, QUEUEOS_UINTN *index);
    QUEUEOS_UINTN index = 0;

    fw_us += microseconds;
    boot = fw_system != 0 ? fw_system->boot_services : 0;
    if (boot == 0)
        return;
    create_event = (QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *)(
        uint32_t, QUEUEOS_UINTN, void *, void *, void **))boot->create_event;
    set_timer = (QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *)(
        void *, uint32_t, uint64_t))boot->set_timer;
    wait_for = (QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *)(
        QUEUEOS_UINTN, void **, QUEUEOS_UINTN *))boot->wait_for_event;
    if (fw_tick == 0 && create_event != 0 && create_event(0x80000000u, 8, 0, 0, &fw_tick) != 0)
        fw_tick = 0;
    if (fw_tick != 0 && set_timer != 0 && wait_for != 0
        && set_timer(fw_tick, 2, (uint64_t)microseconds * 10u) == 0
        && wait_for(1, &fw_tick, &index) == 0)
        return;
    if (boot->stall != 0)
        boot->stall(microseconds);
}

static void push_pointer(lua_State *L, int x, int y, int click)
{
    lua_createtable(L, 0, 5);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "pointer");
    lua_pushinteger(L, x);
    lua_setfield(L, -2, "x");
    lua_pushinteger(L, y);
    lua_setfield(L, -2, "y");
    lua_pushboolean(L, click);
    lua_setfield(L, -2, "click");
    lua_pushinteger(L, fw_wheel);
    lua_setfield(L, -2, "wheel");
}

static int fw_wait(lua_State *L)
{
    unsigned waited = 0;

    while (waited < 100000u) {
        const char *key;
        unsigned slice = 20000u;
        int x = 1;
        int y = 1;
        int click = 0;

        if (100000u - waited < slice)
            slice = 100000u - waited;
        wait_slice(slice);
        waited += slice;
        key = read_key();
        if (key != 0) {
            lua_createtable(L, 0, 1);
            lua_pushstring(L, key);
            lua_setfield(L, -2, "key");
            return 1;
        }
        if (poll_pointer(&x, &y, &click)) {
            push_pointer(L, x, y, click);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

static int fw_now(lua_State *L)
{
    lua_pushnumber(L, (lua_Number)fw_us / 1000000.0);
    return 1;
}

static void install_firmware(lua_State *L)
{
    lua_createtable(L, 0, 8);
    lua_pushcfunction(L, fw_rows_fn);
    lua_setfield(L, -2, "rows");
    lua_pushcfunction(L, fw_cols_fn);
    lua_setfield(L, -2, "cols");
    lua_pushcfunction(L, fw_clear);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, fw_field);
    lua_setfield(L, -2, "field");
    lua_pushcfunction(L, fw_write);
    lua_setfield(L, -2, "write");
    lua_pushcfunction(L, fw_wait);
    lua_setfield(L, -2, "wait");
    lua_pushcfunction(L, fw_now);
    lua_setfield(L, -2, "now");
    lua_pushcfunction(L, fw_cursor);
    lua_setfield(L, -2, "cursor");
    lua_setglobal(L, "firmware");
}

static void push_text(lua_State *L, const char *key, const char *value)
{
    lua_pushstring(L, value != 0 ? value : "");
    lua_setfield(L, -2, key);
}

static int push_slots(lua_State *L, const QueueosGuestSlot *slots, int count)
{
    int i;

    if (count < 0)
        count = 0;
    if (count > QUEUEOS_SLOT_COUNT)
        count = QUEUEOS_SLOT_COUNT;
    lua_createtable(L, count, 0);
    for (i = 0; i < count; i++) {
        const QueueosGuestSlot *slot = &slots[i];

        lua_createtable(L, 0, 13);
        push_text(L, "kind", kind_name(slot->kind));
        push_text(L, "name", slot->name);
        push_text(L, "version", slot->version);
        push_text(L, "published", slot->published);
        lua_pushinteger(L, (lua_Integer)slot->countdown);
        lua_setfield(L, -2, "countdown");
        push_text(L, "booted", slot->booted);
        push_text(L, "tag", slot->tag);
        push_text(L, "file", slot->file);
        lua_pushboolean(L, slot->launchable ? 1 : 0);
        lua_setfield(L, -2, "launchable");
        lua_pushinteger(L, (lua_Integer)(i + 1));
        lua_setfield(L, -2, "index");
        if (slot->json != 0) {
            lua_pushstring(L, slot->json);
            lua_setfield(L, -2, "json");
        }
        if (slot->launchable) {
            lua_createtable(L, 0, 4);
            push_text(L, "file", slot->file);
            lua_pushinteger(L, (lua_Integer)slot->offset);
            lua_setfield(L, -2, "offset");
            lua_pushinteger(L, (lua_Integer)slot->length);
            lua_setfield(L, -2, "length");
            push_text(L, "sha256", slot->sha);
            lua_setfield(L, -2, "base");
        }
        lua_rawseti(L, -2, (lua_Integer)(i + 1));
    }
    lua_setglobal(L, "slots");
    return 0;
}

static int load_script(lua_State *L, const char *name)
{
    const QueueosLuaScript *script = queueos_lua_script(name);

    if (script == 0 || script->bytes == 0)
        return -1;
    if (luaL_loadbuffer(L, (const char *)script->bytes, script->length, name) != LUA_OK)
        return -1;
    if (lua_pcall(L, 0, 1, 0) != LUA_OK)
        return -1;
    return 0;
}

static int publish_module(lua_State *L, const char *name)
{
    if (load_script(L, name) != 0)
        return -1;
    lua_getglobal(L, "package");
    if (!lua_istable(L, -1))
        return -1;
    lua_getfield(L, -1, "loaded");
    if (!lua_istable(L, -1))
        return -1;
    lua_pushvalue(L, -3);
    lua_setfield(L, -2, name);
    lua_pop(L, 3);
    return 0;
}

static int chosen_slot(lua_State *L)
{
    int isnum = 0;
    lua_Integer value = lua_tointegerx(L, -1, &isnum);
    lua_Number as_float;
    int isfloat = 0;

    if (!isnum) {
        as_float = lua_tonumberx(L, -1, &isfloat);
        if (!isfloat)
            return 0;
        value = (lua_Integer)as_float;
        if ((lua_Number)value != as_float)
            return 0;
    }
    if (value < 1 || value > QUEUEOS_SLOT_COUNT)
        return 0;
    return (int)value;
}

int queueos_screen_run(QUEUEOS_EFI_SYSTEM_TABLE *system,
                       const QueueosGuestSlot *slots,
                       int count)
{
    lua_State *L;
    int chosen = 0;
    static const char *modules[] = {
        "wordmark",
        "screen-geo",
        "boot-screen",
        "json-text"
    };
    int i;

    fw_system = system;
    fw_us = 0;
    read_geometry();
    bind_pointer();
    if (slots == 0 || count < 1) {
        note_failed();
        return 0;
    }
    L = luaL_newstate();
    if (L == 0) {
        note_failed();
        return 0;
    }
    luaL_openlibs(L);
    install_firmware(L);
    if (push_slots(L, slots, count) != 0) {
        note_failed();
        lua_close(L);
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (publish_module(L, modules[i]) != 0) {
            note_failed();
            lua_close(L);
            return 0;
        }
    }
    if (load_script(L, "guest-screen") != 0) {
        note_failed();
        lua_close(L);
        return 0;
    }
    chosen = chosen_slot(L);
    lua_close(L);
    return chosen;
}
