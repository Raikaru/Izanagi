#include "p4pc/native_rows.h"

/* 00318840: fclCombineUpdateEntryHighlight, retail; entry 9 (the mod card) uses the native-rows
 * slots and the extended pack for its symbol width. Entries 0..8 are exactly retail. */
void fclCombineUpdateEntryHighlight(u8 *task, s8 row, s8 entry, s16 depth, s16 layer) {
    FclDrawColor drawColor1;
    FclDrawColor drawColor2;
    FclDrawColor drawColor3;
    FclDrawColor drawColor4;
    FclVec2 pos;
    u8 *work;
    u8 *h;
    s32 resource;
    s16 icon;
    f32 scale;

    work = ((struct KwlnTask *)task)->work;
    resource = p4_native_rows_velvet_frame(entry);
    pos = ((FclCombineRowSprite *)fclSpriteEntry(resource))->position;
    icon = p4_native_rows_velvet_symbol(entry);
    if (row == ((FclCombineWork *)work)->commandCursor) {
        ((FclCombineRowSprite *)fclSpriteEntry(0x1DC))->position = pos;
        ((FclCombineRowSprite *)fclSpriteEntry(0x216))->position = fclMakeVec2(6.0f + pos.x, 62.0f + pos.y);
        ((FclCombineRowSprite *)fclSpriteEntry(0x217))->position = fclMakeVec2(33.0f + pos.x, 62.0f + pos.y);
        ((FclCombineRowSprite *)fclSpriteEntry(0x218))->position = fclMakeVec2(20.0f + pos.x, 62.0f + pos.y);
        ((FclCombineRowSprite *)fclSpriteEntry(0x219))->position = fclMakeVec2(6.0f + pos.x, pos.y - 44.0f);
        ((FclCombineRowSprite *)fclSpriteEntry(0x21A))->position = fclMakeVec2(33.0f + pos.x, pos.y - 44.0f);
        ((FclCombineRowSprite *)fclSpriteEntry(0x21B))->position = fclMakeVec2(20.0f + pos.x, pos.y - 44.0f);
        scale = iGpffff8360;
        ((FclCombineDrawSprite *)fclSpriteEntry(icon))->scaleX = scale;
        ((FclCombineDrawSprite *)fclSpriteEntry(icon))->scaleY = scale;
        ((FclCombineRowSprite *)fclSpriteEntry(icon))->depth = 59.0f;
        ((FclCombineRowSprite *)fclSpriteEntry(icon))->order = 0xAB;
        h = sdkSpriteNewNode(p4_native_rows_velvet_pack((u32)fclCombineGetSpritePool()), icon);
        ((FclCombineRowSprite *)fclSpriteEntry(icon))->position = fclMakeVec2((5.0f + pos.x) - 2.0f * (sdkSpriteScaledWidth(h) / 10.0f), pos.y - 3.0f);
        sdkSpriteFreeNode(h);
        drawColor1 = fclMakeDrawColor(0xE0, 0xFF, 0x33, 0xFF);
        ((FclCombineRowSprite *)fclSpriteEntry(icon))->color = drawColor1;
        fclSpriteSetFlag(p4_native_rows_velvet_frame(entry), 0, 1);
        fclSpriteSetFlag(p4_native_rows_velvet_small(entry), 0, 1);
        fclSpriteCreate((s16)(p4_native_rows_velvet_large(entry)), fclMakeVec2(pos.x - 12.0f, 104.0f + pos.y), 59.0f, 0xAC);
        if (entry < 6) {
            fclSpriteCreate(0xFC, fclMakeVec2(pos.x - 12.0f, 123.0f + pos.y), 59.0f, 0xAC);
            drawColor2 = fclMakeDrawColor(0xE0, 0xFF, 0x33, 0xFF);
            ((FclCombineRowSprite *)fclSpriteEntry(0xFC))->color = drawColor2;
            return;
        }
        fclSpriteSetFlag(0xFC, 0, 1);
        return;
    }
    fclSpriteSetFlag(p4_native_rows_velvet_large(entry), 0, 1);
    fclSpriteSetFlag(resource, 0, 0);
    resource = p4_native_rows_velvet_small(entry);
    fclSpriteSetFlag(resource, 0, 0);
    drawColor3 = fclMakeDrawColor(0x42, 0x6E, 0xFF, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(resource))->color = drawColor3;
    ((FclCombineDrawSprite *)fclSpriteEntry(icon))->scaleX = 1.0f;
    ((FclCombineDrawSprite *)fclSpriteEntry(icon))->scaleY = 1.0f;
    ((FclCombineRowSprite *)fclSpriteEntry(icon))->depth = depth;
    ((FclCombineRowSprite *)fclSpriteEntry(icon))->order = layer + 1;
    ((FclCombineRowSprite *)fclSpriteEntry(icon))->position = fclMakeVec2(6.0f + pos.x, 4.0f + pos.y);
    drawColor4 = fclMakeDrawColor(0x42, 0x6E, 0xFF, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(icon))->color = drawColor4;
}
