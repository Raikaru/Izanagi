#ifndef P4PC_GOLDEN_MESSAGE_H
#define P4PC_GOLDEN_MESSAGE_H
#include <stddef.h>
#include <stdint.h>
/* Borrowed views: retain the immutable input for the lifetime of every view.
 * All functions return NULL on success, a stable named refusal on failure. */
typedef struct P4GoldenMessage {
    const uint8_t *bytes; size_t size;
    uint32_t dialogs, speakers, speaker_table;
} P4GoldenMessage;
typedef struct P4GoldenDialog {
    const P4GoldenMessage *message;
    const uint8_t *label; /* 24 bytes, not necessarily terminated */
    uint32_t kind, table, text, text_size;
    uint16_t count, speaker, pattern;
} P4GoldenDialog;
typedef enum P4GoldenTokenKind {
    P4_GOLDEN_END, P4_GOLDEN_GLYPH, P4_GOLDEN_NEWLINE, P4_GOLDEN_CONTROL
} P4GoldenTokenKind;
typedef struct P4GoldenToken {
    P4GoldenTokenKind kind;
    uint16_t code, glyph; /* encoded character and font index; not PS2 RAW */
    uint32_t scalar; /* ASCII scalar, zero when no Unicode claim is made */
    uint8_t group, index, argument_count;
    const uint8_t *arguments; /* little-endian u16 words, borrowed */
} P4GoldenToken;
typedef struct P4GoldenText { const uint8_t *bytes; size_t size, position; } P4GoldenText;
const char *p4_golden_message_open(const uint8_t *bytes, size_t size, P4GoldenMessage *out);
const char *p4_golden_message_dialog(const P4GoldenMessage *message, uint32_t index, P4GoldenDialog *out);
const char *p4_golden_message_page(const P4GoldenDialog *dialog, uint16_t page, P4GoldenText *out);
const char *p4_golden_message_speaker(const P4GoldenMessage *message, uint32_t index, P4GoldenText *out);
const char *p4_golden_text_next(P4GoldenText *text, P4GoldenToken *out);
#endif
