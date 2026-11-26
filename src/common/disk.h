#ifndef QUEUEOS_DISK_H
#define QUEUEOS_DISK_H

/* Database disk. There is no filesystem.
 *
 * Ten slots of 4 KiB start at byte 0. The boot range follows them and
 * holds Base and, later, pictures. The database body starts at
 * QUEUEOS_BODY_START. That number lives only in this file so the body
 * can move without a second copy drifting through readers, tests, or
 * sample JSON. It is not a format version. A slot names its own
 * offsets. Boot trusts those offsets and rejects one that lands in the
 * slots or past the body.
 */

#define QUEUEOS_SLOT_BYTES 4096u
#define QUEUEOS_SLOT_COUNT 10
#define QUEUEOS_SLOT_REGION (QUEUEOS_SLOT_BYTES * QUEUEOS_SLOT_COUNT)

/* 8 MiB from the start of the disk. */
#define QUEUEOS_BODY_START (8u * 1024u * 1024u)

#endif
