#include "p4pc/native_glyphs.h"

int p4_native_glyphs_validate(const uint8_t *bytes, size_t size, P4NativeGlyphFlavor flavor, int multiline)
{
    if (!bytes || !size || size > 65536u || bytes[size - 1] ||
        (flavor != P4_GLYPHS_MESSAGE && flavor != P4_GLYPHS_RAW)) return -1;
    for (size_t i = 0; i + 1 < size; ++i) {
        uint8_t c = bytes[i];
        if (c >= 0x20 && c < 0x7f) continue;
        if (c == 10 && multiline && flavor == P4_GLYPHS_MESSAGE) continue;
        if (c < 0x80 || c >= 0xf0 || i + 2 >= size || bytes[i + 1] < 0x80) return -1;
        if (flavor == P4_GLYPHS_RAW && c >= 0xc0) return -1;
        ++i;
    }
    return 0;
}
