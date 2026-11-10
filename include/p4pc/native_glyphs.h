/* Shared glyph-only validation; no table registry, Unicode codec or renderer. */
#ifndef P4PC_NATIVE_GLYPHS_H
#define P4PC_NATIVE_GLYPHS_H
#include <stddef.h>
#include <stdint.h>
typedef enum P4NativeGlyphFlavor {
    P4_GLYPHS_MESSAGE = 1, /* BuildTextFull: 0xc080+ selects secondary style. */
    P4_GLYPHS_RAW = 2      /* CreateTextList/ExCreateText: plain glyph codes. */
} P4NativeGlyphFlavor;
/* 0 valid, -1 invalid. size includes the final NUL; interior NULs and native
 * commands are refused. RAW never accepts newline or MSG alternate-style codes.
 * MESSAGE accepts newline only when multiline is nonzero. A command-looking
 * trail byte (such as 0xf6 in the encoded space 0x83f6) is still a glyph byte.
 * This validates syntax, not asset glyph coverage or Unicode correspondence. */
int p4_native_glyphs_validate(const uint8_t *bytes, size_t size,
    P4NativeGlyphFlavor flavor, int multiline);
/* Measure an already validated, guest-backed RAW line using the exact Config
 * ExCreateText(flags=1) recipe, then release without drawing. style is its font
 * argument (Config uses 1), palette is its chr argument (rows 8/help 6).
 * Returns native whole-pixel widthOut *16, or -1 on allocation/range failure.
 * Active horizontal scale/kerning is preserved, never replaced by a guess. */
int32_t p4_native_glyphs_measure_raw(const uint8_t *guest_bytes, int32_t style, uint8_t palette);
#endif
