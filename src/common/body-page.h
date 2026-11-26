#ifndef QUEUEOS_BODY_PAGE_H
#define QUEUEOS_BODY_PAGE_H

#include "disk.h"

#include <stdint.h>

/* The stored sentence without its newline, and a NUL.
 * The longest sentence is slot 10: 35 characters. */
#define QUEUEOS_BODY_LINE 36

/* One page at QUEUEOS_BODY_START.
 *
 * A JSON array of {"stored":"STAMP","slot":N} objects, then "sha256 "
 * and 64 lowercase hex digits of those JSON bytes only, then a newline.
 * The rest of the page is zeros. The writer emits that compact order.
 * Jansson reads the array. A spaced or reordered object is still a ring
 * when the hash matches. The next write rewrites the canonical text.
 *
 * queueos_body_read returns 1 and fills line with the newest sentence,
 * "stored STAMP slot N", when the ring has a row. slot, when not NULL,
 * receives that row's 1..10. It returns 0 when the page is absent, empty,
 * or refused, and leaves slot alone. It returns -1 when page or line is
 * NULL. queueos_body_write appends one row, or replaces a refused page
 * with a one-element array. It returns 0, or -1 when the page pointer,
 * the stamp, or the slot is refused. A refusal leaves the caller's bytes
 * unchanged.
 */
int queueos_body_read(const uint8_t page[QUEUEOS_BODY_PAGE],
                      char line[QUEUEOS_BODY_LINE], int *slot);
int queueos_body_write(uint8_t page[QUEUEOS_BODY_PAGE],
                       const char *stamp, int slot);

#endif
