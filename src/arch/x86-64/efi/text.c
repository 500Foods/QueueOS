/* Screen path for the stubs.
 *
 * ClearScreen fills with the background that is already selected. The
 * attribute is set before the clear so the frame is red, which is what
 * the screenshot check samples. The plan's sentence order is unchanged:
 * the sentence is printed after the clear.
 *
 * Diagnostic lines are serial only. They are not drawn.
 */

#include "text.h"

#include "serial.h"

enum { QUEUEOS_TEXT_MAX = 96 };

static void queueos_to_char16(const char *text, QUEUEOS_CHAR16 *out, QUEUEOS_UINTN cap)
{
    QUEUEOS_UINTN i;

    if (cap == 0) {
        return;
    }
    i = 0;
    if (text != 0) {
        while (text[i] != '\0' && (i + 1) < cap) {
            out[i] = (QUEUEOS_CHAR16)(uint8_t)text[i];
            i++;
        }
    }
    out[i] = 0;
}

static void queueos_screen_write(QUEUEOS_EFI_SYSTEM_TABLE *system, const char *sentence)
{
    QUEUEOS_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out;
    QUEUEOS_CHAR16 wide[QUEUEOS_TEXT_MAX];

    if (system == 0 || system->con_out == 0) {
        return;
    }
    out = system->con_out;
    if (out->set_attribute == 0 || out->clear_screen == 0 || out->output_string == 0) {
        return;
    }
    out->set_attribute(out, QUEUEOS_TEXT_WHITE_ON_RED);
    out->clear_screen(out);
    queueos_to_char16(sentence, wide, QUEUEOS_TEXT_MAX);
    out->output_string(out, wide);
}

void queueos_report(QUEUEOS_EFI_SYSTEM_TABLE *system, const char *diagnostic, const char *sentence)
{
    queueos_serial_init();
    queueos_serial_write_line(diagnostic);
    queueos_screen_write(system, sentence);
    queueos_serial_write_line(sentence);
}
