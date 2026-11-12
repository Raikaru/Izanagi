#include "p4pc/native_rows.h"

/* 00317900: fclCombineDrawEntryCell, retail; the mod card (entry 9) draws its symbol, labels and
 * base from the native-rows slots (include/p4pc/native_rows.h) and the selected test reads the tenth
 * (side) row through p4_native_rows_velvet_row. Entries 0..8 use exactly the retail slots. */
void fclCombineDrawEntryCell(u8 *task, FclVec2 startPosition, FclVec2 endPosition, s8 entry, s16 delay, s16 depth, s16 layer) {
    extern void fclSpriteStartAlphaAt(s16, u8, u8, u8, s32, s16);
    extern void fclSpriteStartPositionAt(s16, FclVec2, FclVec2, u32, u32, s16);
    FclDrawColor drawColor1;
    FclDrawColor drawColor2;
    FclDrawColor drawColor3;
    FclDrawColor drawColor4;
    FclDrawColor drawColor5;
    FclDrawColor drawColor6;
    FclDrawColor drawColor7;
    s16 sprite;
    s32 depthUp;
    s32 order;
    u8 *work = ((struct KwlnTask *)task)->work;
    FclVec2 pos = fclMakeVec2(startPosition.x, startPosition.y);
    u8 *node;
    s32 frame;
    s16 frameId;
    s32 fadeA;
    s32 fadeB;
    s32 fadeC;
    f32 scale;
    s32 orderUp;

    sprite = p4_native_rows_velvet_symbol(entry);
    order = layer;
    orderUp = order + 1;
    fclSpriteCreate(sprite, fclMakeVec2(6.0f + pos.x, 4.0f + pos.y), (f32)depth, orderUp);
    drawColor1 = fclMakeDrawColor(0x46, 0x6A, 0x82, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(sprite))->color = drawColor1;
    ((FclCombineDrawSprite *)fclSpriteEntry(sprite))->scaleX = 1.0f;
    ((FclCombineDrawSprite *)fclSpriteEntry(sprite))->scaleY = 1.0f;
    fclSpriteStartPositionAt(sprite, fclMakeVec2(6.0f + pos.x, 4.0f + pos.y), fclMakeVec2(6.0f + endPosition.x, 4.0f + endPosition.y), 0, 8, delay);
    frameId = frame = p4_native_rows_velvet_frame(entry);
    depthUp = depth + 1;
    fclSpriteCreate(frameId, endPosition, (f32)depthUp, order);
    fclSpriteCreate(p4_native_rows_velvet_small(entry), fclMakeVec2(6.0f + endPosition.x, 78.0f + endPosition.y), (f32)depthUp, orderUp);
    drawColor2 = fclMakeDrawColor(0x46, 0x6A, 0x82, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(p4_native_rows_velvet_small(entry)))->color = drawColor2;
    fclSpriteCreate(p4_native_rows_velvet_large(entry), fclMakeVec2(endPosition.x - 12.0f, 104.0f + endPosition.y), (f32)depth, order + 2);
    drawColor3 = fclMakeDrawColor(0xE0, 0xFF, 0x33, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(p4_native_rows_velvet_large(entry)))->color = drawColor3;
    if (entry == ((FclCombineRow *)p4_native_rows_velvet_row(work, ((FclCombineWork *)work)->commandCursor))->id) {
        fclSpriteCreate(sprite, fclMakeVec2(6.0f + pos.x, 4.0f + pos.y), 59.0f, 0xAB);
        scale = iGpffff8360;
        ((FclCombineDrawSprite *)fclSpriteEntry(sprite))->scaleX = scale;
        ((FclCombineDrawSprite *)fclSpriteEntry(sprite))->scaleY = scale;
        node = sdkSpriteNewNode(p4_native_rows_velvet_pack((u32)fclCombineGetSpritePool()), (s16)sprite);
        fclSpriteStartPositionAt(sprite, fclMakeVec2((5.0f + pos.x) - 2.0f * (sdkSpriteScaledWidth(node) / 10.0f), pos.y - 3.0f),
                      fclMakeVec2((5.0f + endPosition.x) - 2.0f * (sdkSpriteScaledWidth(node) / 10.0f), endPosition.y - 3.0f), 0, 8, delay);
        sdkSpriteFreeNode(node);
        drawColor4 = fclMakeDrawColor(0xE0, 0xFF, 0x33, 0xFF);
        ((FclCombineRowSprite *)fclSpriteEntry(sprite))->color = drawColor4;
        fclSpriteCreate(0x1DC, pos, 60.0f, 0xAA);
        fclSpriteStartPositionAt(0x1DC, pos, endPosition, 0, 8, delay);
        fclSpriteStartPositionAt(p4_native_rows_velvet_large(entry), fclMakeVec2(pos.x - 12.0f, 104.0f + pos.y), fclMakeVec2(endPosition.x - 12.0f, 104.0f + endPosition.y), 0, 8, delay);
        fclSpriteSetFlag(frame, 0, 1);
        if (entry < 6) {
            fclSpriteCreate(0xFC, fclMakeVec2(pos.x - 12.0f, 123.0f + pos.y), 59.0f, 0xAC);
            fclSpriteStartPositionAt(0xFC, fclMakeVec2(pos.x - 12.0f, 123.0f + pos.y), fclMakeVec2(endPosition.x - 12.0f, 123.0f + endPosition.y), 0, 8, delay);
            drawColor5 = fclMakeDrawColor(0xE0, 0xFF, 0x33, 0xFF);
            ((FclCombineRowSprite *)fclSpriteEntry(0xFC))->color = drawColor5;
        }
        fclSpriteSetFlag(p4_native_rows_velvet_small(entry), 0, 1);
        fclSpriteReset(0x216, 0x87);
        ((FclCombineRowSprite *)fclSpriteEntry(0x216))->flags = 0;
        fclSpriteCreate(0x216, fclMakeVec2(6.0f + pos.x, 62.0f + pos.y), 62.0f, 0xAA);
        fclSpriteStartPositionAt(0x216, fclMakeVec2(6.0f + pos.x, 62.0f + pos.y), fclMakeVec2(6.0f + endPosition.x, 62.0f + endPosition.y), 0, 8, delay);
        fadeA = delay + 8;
        fclSpriteStartAlphaAt(0x216, 0x40, 0x80, 1, 0x3C, fadeA);
        ((FclCombineRowSprite *)fclSpriteEntry(0x216))->flag73 = 1;
        fclSpriteSetFlag(0x216, 0xB, 0);
        fclSpriteReset(0x217, 0x87);
        ((FclCombineRowSprite *)fclSpriteEntry(0x217))->flags = 0;
        fclSpriteCreate(0x217, fclMakeVec2(33.0f + pos.x, 62.0f + pos.y), 62.0f, 0xAA);
        fclSpriteStartPositionAt(0x217, fclMakeVec2(33.0f + pos.x, 62.0f + pos.y), fclMakeVec2(33.0f + endPosition.x, 62.0f + endPosition.y), 0, 8, delay);
        fadeB = delay + 0x26;
        fclSpriteStartAlphaAt(0x217, 0x40, 0x80, 1, 0x3C, fadeB);
        ((FclCombineRowSprite *)fclSpriteEntry(0x217))->flag73 = 1;
        fclSpriteSetFlag(0x217, 0xB, 0);
        fclSpriteReset(0x218, 0x86);
        ((FclCombineRowSprite *)fclSpriteEntry(0x218))->flags = 0;
        fclSpriteCreate(0x218, fclMakeVec2(20.0f + pos.x, 62.0f + pos.y), 61.0f, 0xAA);
        fclSpriteStartPositionAt(0x218, fclMakeVec2(20.0f + pos.x, 62.0f + pos.y), fclMakeVec2(20.0f + endPosition.x, 62.0f + endPosition.y), 0, 8, delay);
        fadeC = delay + 0x17;
        fclSpriteStartAlphaAt(0x218, 0x40, 0x80, 1, 0x3C, fadeC);
        ((FclCombineRowSprite *)fclSpriteEntry(0x218))->flag73 = 1;
        fclSpriteSetFlag(0x218, 0xB, 0);
        fclSpriteReset(0x219, 0x89);
        ((FclCombineRowSprite *)fclSpriteEntry(0x219))->flags = 0;
        fclSpriteCreate(0x219, fclMakeVec2(6.0f + pos.x, pos.y - 44.0f), 62.0f, 0xAA);
        fclSpriteStartPositionAt(0x219, fclMakeVec2(6.0f + pos.x, pos.y - 44.0f), fclMakeVec2(6.0f + endPosition.x, endPosition.y - 44.0f), 0, 8, delay);
        fclSpriteStartAlphaAt(0x219, 0x80, 0xFF, 1, 0x3C, fadeA);
        ((FclCombineRowSprite *)fclSpriteEntry(0x219))->flag73 = 1;
        fclSpriteSetFlag(0x219, 0xB, 0);
        fclSpriteReset(0x21A, 0x89);
        ((FclCombineRowSprite *)fclSpriteEntry(0x21A))->flags = 0;
        fclSpriteCreate(0x21A, fclMakeVec2(33.0f + pos.x, pos.y - 44.0f), 62.0f, 0xAA);
        fclSpriteStartPositionAt(0x21A, fclMakeVec2(33.0f + pos.x, pos.y - 44.0f), fclMakeVec2(33.0f + endPosition.x, endPosition.y - 44.0f), 0, 8, delay);
        fclSpriteStartAlphaAt(0x21A, 0x80, 0xFF, 1, 0x3C, fadeB);
        ((FclCombineRowSprite *)fclSpriteEntry(0x21A))->flag73 = 1;
        fclSpriteSetFlag(0x21A, 0xB, 0);
        fclSpriteReset(0x21B, 0x88);
        ((FclCombineRowSprite *)fclSpriteEntry(0x21B))->flags = 0;
        fclSpriteCreate(0x21B, fclMakeVec2(20.0f + pos.x, pos.y - 44.0f), 61.0f, 0xAA);
        fclSpriteStartPositionAt(0x21B, fclMakeVec2(20.0f + pos.x, pos.y - 44.0f), fclMakeVec2(20.0f + endPosition.x, endPosition.y - 44.0f), 0, 8, delay);
        fclSpriteStartAlphaAt(0x21B, 0x80, 0xFF, 1, 0x3C, fadeC);
        ((FclCombineRowSprite *)fclSpriteEntry(0x21B))->flag73 = 1;
        fclSpriteSetFlag(0x21B, 0xB, 0);
        return;
    }
    fclSpriteStartPositionAt(p4_native_rows_velvet_frame(entry), pos, endPosition, 0, 8, delay);
    fclSpriteStartPositionAt(p4_native_rows_velvet_small(entry), fclMakeVec2(6.0f + pos.x, 78.0f + pos.y), fclMakeVec2(6.0f + endPosition.x, 78.0f + endPosition.y), 0, 8, delay);
    drawColor6 = fclMakeDrawColor(0x42, 0x6E, 0xFF, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(p4_native_rows_velvet_small(entry)))->color = drawColor6;
    fclSpriteSetFlag(p4_native_rows_velvet_large(entry), 0, 1);
    drawColor7 = fclMakeDrawColor(0x42, 0x6E, 0xFF, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(sprite))->color = drawColor7;
}
