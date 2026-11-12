/* 002bc4b0: battle/item detail text uses a description, not the raw item name.
 * All retail bank selections, argument widths and casts remain unchanged. */
#include "p4pc/game_items_native.h"
void fclMesWindowDrawItemCategory(f32 inputDepth, s16 item, s32 x, s32 y,
                    s32 inputColor, s32 inputFont, s32 inputMode)
{
    extern s32 itfMesWindowDrawEntry(f32 x, f32 y, f32 scale, s32 color, s32 slotIndex, s32 effect, s32 flags, s32 handle, s32 recordIndex);
    u32 category;
    f32 depth;
    s32 color;
    s32 font;
    s32 mode;

    if (p4_game_tables_items_native_id_is_new((u32)item)) {
        p4_game_tables_text_draw(p4_game_tables_items_native_description((u32)item),
            (f32)x, (f32)y, inputDepth, (u32)inputColor, (u32)inputFont, (u32)inputMode, 1, 1);
        return;
    }
    depth = inputDepth;
    color = inputColor;
    font = inputFont;
    mode = inputMode;
    category = datEntryGetKind(item);
    switch (category) {
    case 0:
        itfMesWindowDrawEntry((f32)x, (f32)y, depth,
                      color, font, mode, 1, D_0063F2B0[0],
                      (s16)datItemBankOffset(item));
        return;
    case 1:
        itfMesWindowDrawEntry((f32)x, (f32)y, depth,
                      color, font, mode, 1, D_0063F2B0[1],
                      (s16)datItemBankOffset(item));
        return;
    case 2:
        itfMesWindowDrawEntry((f32)x, (f32)y, depth,
                      color, font, mode, 1, D_0063F2B0[2],
                      (s16)datItemBankOffset(item));
        return;
    case 3:
        itfMesWindowDrawEntry((f32)x, (f32)y, depth,
                      color, font, mode, 1, D_0063F2B0[4],
                      (s16)datItemBankOffset(item));
        return;
    case 4:
        itfMesWindowDrawEntry((f32)x, (f32)y, depth,
                      color, font, mode, 1, D_0063F2B0[5],
                      (s16)datItemBankOffset(item));
        return;
    case 5:
        itfMesWindowDrawEntry((f32)x, (f32)y, depth,
                      color, font, mode, 1, D_0063F2B0[7],
                      (s16)datItemBankOffset(item));
    default:
        return;
    }
}
