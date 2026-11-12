#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"

/* 00189940..00189e8c: original field HUD update, verified against the ELF.
 * The only addition submits a stock frame/font after the native HUD; its helper
 * returns without allocations, guest writes or draws unless touch is active.
 * Preserve the hidden-panel early return, all ten-frame slides and text slots. */
s32 FldPanel2dUpdate(u8 *task)
{
    FldPanel2dWork *panelWork;
    u8 *work;
    u8 *primitive;
    f32 depth;
    f32 reciprocalDepth;
    f32 top;
    f32 progress;
    s32 i;

    work = ((KwlnTask *)task)->work;
    panelWork = (FldPanel2dWork *)work;
    if (((FldPanel2dWork *)work)->hidden == 1) {
        return 0;
    }

    switch (((FldPanel2dWork *)work)->state) {
    case 0:
        depth = D_008872F8[0] - (f32)58980;
        reciprocalDepth = 1.0f / depth;
        ((FldPanel2dWork *)work)->side = 0;
        panelWork->slotX[panelWork->side] = 175.0f;
        ((FldPanel2dWork *)work)->vertices[0].x = 0.0f;
        top = (f32)401;
        ((FldPanel2dWork *)work)->vertices[0].y = top;
        ((FldPanel2dWork *)work)->vertices[0].z = depth;
        ((FldPanel2dWork *)work)->vertices[1].x = 25.0f;
        ((FldPanel2dWork *)work)->vertices[1].y = top;
        ((FldPanel2dWork *)work)->vertices[1].z = depth;
        ((FldPanel2dWork *)work)->vertices[2].x = 0.0f;
        progress = (f32)427;
        ((FldPanel2dWork *)work)->vertices[2].y = progress;
        ((FldPanel2dWork *)work)->vertices[2].z = depth;
        ((FldPanel2dWork *)work)->vertices[3].x = 25.0f;
        ((FldPanel2dWork *)work)->vertices[3].y = progress;
        ((FldPanel2dWork *)work)->vertices[3].z = depth;
        ((FldPanel2dWork *)work)->vertices[4].x = 326.0f;
        ((FldPanel2dWork *)work)->vertices[4].y = top;
        ((FldPanel2dWork *)work)->vertices[4].z = depth;
        ((FldPanel2dWork *)work)->vertices[5].x = 660.0f;
        ((FldPanel2dWork *)work)->vertices[5].y = top;
        ((FldPanel2dWork *)work)->vertices[5].z = depth;
        ((FldPanel2dWork *)work)->vertices[6].x = 326.0f;
        ((FldPanel2dWork *)work)->vertices[6].y = progress;
        ((FldPanel2dWork *)work)->vertices[6].z = depth;
        ((FldPanel2dWork *)work)->vertices[7].x = 660.0f;
        ((FldPanel2dWork *)work)->vertices[7].y = progress;
        ((FldPanel2dWork *)work)->vertices[7].z = depth;
        for (i = 0; i < 4; i++) {
            primitive = work + (i << 6);
            ((FldPanel2dWork *)primitive)->vertices[0].reciprocalZ = reciprocalDepth;
            ((FldPanel2dWork *)primitive)->vertices[4].reciprocalZ = reciprocalDepth;
            ((FldPanel2dWork *)primitive)->vertices[0].red = 255.0f;
            ((FldPanel2dWork *)primitive)->vertices[0].green = 0.0f;
            ((FldPanel2dWork *)primitive)->vertices[0].blue = 0.0f;
            ((FldPanel2dWork *)primitive)->vertices[0].alpha = 0.0f;
            ((FldPanel2dWork *)primitive)->vertices[4].red = 255.0f;
            ((FldPanel2dWork *)primitive)->vertices[4].green = 0.0f;
            ((FldPanel2dWork *)primitive)->vertices[4].blue = 0.0f;
            ((FldPanel2dWork *)primitive)->vertices[4].alpha = 0.0f;
        }
        ((FldPanel2dWork *)work)->state += 1;
        break;
    case 1:
        ((FldPanel2dWork *)work)->alpha = sinf((D_00761184 * (f32)((FldPanel2dWork *)work)->animFrame) / 10.0f);
        if (panelWork->animFrame < 10) {
            panelWork->animFrame += 1;
        } else {
            ((FldPanel2dWork *)work)->state += 1;
        }
        ((FldPanel2dWork *)work)->posX = (-300.0f + 0.0f) + 316.0f * panelWork->alpha;
        ((FldPanel2dWork *)work)->posY = ((f32)371 + 0.0f) + 30.0f * panelWork->alpha;
        /* fallthrough */
    case 2:
        sflDataDrawNarrowRoundedFrame((s32)((FldPanel2dWork *)work)->posX, (s32)((FldPanel2dWork *)work)->posY, (f32)59000, 301, 26, 0xFAFF20, 255, 1, 150, 13, D_00794C60, -5.0f, 1.0f, 1.0f);
        sflDataDrawNarrowRoundedFrame(25, 401, (f32)59000, 301, 26, 0x191919, 255, 1, 150, 13, D_00794C60, 0.0f, 1.0f, ((FldPanel2dWork *)work)->alpha);
        if (!(panelWork->alpha < 1.0f)) {
            if (panelWork->textHandles[!panelWork->side] != 0) {
                progress = sinf((D_00761184 * (f32)((FldPanel2dWork *)work)->blinkTimer) / 10.0f);
                panelWork->slotX[panelWork->side] = 175.0f + 336.0f * progress;
                panelWork->slotX[!panelWork->side] = -161.0f + 336.0f * progress;
                if (panelWork->blinkTimer < 10) {
                    panelWork->blinkTimer += 1;
                } else {
                    panelWork->textHandles[panelWork->side] = 0;
                    ((FldPanel2dWork *)work)->side = !(u32)((FldPanel2dWork *)work)->side;
                }
            }
            primitive = PrimQueueDrawPrimitive(D_00794C60, 4, work + 0x20, 4);
            ((PrimQueueNode *)primitive)->callback = FldSkyApplyDefaultPreset;
            ((PrimQueueNode *)primitive)->argument.handle = 0;
            PrimQueueDrawPrimitive(D_00794C60, 4, work + 0x120, 4);
            if (((FldPanel2dWork *)work)->text[0] != NULL) {
                frFontExDrawTextDeferredWithCharWidth((f32)(s32)((FldPanel2dWork *)work)->slotX[0], (f32)399, (f32)58990, -1, 0, 1, ((FldPanel2dWork *)work)->text[0], 8, 0, D_00794C60, -1);
            }
            if (((FldPanel2dWork *)work)->text[1] != NULL) {
                frFontExDrawTextDeferredWithCharWidth((f32)(s32)((FldPanel2dWork *)work)->slotX[1], (f32)399, (f32)58990, -1, 0, 1, ((FldPanel2dWork *)work)->text[1], 8, 0, D_00794C60, -1);
            }
        }
        break;
    case 3:
    default:
        break;
    }
    p4_settings_native_corner();
    return 0;
}
