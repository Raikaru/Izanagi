#include "p4pc/game_text_native.h"
#include "p4pc/gmem.h"
#include "native_text_state.h"

extern void *frFontBuildTextFull(uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
extern int32_t itfMesListWidth(int32_t);
extern int32_t frFontListKeepOrRelease(int32_t);
extern void frFontSetGlyphAux(int32_t, uint8_t);
extern void frFontSetGlyphColor(int32_t, uint32_t);
extern void frFontExSetScale(int32_t, float);
extern void frFontSetListSlot(int32_t, uint8_t);
extern void frFontDrawListMode(void *, uint32_t);
extern uint8_t *PrimAllocQueueNode(void);
extern void PrimAppendNode(uint8_t *, uint8_t *);
extern void itfMesWindowDrawCallback(uint8_t *, int32_t);
extern int32_t frFontNodeWidth(void *);
extern int32_t frFontAppendList(int32_t, int32_t, int32_t);
extern void *itfMesWindowBuildEntry(float, float, float, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);


int32_t p4_game_tables_text_measure(uint32_t stream, uint32_t style, uint32_t multiline)
{
    /* Use the same active horizontal scale and style-dependent spacing as the
     * actual draw. Legacy 002787d0's temporary 123/128 scale is not a font
     * selection and would not match 00279030 when the active scale is zero. */
    FontState saved = save_font(multiline);
    void *list = frFontBuildTextFull(0, 0, 0, style & 255u, 0, 0, 255, stream, 0);
    int32_t guest_list = (int32_t)P4_H2G(list);
    int32_t width = itfMesListWidth(guest_list);
    frFontListKeepOrRelease(guest_list);
    restore_font(saved);
    return width;
}
static void *build(uint32_t stream, float x, float y, float scale,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags)
{
    void *list = frFontBuildTextFull((uint32_t)(int32_t)(16.0f * x),
        (uint32_t)(int32_t)(8.0f * y), 0, style & 255u, effect & 255u,
        0, 255, stream, 0);
    int32_t guest_list = (int32_t)P4_H2G(list);
    if (flags & 1u) frFontSetGlyphAux(guest_list, (uint8_t)effect);
    frFontSetGlyphColor(guest_list, color);
    frFontExSetScale(guest_list, scale);
    return list;
}
void p4_game_tables_text_draw(uint32_t stream, float x, float y, float scale,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline)
{
    FontState saved = save_font(multiline);
    void *list = build(stream, x, y, scale, color, style, effect, flags);
    frFontSetListSlot((int32_t)P4_H2G(list), (uint8_t)style);
    frFontDrawListMode(list, 1);
    frFontListKeepOrRelease((int32_t)P4_H2G(list));
    restore_font(saved);
}
void p4_game_tables_text_queue(uint32_t stream, float x, float y, float scale,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline, uint32_t queue)
{
    FontState saved = save_font(multiline);
    void *list = build(stream, x, y, scale, color, style, effect, flags);
    frFontSetListSlot((int32_t)P4_H2G(list), (uint8_t)style);
    uint8_t *node = PrimAllocQueueNode();
    uint32_t address = P4_H2G(node);
    /* Same 0x30-byte owner as 00279350: the callback draws/releases the newly
     * built glyph list. Validated command-free input leaves no source-stream
     * reference in FrFontTextRun/GlyphData; glyph-cache references are owned
     * by that list until the real primitive queue invokes 00279300. */
    P4_G32(address + 8u) = p4_fn_h2g_sig(itfMesWindowDrawCallback, "vpi");
    P4_G32(address + 16u) = P4_H2G(list);
    PrimAppendNode(P4_G2H(queue), node);
    restore_font(saved);
}
void p4_game_tables_text_draw_pair(uint32_t stream, float x, float y, float scale,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline,
    int32_t second_bank, int32_t second_record)
{
    FontState saved = save_font(multiline);
    void *list = build(stream, x, y, scale, color, style, effect, flags);
    if (second_record != 0) {
        /* 00279470 uses previous@24 to choose the second entry's x offset. */
        float second_x = x;
        if (P4_G32(P4_H2G(list) + 0x24u)) second_x += (float)frFontNodeWidth(list);
        void *second = itfMesWindowBuildEntry(second_x, 25.0f + y, scale,
            (int32_t)color, (int32_t)style, (int32_t)effect, (int32_t)flags, second_bank, second_record);
        int32_t combined = frFontAppendList((int32_t)P4_H2G(list), (int32_t)P4_H2G(second), 0);
        list = P4_G2H((uint32_t)combined);
    }
    frFontSetListSlot((int32_t)P4_H2G(list), (uint8_t)style);
    frFontDrawListMode(list, 1);
    frFontListKeepOrRelease((int32_t)P4_H2G(list));
    restore_font(saved);
}

typedef struct FontGeometryState { FontState font; uint32_t vertical; } FontGeometryState;
static FontGeometryState geometry_begin(uint32_t multiline, uint32_t horizontal, uint32_t vertical)
{
    FontGeometryState saved = {save_font(multiline), P4_G32(0x007645a4u)};
    P4_G32(0x007645a0u) = horizontal;
    P4_G32(0x007645a4u) = vertical;
    return saved;
}
static void geometry_end(FontGeometryState saved)
{
    P4_G32(0x007645a4u) = saved.vertical;
    restore_font(saved.font);
}
int32_t p4_game_tables_text_measure_geometry(uint32_t stream, uint32_t style,
    uint32_t multiline, uint32_t horizontal_128, uint32_t vertical_128)
{
    if (horizontal_128 > 128u || vertical_128 > 128u) return -1;
    if (!horizontal_128 || !vertical_128) return 0;
    FontGeometryState saved = geometry_begin(multiline, horizontal_128, vertical_128);
    void *list = frFontBuildTextFull(0, 0, 0, style & 255u, 0, 0, 255, stream, 0);
    int32_t guest_list = (int32_t)P4_H2G(list);
    int32_t width = itfMesListWidth(guest_list);
    frFontListKeepOrRelease(guest_list);
    geometry_end(saved);
    return width;
}
int32_t p4_game_tables_text_draw_geometry(uint32_t stream, float x, float y, float depth,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags, uint32_t multiline,
    uint32_t horizontal_128, uint32_t vertical_128)
{
    if (horizontal_128 > 128u || vertical_128 > 128u) return -1;
    if (!horizontal_128 || !vertical_128) return 0;
    FontGeometryState saved = geometry_begin(multiline, horizontal_128, vertical_128);
    void *list = build(stream, x, y, depth, color, style, effect, flags);
    frFontSetListSlot((int32_t)P4_H2G(list), (uint8_t)style);
    frFontDrawListMode(list, 1);
    frFontListKeepOrRelease((int32_t)P4_H2G(list));
    geometry_end(saved);
    return 0;
}
