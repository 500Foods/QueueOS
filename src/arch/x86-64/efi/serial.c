/* 16550 at I/O port 0x3F8, 115200 8N1. That is QEMU's default UART.
 * Firmware's ConOut path is CHAR16 and does not write these bytes.
 * The harness reads serial.log from this port.
 */

#include "serial.h"

#include <stdint.h>

#define QUEUEOS_COM1 0x3F8u
#define QUEUEOS_LSR_THRE 0x20u

static inline void queueos_outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t queueos_inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void queueos_serial_init(void)
{
    queueos_outb(QUEUEOS_COM1 + 1, 0x00); /* Interrupt enable off. */
    queueos_outb(QUEUEOS_COM1 + 3, 0x80); /* Divisor latch. */
    queueos_outb(QUEUEOS_COM1 + 0, 0x01); /* 115200 / 115200. */
    queueos_outb(QUEUEOS_COM1 + 1, 0x00);
    queueos_outb(QUEUEOS_COM1 + 3, 0x03); /* 8 bits, no parity, one stop. */
    queueos_outb(QUEUEOS_COM1 + 2, 0x07); /* Enable and clear the FIFO. */
    queueos_outb(QUEUEOS_COM1 + 4, 0x03); /* DTR and RTS. Polled transmit only. */
}

static void queueos_serial_write_byte(uint8_t value)
{
    while ((queueos_inb(QUEUEOS_COM1 + 5) & QUEUEOS_LSR_THRE) == 0) {
    }
    queueos_outb(QUEUEOS_COM1 + 0, value);
}

void queueos_serial_write_line(const char *text)
{
    const char *cursor;

    if (text == 0) {
        return;
    }
    for (cursor = text; *cursor != '\0'; cursor++) {
        queueos_serial_write_byte((uint8_t)*cursor);
    }
    queueos_serial_write_byte('\r');
    queueos_serial_write_byte('\n');
}
