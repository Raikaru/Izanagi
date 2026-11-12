#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"

/* 0035dfb0..0035e69c: the real five-frame ON/OFF slash/masked black pill.
 * Keep its sin/rotation, destination-alpha tag pairs, palettes and GS restores.
 * Only the logical value/timer lookup changes for expanded lists: physical row
 * six must never alias options[6] onto previousOptions[0], nor may scrolling
 * overwrite the six retail option values. Retail-only lookups remain original. */
void cmpConfigDrawOnOff(u8 *work, s32 optionIndex, s32 drawMode)
{
    u8 color[4];
    f32 opacity;
    f32 baseX;
    f32 baseY;
    f32 x;
    f32 y;
    f32 phase;
    f32 angle;
    f32 rowY;
    f32 rowOpacity;
    u8 arrowAlpha;
    u8 otherColor;
    u8 *sprite;
    u8 alpha;
    u8 *row;
    u16 counter;
    P4SettingsNativeState *pc = (P4SettingsNativeState *)p4_settings_native_state((u32)work, 0);
    u32 counterAddress;
    u32 rowAddress;
    f32 *rowXField;
    f32 *rowYField;

    baseX = ((CmpConfigWork *)work)->origin.x;
    baseY = ((CmpConfigWork *)work)->origin.y;
    opacity = (f32)*work / 255.0f;
    if (drawMode != 1) {
        rowAddress = optionIndex * 0x30;
        rowAddress += (u32)work;
        row = (u8 *)rowAddress;
        x = 60.0f + (464.0f + (baseX + ((CmpConfigWork *)row)->motion[8].position.x));
        y = (119.0f + (baseY + ((CmpConfigWork *)row)->motion[8].position.y)) + 32.0f * (f32)optionIndex;
        arrowAlpha = (u8)((f32)((CmpConfigWork *)row)->motion[8].opacity * opacity);
        sprite = ((CmpConfigWork *)work)->sprites[5];
        counterAddress = optionIndex * 2;
        counterAddress += (u32)work;
        counter = pc ? *(u16 *)pc->rows[optionIndex].timer
            : ((CmpConfigWork *)counterAddress)->timers[0];
        if ((s32)counter < 5)
            phase = sinf((iGpffff8094 * (f32)counter) / 5.0f);
        else
            phase = 1.0f;
        if ((pc ? pc->rows[optionIndex].boolean : ((s32 *)(work + 0x48))[optionIndex]) != 0)
            angle = (90.0f + 180.0f * phase) - 180.0f;
        else
            angle = (-90.0f + 180.0f * phase) - 180.0f;
        sdkSpriteDrawSized(sprite, x, y, 0.0f, 0xFF, 0xFF, 0xFF, arrowAlpha,
                      0x1000, 0x1000, 0x11, angle, 0x11);
    }
    if (drawMode == 2) {
        mdlEffectEmitMaskedTagPair();
        D_00887300[0](1, 0);
        color[0] = 0;
        color[1] = 0;
        color[2] = 0;
        color[3] = 0;
        effMiscDrawScreenColor(color, 0);
        counterAddress = optionIndex * 2;
        counterAddress += (u32)work;
        counter = pc ? *(u16 *)pc->rows[optionIndex].timer
            : ((CmpConfigWork *)counterAddress)->timers[0];
        if ((s32)counter < 5)
            phase = sinf((iGpffff8094 * (f32)counter) / 5.0f);
        else
            phase = 1.0f;
        if ((pc ? pc->rows[optionIndex].boolean : ((s32 *)(work + 0x48))[optionIndex]) != 0)
            angle = (90.0f + 180.0f * phase) - 180.0f;
        else
            angle = (-90.0f + 180.0f * phase) - 180.0f;
        sdkSpriteDrawSized(sprite, x, y, 0.0f, 0xFF, 0xFF, 0xFF, 0xFF,
                      0x1000, 0x1000, 0x11, angle, 0x11);
        mdlEffectEmitTagPair();
        RpSkyRenderStateSet(3, 0x2D801);
        RpSkyRenderStateSet(2, 0x44);
    }
    rowOpacity = (f32)work[optionIndex * 0x30 + 0xC2];
    rowOpacity *= opacity;
    alpha = (u8)rowOpacity;
    if (drawMode == 1) {
        color[0] = 0x80; color[1] = 0x80; color[2] = 0x80; otherColor = 0x80;
    } else if (drawMode == 2) {
        color[0] = 0xFF; color[1] = 0xFF; color[2] = 0xFF; otherColor = 0xFF;
    } else {
        counterAddress = optionIndex * 4;
        counterAddress += (u32)work;
        if ((pc ? pc->rows[optionIndex].boolean : ((CmpConfigWork *)counterAddress)->options[0]) != 0) {
            color[0] = 0xFF; color[1] = 0xFF; color[2] = 0xFF; otherColor = 0x80;
        } else {
            color[0] = 0x80; color[1] = 0x80; color[2] = 0x80; otherColor = 0xFF;
        }
    }
    rowXField = &((f32 *)(work + 0x208))[optionIndex * 12];
    x = (f32)0x1DD;
    x = x + (baseX + *rowXField);
    rowYField = (f32 *)(optionIndex * 0x30 + work + 0x20C);
    rowY = 32.0f * (f32)optionIndex;
    sdkSpriteDrawAt(((CmpConfigWork *)work)->sprites[3], x,
                  rowY + (121.0f + (baseY + *rowYField)),
                  color[0], color[1], color[2], alpha);
    sdkSpriteDrawAt(((CmpConfigWork *)work)->sprites[4],
                  556.0f + (baseX + *rowXField),
                  rowY + (131.0f + (baseY + *rowYField)),
                  otherColor, otherColor, otherColor, alpha);
    if (drawMode == 2) {
        RpSkyRenderStateSet(3, 0x717FB);
        RpSkyRenderStateSet(2, 0x44);
    }
}