#include "p4pc/game_elements_ui_native.h"

void shdPersonaDrawAffinityPanel(Vec2f position, f32 depth, u8 alpha, u8 *persona, s32 *sprites)
{
    f32 baseY;
    Vec2f point;
    PackedColor4 color;
    u32 affinityIndex;
    s32 value;
    s32 panelSprites;
    s32 digitSprites;
    s32 affinitySprites;
    s32 icon;
    s32 entry;
    f32 rowY;
    f32 numberX;

    baseY = position.y;
    panelSprites = sprites[0];
    if (panelSprites == 0) {
        kAssertFail(D_005E4868, 0x298);
    }
    digitSprites = sprites[1];
    if (digitSprites == 0) {
        kAssertFail(D_005E4868, 0x29A);
    }
    affinitySprites = sprites[2];
    if (affinitySprites == 0) {
        kAssertFail(D_005E4868, 0x29C);
    }
    point.x = position.x;
    point.y = baseY;
    value = alpha & 0xFF;
    value = 0xFF - value;
    sdkSpriteDrawOnceTinted(0, panelSprites, 0x42, point.x, point.y, value, 0xFF, 0xA0, 0x0B, depth, 0);
    point.x = position.x - 273.0f;
    point.y = 3.0f + baseY;
    affinityIndex = 0;
    if (p4_game_elements_ui_native_count()) {
        /* 00115940 copies the source persona's canonical ID into this declared
         * field; the display block is NOT a DatPersonaRecord. A future wider
         * persona handle must migrate that builder, not reinterpret padding. */
        s32 result = p4_game_elements_ui_native_persona_affinities(
            (u32)((PersonaRenderRecord *)persona)->id,
            (const u32 *)((PersonaRenderRecord *)persona)->affinities,
            point.x, point.y, depth, alpha, (u32)panelSprites, (u32)affinitySprites);
        if (result != 0) p4_game_elements_ui_native_refuse(result);
    } else {
        /* No expanded element cohort: every retail operation and layout. */
        while (affinityIndex < 7U) {
            rowY = point.y;
            sdkSpriteDrawOnceTinted(0, affinitySprites, affinityIndex + 0x2E, point.x, rowY, value, 0xFF, 0xFF, 0x81, depth, 0);
            sdkSpriteDrawOnceTinted(0, panelSprites, 0x43, point.x - 2.0f, 22.0f + rowY, value, 0xFF, 0xE9, 0x2C, depth, 0);
            entry = ((PersonaRenderRecord *)persona)->affinities[affinityIndex];
            if ((entry & 0x02000000) != 0) {
                icon = 0x35;
            } else if ((entry & 0x08000000) != 0) {
                icon = 0x36;
            } else if ((entry & 0x10000000) != 0) {
                icon = 0x37;
            } else if ((entry & 0x01000000) != 0) {
                icon = 0x38;
            } else if ((entry & 0x04000000) != 0) {
                icon = 0x39;
            } else {
                icon = 0x3A;
            }
            sdkSpriteDrawOnceTinted(0, affinitySprites, icon, point.x, 24.0f + rowY, value, 0xDE, 0x75, 0, depth, 0);
            if (affinityIndex == 0) {
                point.x += 51.0f;
            } else {
                point.x += 37.0f;
            }
            affinityIndex++;
        }
    }
    color.rgba[0] = 0x2D;
    color.rgba[1] = 0x2D;
    color.rgba[2] = 0x2D;
    ((s8 *)color.rgba)[3] = (s8)alpha;
    point.x = position.x - 315.0f;
    point.y = 152.0f + baseY;
    value = (u32)alpha & 0xFFU;
    value = 0xFFU - value;
    sdkSpriteDrawOnceTinted(0, panelSprites, 0x44, point.x, point.y, value, 0xFF, 0xA0, 0x0B, depth, 0);
    point.x = position.x + 31.0f - 315.0f;
    rowY = 152.0f + (baseY - 93.0f);
    point.y = rowY;
    sdkSpriteDrawOnceTinted(0, affinitySprites, 0x2C, point.x, rowY, value, 0xFF, 0xFF, 0x81, depth, 0);
    sdkSpriteDrawOnceTinted(0, affinitySprites, 0x2D, point.x, 57.0f + rowY, value, 0xFF, 0xFF, 0x81, depth, 0);
    value = 0;
    numberX = position.x + 65.0f - 315.0f;
    baseY = 152.0f + (baseY - 88.0f);
    while (value < 5U) {
        entry = ((u8 *)((u32)persona + value))[7];
        point.x = numberX;
        rowY = (f32)(u32)(value * 0x13);
        point.y = baseY + rowY;
        shdPersonaDrawDigits(point, depth, color.packed, entry & 0xFF, digitSprites, 0);
        value++;
    }
}
