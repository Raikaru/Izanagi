#include "p4pc/native_glyphs.h"
#include "native_text_state.h"
extern uint32_t frFontExCreateText(float, float, float, int32_t, int8_t, int32_t, const char *, int32_t, int32_t *);
extern uint32_t frFontListRelease(uint32_t);
int32_t p4_native_glyphs_measure_raw(const uint8_t *guest_bytes, int32_t style, uint8_t palette)
{
    FontState saved = save_font(0);
    int32_t width = 0;
    uint32_t list = frFontExCreateText(0, 0, 0, 0, (int8_t)palette, style, (const char *)guest_bytes, 1, &width);
    if (list) frFontListRelease(list);
    restore_font(saved);
    if (!list || width < 0 || width > INT32_MAX / 16) return -1;
    return width * 16;
}
