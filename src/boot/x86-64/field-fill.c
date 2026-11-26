/* Paint the light shade over the interior of the text console.
 *
 * ClearScreen is one rectangle. Writing U+2591 through the console is
 * one glyph blit per cell, and that is what makes the first picture
 * crawl. When the firmware cell is the usual 8 by 19 glyph, one cell
 * is copied from the console and stamped across the interior with the
 * framebuffer. A miss falls back to one OutputString per row, still
 * without a Lua call or a UTF-8 walk. The stamp is the glyph the
 * console just drew, so a later restore of U+2591 matches it.
 */

#include "field-fill.h"

#include <string.h>

/* 9042a9de-23dc-4a38-96fb-7aded080516a */
static const uint8_t GOP_GUID[16] = {
    0xDE, 0xA9, 0x42, 0x90, 0xDC, 0x23, 0x38, 0x4A,
    0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A
};

/* Light gray on red. The same value screen-run.c uses for the field. */
#define FIELD_ATTR ((QUEUEOS_UINTN)(0x07u | 0x40u))

enum { GLYPH_W = 8, GLYPH_H = 19 };
enum { BLT_TO_VIDEO = 1, BLT_FROM_VIDEO = 2 };

typedef struct GopPixel {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
} GopPixel;

typedef struct GopInfo {
    uint32_t version;
    uint32_t horizontal;
    uint32_t vertical;
    uint32_t format;
    uint32_t mask_red;
    uint32_t mask_green;
    uint32_t mask_blue;
    uint32_t mask_reserved;
    uint32_t pixels_per_scan;
} GopInfo;

typedef struct GopMode {
    uint32_t max_mode;
    uint32_t mode;
    GopInfo *info;
    uint64_t info_size;
    uint64_t fb_base;
    uint64_t fb_size;
} GopMode;

typedef struct Gop Gop;
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *GopBltFn)(
    Gop *self,
    GopPixel *buffer,
    uint32_t operation,
    QUEUEOS_UINTN src_x,
    QUEUEOS_UINTN src_y,
    QUEUEOS_UINTN dst_x,
    QUEUEOS_UINTN dst_y,
    QUEUEOS_UINTN width,
    QUEUEOS_UINTN height,
    QUEUEOS_UINTN delta);

struct Gop {
    void *query_mode;
    void *set_mode;
    GopBltFn blt;
    GopMode *mode;
};

typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *SetCursorFn)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    QUEUEOS_UINTN column,
    QUEUEOS_UINTN row);
typedef QUEUEOS_EFI_STATUS (QUEUEOS_EFIAPI *EnableCursorFn)(
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *self,
    QUEUEOS_BOOLEAN visible);

_Static_assert(sizeof(GopPixel) == 4, "gop pixel");
_Static_assert(offsetof(GopMode, info) == 8, "gop mode info");

static void hide_cursor(QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out)
{
    EnableCursorFn enable;

    if (out == 0 || out->enable_cursor == 0)
        return;
    enable = (EnableCursorFn)out->enable_cursor;
    enable(out, QUEUEOS_FALSE);
}

/* 0 when the cursor did not move. Writing anyway would land the glyph
 * on whatever cell the console still holds, which is the origin after
 * a clear. */
static int put_glyph(QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out,
                     int column, int row, QUEUEOS_CHAR16 glyph)
{
    SetCursorFn move;
    QUEUEOS_CHAR16 text[2];

    if (out == 0 || out->output_string == 0 || out->set_cursor_position == 0)
        return 0;
    move = (SetCursorFn)out->set_cursor_position;
    if (move(out, (QUEUEOS_UINTN)column, (QUEUEOS_UINTN)row) != QUEUEOS_EFI_SUCCESS)
        return 0;
    text[0] = glyph;
    text[1] = 0;
    out->output_string(out, text);
    return 1;
}

