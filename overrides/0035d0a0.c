#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"

/* 0035d0a0..0035dcbc, retail Config draw verified against the ELF.
 * Keep the actual row bars/right-aligned labels, selected pill, ON/OFF widget,
 * equalizer, help and button-glyph footer. PC values reuse Config's OK-value
 * pill and native font plus System's arrow/scrollbar sprites; no GLES canvas.
 * The retail-only path retains its seven labels, timings, palettes and calls. */
void cmpConfigDraw(u8 *work) {
    extern int frFontExDrawText(f32 x, f32 y, f32 scale, int color, s8 chr, int id, const char *str, int flags,
                             int unused);
    extern int frFontExDrawTextWithCharWidth(f32 x, f32 y, f32 scale, int color, s8 chr, int id, const char *str, int flags,
                             int charWidth);
    CmpConfigWork *menu;
    P4SettingsNativeState *pc;
    s32 selectedOption;
    s32 backdropMode;
    s32 textColor;
    s32 index;
    s32 cellIndex;
    s32 pendingOption;
    s32 selected;
    u8 *sprite;
    u8 *buttonColor;
    u8 rowBlue;
    CmpConfigColorSnapshot color;
    u8 alpha;
    Vec2f pos;
    f32 fade;
    f32 baseX;
    f32 baseY;
    f32 graphX;
    f32 graphY;
    f32 topAlpha;
    f32 filledAlpha;

    pc = (P4SettingsNativeState *)p4_settings_native_state((u32)work, 1);
    menu = (CmpConfigWork *)work;
    sdkRenderStateSetDefault();
    baseX = menu->origin.x;
    baseY = menu->origin.y;
    fade = (f32)menu->opacity / 255.0f;
    selectedOption = menu->selectedOption;
    backdropMode = menu->backdropMode;
    if (backdropMode != 0) {
        pos.x = baseX;
        pos.y = baseY;
        alpha = (u8)(255.0f * fade);
        nLineDrawSet(pos, alpha, backdropMode, 0.0f);
    }
    if ((menu->visible & 1) != 0) {
        pos.x = 26.0f + (baseX + menu->motion[0].position.x);
        pos.y = 388.0f + (baseY + menu->motion[0].position.y);
        alpha = (u8)((f32)menu->motion[0].opacity * fade);
        sprite = menu->sprites[0];
        if (pc && pc->title)
            p4_settings_native_draw_title(pos.x, pos.y + 10.0f, 0x5544DD00u | alpha);
        else
            sdkSpriteDrawAt(sprite, pos.x, pos.y, 0x5E, 0x37, 0xFF, alpha);
    }
    if ((menu->visible & 2) != 0) {
        {
            u8 *graphEntry, *graphSprite;
            graphX = (baseX + menu->motion[17].position.x) - 23.0f;
            graphY = baseY + menu->motion[17].position.y;
            alpha = (u8)((f32)menu->motion[17].opacity * fade);
            graphSprite = menu->sprites[10];
            index = 0;
            topAlpha = 0.6f * (f32)alpha;
            filledAlpha = 0.5f * (f32)alpha;
            for (; index < 11; index++) {
                pos.x = graphX + (f32)(index * 45);
                cellIndex = 0;
                /* Live graph counters may change during sprite callbacks. */
                graphEntry = work + index * 10;
                for (; cellIndex < ((CmpConfigWork *)graphEntry)->graph[0].height; cellIndex++) {
                    if (cellIndex == ((CmpConfigWork *)graphEntry)->graph[0].height - 1) {
                        color.rgba.red = 0xFF;
                        color.rgba.green = 0xFF;
                        color.rgba.blue = 0xA4;
                        color.rgba.alpha = (u8)topAlpha;
                    } else if (cellIndex < ((CmpConfigWork *)graphEntry)->graph[0].filled) {
                        color.rgba.red = 0xFE;
                        color.rgba.green = 0xFF;
                        color.rgba.blue = 0x56;
                        color.rgba.alpha = (u8)filledAlpha;
                    } else {
                        continue;
                    }
                    pos.y = graphY + (f32)(cellIndex * 17);
                    sdkSpriteDrawAt(graphSprite, pos.x, pos.y, color.rgba.red, color.rgba.green, color.rgba.blue,
                                  color.rgba.alpha);
                }
            }
        }
        index = 0;
        pendingOption = -1;
        for (; index < 7; index++) {
            P4SettingsNativeRow *nativeRow = pc ? &pc->rows[index] : NULL;
            if (nativeRow && nativeRow->kind == P4_SETTINGS_NATIVE_EMPTY) continue;
            s32 retailIndex = nativeRow
                ? (nativeRow->kind == P4_SETTINGS_NATIVE_RETAIL ? nativeRow->index : -1) : index;
            {
                f32 optionX, optionY;
                CmpConfigMotion *optionRow;
                u8 green;
                f32 pairY;
                /* Signed row offsets retain the work-relative option view. */
                optionRow = (CmpConfigMotion *)(work + index * 48 + 0xA8);
                optionX = baseX + optionRow->position.x;
                optionY = (f32)index * 32.0f + (baseY + optionRow->position.y);
                pos.x = 219.0f + optionX;
                pos.y = 119.0f + optionY;
                alpha = (u8)((f32)optionRow->opacity * fade);
                if ((retailIndex == 2) && (menu->options[1] == 0)) {
                    color.rgba.red = 0xA0;
                    color.rgba.green = 0xA0;
                    color.rgba.blue = 0xA0;
                } else {
                    color.transport = *(f32 *)D_0064B2E0;
                }
                sprite = menu->sprites[1];
                rowBlue = color.rgba.blue;
                green = color.rgba.green;
                pairY = pos.y;
                sdkSpriteDrawAt(sprite, pos.x, pairY, color.rgba.red, green, rowBlue, alpha);
                sprite = menu->sprites[2];
                sdkSpriteDrawAt(sprite, 380.0f + pos.x, pairY, color.rgba.red, green, rowBlue, alpha);
                if (index == selectedOption) {
                    u8 *selectedPalette;
                    selected = 1;
                    selectedPalette = D_0064B2E8;
                    sprite = menu->sprites[6];
                    if (nativeRow && nativeRow->kind == P4_SETTINGS_NATIVE_INFO)
                        p4_settings_native_draw_info_bar(sprite, pos.x, pairY,
                            ((u32)selectedPalette[0] << 24) | ((u32)selectedPalette[1] << 16) |
                            ((u32)selectedPalette[2] << 8) | alpha);
                    else
                        sdkSpriteDrawAt(sprite, pos.x, pairY, selectedPalette[0], selectedPalette[1],
                                      selectedPalette[2], alpha);
                    sprite = menu->sprites[8];
                    sdkSpriteDrawAt(sprite,
                        (nativeRow && nativeRow->kind == P4_SETTINGS_NATIVE_INFO ? 380.0f : 247.0f) + pos.x,
                        pairY, selectedPalette[0], selectedPalette[1], selectedPalette[2], alpha);
                    if ((retailIndex == 2) && (menu->options[1] == 0)) {
                        textColor = (alpha & 0xFF) | 0xB4B4B400;
                    } else {
                        textColor = (alpha & 0xFF) | 0xFFFFFF00;
                    }
                } else {
                    selected = 0;
                    if (retailIndex == 2) {
                        textColor = (((u32)(0x80) << 24) | ((u32)(0x80) << 16) | ((u32)(0x80) << 8) | (alpha));
                    } else {
                        textColor = (alpha & 0xFF) | 0x80808000;
                    }
                }
                if (nativeRow && (nativeRow->flags & P4_SETTINGS_NATIVE_ROW_DISABLED))
                    textColor = (alpha & 0xFF) | 0xA0A0A000;
                pos.x = (f32)0x1C7 + optionX;
                pos.y = 122.0f + optionY;
                if (nativeRow && nativeRow->kind != P4_SETTINGS_NATIVE_RETAIL &&
                    nativeRow->kind != P4_SETTINGS_NATIVE_CONFIRM)
                    p4_settings_native_draw_label(pos.x, pos.y, (u32)textColor, index);
                else
                    frFontExDrawTextWithCharWidth(pos.x, pos.y, 0.0f, textColor, 8, 1, (const char *)(nativeRow ? nativeRow->label : D_0064D230[index]), 2, -1);
            }
            if (nativeRow && nativeRow->kind == P4_SETTINGS_NATIVE_INFO) continue;
            if (nativeRow ? (nativeRow->kind == P4_SETTINGS_NATIVE_RETAIL ||
                nativeRow->kind == P4_SETTINGS_NATIVE_TOGGLE) : index != 6) {
                u16 *timer = nativeRow ? (u16 *)nativeRow->timer : &menu->timers[index];
                if (*timer >= 5) {
                    cmpConfigDrawOnOff(work, index, 0);
                } else {
                    if (pendingOption != -1) {
                        cmpConfigDrawOnOff(work, pendingOption, 1);
                        cmpConfigDrawOnOff(work, pendingOption, 2);
                    }
                    pendingOption = index;
                    *timer = *timer + 1;
                }
            } else {
                u8 *button;
                f32 *buttonX;
                f32 *buttonY;
                u8 *buttonPalette;
                /* Keep the field addresses, not their values, across callbacks.
                   This is a size-based lookup in the second animation group. */
                button = work + index * sizeof(CmpConfigMotion);
                buttonX = &((CmpConfigWork *)button)->motion[8].position.x;
                pos.x = (f32)0x1CF + (baseX + *buttonX);
                buttonY = &((CmpConfigWork *)button)->motion[8].position.y;
                pos.y = (nativeRow ? 119.0f + (f32)index * 32.0f : (f32)0x137) + (baseY + *buttonY);
                buttonPalette = D_0064B2E8;
                sprite = menu->sprites[7];
                if (nativeRow && nativeRow->kind != P4_SETTINGS_NATIVE_CONFIRM)
                    sdkSpriteDrawSized(sprite, pos.x, pos.y, 0.0f,
                        buttonPalette[0], buttonPalette[1], buttonPalette[2], alpha,
                        0xF5C, 0x1000, 0, 0.0f, 0);
                else
                    sdkSpriteDrawAt(sprite, pos.x, pos.y, buttonPalette[0], buttonPalette[1], buttonPalette[2], alpha);
                sprite = menu->sprites[8];
                sdkSpriteDrawAt(sprite, ((nativeRow && nativeRow->kind != P4_SETTINGS_NATIVE_CONFIRM) ? 144.0f : 150.0f) + pos.x, pos.y, buttonPalette[0], buttonPalette[1], buttonPalette[2],
                              alpha);
                pos.x = (f32)0x205 + (baseX + *buttonX);
                pos.y = (nativeRow ? 127.0f + (f32)index * 32.0f : (f32)0x13F) + (baseY + *buttonY);
                sprite = menu->sprites[9];
                if (selected != 0) {
                    buttonColor = D_0064B2EC;
                } else {
                    buttonColor = D_0064B304;
                }
                if (nativeRow && (nativeRow->kind == P4_SETTINGS_NATIVE_VALUE ||
                    nativeRow->kind == P4_SETTINGS_NATIVE_BINDING ||
                    nativeRow->kind == P4_SETTINGS_NATIVE_CHOICE)) {
                    p4_settings_native_draw_value(535.0f + (baseX + *buttonX),
                        pos.y - 5.0f, (u32)textColor, index);
                    /* Equipment's real stat-arrow art, through the stock
                       rotation/pivot path, fits within the value pill. */
                    if (nativeRow->kind != P4_SETTINGS_NATIVE_CHOICE) {
                    sdkSpriteDrawSized((u8 *)pc->choiceArrows[0],
                        474.0f + (baseX + *buttonX), pos.y + 7.0f, 0.0f,
                        buttonColor[0], buttonColor[1], buttonColor[2], alpha,
                        0xC00, 0xC00, 8, 225.0f, 8);
                    sdkSpriteDrawSized((u8 *)pc->choiceArrows[1],
                        596.0f + (baseX + *buttonX), pos.y + 7.0f, 0.0f,
                        buttonColor[0], buttonColor[1], buttonColor[2], alpha,
                        0xC00, 0xC00, 8, 45.0f, 8);
                    }
                } else {
                    sdkSpriteDrawAt(sprite, pos.x, pos.y, buttonColor[0], buttonColor[1], buttonColor[2], alpha);
                }
            }
        }
        if (pendingOption != -1) {
            cmpConfigDrawOnOff(work, pendingOption, 1);
            cmpConfigDrawOnOff(work, pendingOption, 2);
        }
        if (pc && pc->count > 7) {
            f32 scrollX = 626.0f + (baseX + menu->motion[15].position.x);
            f32 scrollY = 102.0f + (baseY + menu->motion[15].position.y) + pc->scrollOffset;
            u8 scrollAlpha = (u8)((f32)menu->motion[15].opacity * fade);
            /* System's 165-pixel rail spans this seven-row list's 227 pixels. */
            sdkSpriteDrawSized((u8 *)pc->scrollbar[1], scrollX, scrollY, 0.0f,
                0xFF, 0xFF, 0xFF, scrollAlpha, 0x1000, 0x1603, 0, 0.0f, 0);
            sdkSpriteDrawAt((u8 *)pc->scrollbar[2], scrollX, scrollY + 227.0f,
                0xFF, 0xFF, 0xFF, scrollAlpha);
            sdkSpriteDrawAt((u8 *)pc->scrollbar[0], scrollX,
                scrollY + 3.0f + (f32)(pc->scroll * 196 / (pc->count - 7)),
                D_0064B2E8[0], D_0064B2E8[1], D_0064B2E8[2], scrollAlpha);
        }
        pos.x = (f32)0x26B + (baseX + menu->motion[15].position.x);
        pos.y = (f32)0x15B + (baseY + menu->motion[15].position.y);
        alpha = (u8)((f32)menu->motion[15].opacity * fade);
        textColor = alpha | ~0xFF;
        if (pc && pc->rows[selectedOption].kind != P4_SETTINGS_NATIVE_RETAIL &&
            pc->rows[selectedOption].kind != P4_SETTINGS_NATIVE_CONFIRM)
            p4_settings_native_draw_help(pos.x, pos.y, (u32)textColor, selectedOption);
        else
            frFontExDrawText(pos.x, pos.y, 0.0f, textColor, 6, 1, (const char *)(pc ? pc->rows[selectedOption].help : D_0064D380[selectedOption]), 2, 0);
    }
    pos.x = 640.0f + (baseX + menu->motion[16].position.x);
    pos.y = 400.0f + (baseY + menu->motion[16].position.y);
    alpha = (u8)((f32)menu->motion[16].opacity * fade);
    sdkSpriteDrawFooter(pos, 0.0f, alpha, pc ?
        ((pc->rows[selectedOption].kind == P4_SETTINGS_NATIVE_INFO ||
          pc->rows[selectedOption].kind == P4_SETTINGS_NATIVE_EMPTY ||
          (pc->rows[selectedOption].flags & P4_SETTINGS_NATIVE_ROW_DISABLED)) ? 15 :
         (pc->rows[selectedOption].kind == P4_SETTINGS_NATIVE_RETAIL ||
          pc->rows[selectedOption].kind == P4_SETTINGS_NATIVE_TOGGLE) ? 0x12 : 0)
        : menu->helpStyle, menu->helpTexture);
}