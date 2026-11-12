#include "p4pc/native_rows.h"

/* 00318f30: fclCombineMarkRow, retail; row 9 (the mod card) marks its own symbol pair. */
void fclCombineMarkRow(s16 row) {
    s16 sprite;
    u8 *p;
    f32 fv;
    FclVec2 drawPosition1;
    FclDrawColor drawColor1;
    FclVec2 drawPosition2;

    sprite = p4_native_rows_velvet_symbol(row);
    fclSpriteReset(0x2F2, (s16)(sprite + 1));
    p = fclSpriteEntry(sprite);
    drawPosition1 = ((FclCombineRowSprite *)p)->position;
    drawPosition2 = fclMakeVec2(drawPosition1.x - 15.0f, drawPosition1.y - 7.0f);
    fclSpriteCreate(0x2F2, drawPosition2, 58.0f, 0xAB);
    fclSpriteStartAlphaAt(0x2F2, 0xFF, 0, 0, 0xF, 0);
    fv = iGpffff8360;
    ((FclCombineDrawSprite *)fclSpriteEntry(0x2F2))->scaleX = fv;
    ((FclCombineDrawSprite *)fclSpriteEntry(0x2F2))->scaleY = fv;
    drawColor1 = fclMakeDrawColor(0xE0, 0xFF, 0x33, 0xFF);
    ((FclCombineRowSprite *)fclSpriteEntry(0x2F2))->color = drawColor1;
}