static int same_pixel(GopPixel a, GopPixel b)
{
    return a.blue == b.blue && a.green == b.green && a.red == b.red
        && a.reserved == b.reserved;
}

static int mostly(const GopPixel *pixels, int count, GopPixel color)
{
    int i;
    int hits = 0;

    for (i = 0; i < count; i++) {
        if (same_pixel(pixels[i], color))
            hits++;
    }
    return hits * 5 >= count * 4;
}

/* U+2591 is a light mix of the block and the space. A third color
 * means the cell we read is not that glyph.
 */
static int mixed(const GopPixel *pixels, int count, GopPixel ink, GopPixel paper)
{
    int i;
    int saw_ink = 0;
    int saw_paper = 0;

    for (i = 0; i < count; i++) {
        if (same_pixel(pixels[i], ink))
            saw_ink = 1;
        else if (same_pixel(pixels[i], paper))
            saw_paper = 1;
        else
            return 0;
    }
    return saw_ink && saw_paper;
}

static int read_cell(Gop *gop, GopPixel *tile, QUEUEOS_UINTN x, QUEUEOS_UINTN y)
{
    if (gop->blt == 0)
        return 0;
    return gop->blt(gop, tile, BLT_FROM_VIDEO, x, y, 0, 0,
                    GLYPH_W, GLYPH_H, 0) == QUEUEOS_EFI_SUCCESS;
}

static Gop *find_gop(QUEUEOS_EFI_SYSTEM_TABLE *system)
{
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    void **handles = 0;
    QUEUEOS_UINTN count = 0;
    Gop *gop = 0;
    QUEUEOS_EFI_STATUS status;

    if (system == 0 || system->boot_services == 0)
        return 0;
    boot = system->boot_services;
    if (boot->locate_handle_buffer == 0 || boot->handle_protocol == 0)
        return 0;
    status = boot->locate_handle_buffer(2, GOP_GUID, 0, &count, &handles);
    if (status != QUEUEOS_EFI_SUCCESS || count == 0 || handles == 0)
        return 0;
    boot->handle_protocol(handles[0], GOP_GUID, (void **)&gop);
    if (boot->free_pool != 0)
        boot->free_pool(handles);
    if (gop == 0 || gop->blt == 0 || gop->mode == 0 || gop->mode->info == 0)
        return 0;
    return gop;
}

/* One OutputString per row. The line starts at column 2 and stops
 * before the last column, including on the last row.
 */
static void text_fill(QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out,
                      int rows, int cols)
{
    SetCursorFn move;
    QUEUEOS_CHAR16 line[256];
    int n;
    int i;
    int y;

    if (out == 0 || out->output_string == 0 || cols < 3 || rows < 1)
        return;
    n = cols - 2;
    if (n > 255)
        n = 255;
    for (i = 0; i < n; i++)
        line[i] = 0x2591;
    line[n] = 0;
    if (out->set_attribute != 0)
        out->set_attribute(out, FIELD_ATTR);
    move = (SetCursorFn)out->set_cursor_position;
    for (y = 0; y < rows; y++) {
        if (move != 0)
            move(out, 1, (QUEUEOS_UINTN)y);
        out->output_string(out, line);
    }
}

/* Returns 1 when the interior was stamped. The caller paints with
 * text_fill when this returns 0, which also covers a sample glyph.
 */
