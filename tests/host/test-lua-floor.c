/* Freestanding host link of vendored Lua. No firmware.
 * pcall proves _setjmp. The snippet is what the boot scripts call.
 */

#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t heap[8u * 1024u * 1024u];
static size_t heap_used;

struct cell {
    size_t size;
    int live;
};

static void finish(int code)
{
    __asm__ volatile(
        "syscall"
        :
        : "a"((long)60), "D"((long)code)
        : "rcx", "r11", "memory");
}

static void *place(size_t bytes)
{
    size_t aligned = (bytes + 15u) & ~(size_t)15u;
    uint8_t *p = heap;
    struct cell *cell;

    if (aligned < bytes)
        return 0;
    while (p + sizeof(struct cell) <= heap + heap_used) {
        cell = (struct cell *)p;
        if (!cell->live && cell->size >= aligned) {
            cell->live = 1;
            return (uint8_t *)cell + sizeof(struct cell);
        }
        if (cell->size > sizeof heap)
            return 0;
        p += sizeof(struct cell) + cell->size;
    }
    if (heap_used + sizeof(struct cell) + aligned > sizeof heap)
        return 0;
    cell = (struct cell *)(heap + heap_used);
    cell->size = aligned;
    cell->live = 1;
    heap_used += sizeof(struct cell) + aligned;
    return (uint8_t *)cell + sizeof(struct cell);
}

void *malloc(size_t bytes)
{
    void *p;
    uint8_t *out;
    size_t i;

    p = place(bytes);
    if (p == 0)
        return 0;
    out = p;
    for (i = 0; i < bytes; i++)
        out[i] = 0;
    return p;
}

void free(void *p)
{
    struct cell *cell;

    if (p == 0)
        return;
    cell = (struct cell *)((uint8_t *)p - sizeof(struct cell));
    cell->live = 0;
}

void *realloc(void *p, size_t bytes)
{
    struct cell *cell;
    void *next;
    uint8_t *src;
    uint8_t *dst;
    size_t i;
    size_t keep;

    if (p == 0)
        return malloc(bytes);
    if (bytes == 0) {
        free(p);
        return 0;
    }
    cell = (struct cell *)((uint8_t *)p - sizeof(struct cell));
    if (cell->size >= bytes)
        return p;
    next = malloc(bytes);
    if (next == 0)
        return 0;
    src = p;
    dst = next;
    keep = cell->size;
    if (keep > bytes)
        keep = bytes;
    for (i = 0; i < keep; i++)
        dst[i] = src[i];
    free(p);
    return next;
}

/* The kernel enters this file with the stack 16-byte aligned. A normal
 * main is entered one word below that, which is what va_start assumes.
 */
__attribute__((naked)) void test_main(void)
{
    __asm__ volatile("call run_test\n\t");
}

void run_test(void)
{
    lua_State *state;
    static const char *script =
        "local ok, value = pcall(function() return math.floor(3.2) end)\n"
        "if not ok or value ~= 3 then return 1 end\n"
        "if math.max(1, 4) ~= 4 or math.min(1, 4) ~= 1 then return 2 end\n"
        "if 2 ^ 53 ~= 9007199254740992 then return 3 end\n"
        "if string.format('%.0f', 3.2) ~= '3' then return 4 end\n"
        "if string.format('%04x', 10) ~= '000a' then return 5 end\n"
        "if string.format('%2d', 3) ~= ' 3' then return 6 end\n"
        "return 0\n";

    state = luaL_newstate();
    if (state == 0)
        finish(2);
    luaL_openlibs(state);
    if (luaL_dostring(state, script) != 0)
        finish(3);
    if (!lua_isinteger(state, -1) || lua_tointeger(state, -1) != 0)
        finish(4);
    finish(0);
}
