#ifndef P4PC_GOLDEN_FONT_H
#define P4PC_GOLDEN_FONT_H
#include "p4pc/golden_message.h"
#define P4_GOLDEN_FONT_CELL_MAX 64u
#define P4_GOLDEN_FONT_BITMAP_MAX 2048u
typedef struct P4GoldenFont {
    const uint8_t *bytes, *palette, *metrics, *tree, *positions, *bits;
    size_t size, tree_count, bit_bytes;
    uint16_t count, width, height, packed_size;
} P4GoldenFont;
const char *p4_golden_font_open(const uint8_t *bytes, size_t size, P4GoldenFont *out);
/* Decode exactly one packed 4-bit cell. Caller owns scratch, no allocation. */
const char *p4_golden_font_glyph(const P4GoldenFont *font, uint16_t glyph, uint8_t *packed, size_t capacity);
const char *p4_golden_font_metrics(const P4GoldenFont *font, uint16_t glyph, int *left, int *right);
/* Static presentation seam. The callback must explicitly handle each control:
 * return NULL for an intentionally presentation-inert code, or named refusal.
 * It can update the pen/tint and draw substitutions via this same seam. */
typedef struct P4GoldenTextPen { int x, y; uint8_t rgba[4]; } P4GoldenTextPen;
typedef const char *(*P4GoldenControlFn)(void *context, const P4GoldenToken *token, P4GoldenTextPen *pen);
const char *p4_golden_message_render(const P4GoldenFont *font, P4GoldenText text,
    uint8_t *rgba, uint32_t width, uint32_t height, size_t capacity,
    P4GoldenTextPen *pen, P4GoldenControlFn control, void *context);
#endif