static int gop_fill(QUEUEOS_EFI_SYSTEM_TABLE *system,
                    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out,
                    int rows, int cols)
{
    Gop *gop;
    GopInfo *info;
    GopPixel block[GLYPH_W * GLYPH_H];
    GopPixel space[GLYPH_W * GLYPH_H];
    GopPixel shade[GLYPH_W * GLYPH_H];
    GopPixel *row = 0;
    QUEUEOS_EFI_BOOT_SERVICES *boot;
    QUEUEOS_UINTN dx;
    QUEUEOS_UINTN dy;
    QUEUEOS_UINTN width;
    QUEUEOS_UINTN bytes;
    int across;
    int y;
    int ok = 0;
    GopPixel ink;
    GopPixel paper;

    gop = find_gop(system);
    if (gop == 0 || out == 0 || cols < 3 || rows < 1)
        return 0;
    info = gop->mode->info;
    if ((uint32_t)cols * GLYPH_W > info->horizontal
        || (uint32_t)rows * GLYPH_H > info->vertical)
        return 0;
    dx = (info->horizontal - (uint32_t)cols * GLYPH_W) / 2u;
    dy = (info->vertical - (uint32_t)rows * GLYPH_H) / 2u;
    if (out->set_attribute != 0)
        out->set_attribute(out, FIELD_ATTR);
    /* Column 2, row 1. That cell is interior, so the sample is covered
     * by the stamp or by the text fallback.
     */
    if (!put_glyph(out, 1, 0, 0x2588))
        return 0;
    if (!read_cell(gop, block, dx + GLYPH_W, dy))
        return 0;
    if (!read_cell(gop, space, dx, dy))
        return 0;
    ink = block[(GLYPH_H / 2) * GLYPH_W + (GLYPH_W / 2)];
    paper = space[(GLYPH_H / 2) * GLYPH_W + (GLYPH_W / 2)];
    if (same_pixel(ink, paper) || !mostly(block, GLYPH_W * GLYPH_H, ink))
        return 0;
    if (!mostly(space, GLYPH_W * GLYPH_H, paper))
        return 0;
    /* A space first, so the shade is not drawn on top of the block. */
    if (!put_glyph(out, 1, 0, ' ') || !put_glyph(out, 1, 0, 0x2591))
        return 0;
    if (!read_cell(gop, shade, dx + GLYPH_W, dy))
        return 0;
    if (!mixed(shade, GLYPH_W * GLYPH_H, ink, paper))
        return 0;
    boot = system->boot_services;
    if (boot == 0 || boot->allocate_pool == 0 || boot->free_pool == 0)
        return 0;
    across = cols - 2;
    width = (QUEUEOS_UINTN)across * GLYPH_W;
    bytes = width * GLYPH_H * sizeof(GopPixel);
    if (boot->allocate_pool(QUEUEOS_EFI_LOADER_DATA, bytes, (void **)&row) != 0
        || row == 0)
        return 0;
    for (y = 0; y < GLYPH_H; y++) {
        int x;

        for (x = 0; x < across; x++) {
            memcpy(row + (size_t)y * width + (size_t)x * GLYPH_W,
                   shade + y * GLYPH_W,
                   sizeof(GopPixel) * GLYPH_W);
        }
    }
    ok = 1;
    for (y = 0; y < rows; y++) {
        QUEUEOS_UINTN dest_y = dy + (QUEUEOS_UINTN)y * GLYPH_H;

        if (gop->blt(gop, row, BLT_TO_VIDEO, 0, 0, dx + GLYPH_W, dest_y,
                     width, GLYPH_H, 0) != QUEUEOS_EFI_SUCCESS) {
            ok = 0;
            break;
        }
    }
    boot->free_pool(row);
    return ok;
}

/* The sample block sits just inside the border. Replace it, and clear
 * the origin in case an earlier cursor move missed. */
static void settle_corner(QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out)
{
    if (out == 0 || out->output_string == 0)
        return;
    if (out->set_attribute != 0)
        out->set_attribute(out, FIELD_ATTR);
    put_glyph(out, 0, 0, ' ');
    put_glyph(out, 1, 0, 0x2591);
}

void queueos_field_fill(QUEUEOS_EFI_SYSTEM_TABLE *system,
                        QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out,
                        int rows, int cols)
{
    hide_cursor(out);
    if (!gop_fill(system, out, rows, cols))
        text_fill(out, rows, cols);
    settle_corner(out);
    hide_cursor(out);
}
