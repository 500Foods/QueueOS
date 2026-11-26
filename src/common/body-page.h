#ifndef QUEUEOS_BODY_PAGE_H
#define QUEUEOS_BODY_PAGE_H

#include "disk.h"

#include <stdint.h>

/* The stored line without its newline, and a NUL.
 * The longest line is slot 10: 35 characters. */
#define QUEUEOS_BODY_LINE 36

/* One page at QUEUEOS_BODY_START.
 *
 * "stored STAMP slot N\n" then "sha256 " and 64 lowercase hex digits
 * of that stored line only, then a newline. The rest of the page is
 * zeros. The stamp is the twenty-character clock form Base already
 * writes. A later plan can replace this page with a table.
 *
 * queueos_body_read returns 1 and fills line when the page is that
 * record. slot, when not NULL, receives 1..10. It returns 0 when the
 * page is absent or refused, and -1 when page or line is NULL.
 * queueos_body_write returns 0, or -1 when the page pointer, the
 * stamp, or the slot is refused. A refusal leaves the caller's bytes
 * unchanged.
 */
int queueos_body_read(const uint8_t page[QUEUEOS_BODY_PAGE],
                      char line[QUEUEOS_BODY_LINE], int *slot);
int queueos_body_write(uint8_t page[QUEUEOS_BODY_PAGE],
                       const char *stamp, int slot);

#endif
