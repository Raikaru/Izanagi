#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"
extern s32 sdkListStateStepForward(u8 *state);
extern s32 sdkListStateStepBack(u8 *state);

/* 0035f0c0..0035f5a0 is Config input. Retail-only qualification runs the
 * original seven-row branch below, including dependencies, COMSE arguments
 * and fade math.
 * Expanded lists use the same SDK list/cursor, option setters and transitions;
 * Controls and registered pages change views without re-entering resource
 * suppression or changing the six retail flags/18 motion records/0x4b0-byte work. */
s32 cmpConfigStepInput(u32 *phase, s32 *active, u8 *configWork)
{
    extern u16 D_008C024E[];
    extern u16 D_008C0276[];
    extern s32 datCommunityRequestExit(void);
    u8 query[0x30];
    f32 alpha;
    s32 resultFlag;
    s32 kind;
    s32 hit;
    s32 value;
    P4SettingsNativeState *pc = (P4SettingsNativeState *)p4_settings_native_state((u32)configWork, 0);

    *active = (resultFlag = 1);
    switch (*phase) {
    case 0:
        ((CmpConfigWork *)configWork)->unk8 = 0;
        ((CmpConfigWork *)configWork)->unkC = 0;
        ((CmpConfigWork *)configWork)->alpha = 0xFF;
        cmpConfigInitState(configWork);
        p4_settings_native_begin((u32)configWork, ((CmpConfigWork *)configWork)->mode);
        *phase = 1;
        *active = 0;
        break;
    case 1:
        if (cmpConfigAdvancePhase(configWork) & nLineIsIdle()) {
            *phase = 3;
            nLineSelectSet(0x21);
        } else {
            *active = 0;
        }
        break;
    case 2:
        if (cmpConfigAllRowsMet(configWork) != 0) {
            cmpConfigFreeSprites(configWork);
            return 1;
        }
        break;
    case 3:
        if (cmpConfigAllRowsMet(configWork) == 0) {
            break;
        }
        switch (((CmpConfigWork *)configWork)->mode) {
        case 0:
            resrcMngSuppressRender();
            break;
        case 1:
            break;
        }
        cmpConfigSetMode(configWork, 2);
        *phase = 4;
        /* fall through: process input as soon as the Config panel becomes ready */
    case 4:
        if (pc != NULL) {
            s32 touchRow = pc->touchRow;
            s32 touchActivate = pc->activate;
            s32 direction = pc->direction;
            s32 back = pc->back;
            s32 step = pc->step;
            s32 selected = ((CmpConfigWork *)configWork)->selected;
            s32 feedback;
            u16 pressed = D_008C024E[0];
            P4SettingsNativeRow *row;
            pc->touchRow = -1;
            pc->activate = pc->back = pc->step = 0;
            if (p4_settings_model_capturing()) {
                if (back) p4_settings_model_cancel_capture();
                else if (touchActivate) {
                    if (direction < 0) p4_settings_model_clear_capture();
                    else p4_settings_model_confirm_capture();
                }
                goto native_done;
            }
            if (pc->navigation == P4_SETTINGS_NATIVE_SWITCH) {
                cmpConfigSetMode(configWork, 1);
                *phase = 6;
                goto native_done;
            }
            if (pc->navigation == P4_SETTINGS_NATIVE_CLOSE) {
                if ((pc->flags & P4_SETTINGS_NATIVE_SHORTCUT) &&
                    ((CmpConfigWork *)configWork)->mode == 0) {
                    ((CmpConfigWork *)configWork)->background = datCommunityRequestExit();
                    *phase = 5;
                    sdkSndStartStream(0, 0, 0, 2);
                    goto native_done;
                }
                if (!(pc->flags & P4_SETTINGS_NATIVE_HOSTED)) cmpConfigSyncFlags(configWork);
                cmpConfigSetMode(configWork, 1);
                *phase = 2;
                nLineSelectSet(0x22);
                if (((CmpConfigWork *)configWork)->mode == 0)
                    resrcMngUnsuppressRender();
                goto native_done;
            }
            if (pc->view >= 2 && (pressed & 3)) {
                p4_settings_native_help_step((pressed & 1) ? -1 : 1);
                goto native_done;
            }
            if (pressed & 0xC) step = (pressed & 4) ? -7 : 7;
            if (back) {
                if (pc->view == P4_SETTINGS_NATIVE_ROOT &&
                    (pc->flags & P4_SETTINGS_NATIVE_SHORTCUT) &&
                    ((CmpConfigWork *)configWork)->mode == 0) {
                    pressed |= 0x10;
                    sdkSndStartStream(0, 0, 0, 2);
                } else {
                    pressed |= 0x20;
                }
            }
            if (pc->jump >= 0) {
                s32 logical = pc->jump < pc->count ? pc->jump : pc->count - 1;
                pc->jump = -1;
                pc->scroll = logical < pc->count - 7 ? logical : pc->count - 7;
                if (pc->scroll < 0) pc->scroll = 0;
                cmpConfigSetValue(configWork, 0, logical - pc->scroll);
                sdkSndStartStream(0, 1, 0, 0);
                goto native_done;
            }
            if (touchRow >= 0 && touchRow < 7 && pc->scroll + touchRow < pc->count) {
                if (touchRow != selected) {
                    cmpConfigSetValue(configWork, 0, touchRow);
                    selected = touchRow;
                    sdkSndStartStream(0, 1, 0, 0);
                }
                if (touchActivate) pressed |= 0x40;
            }
            row = &pc->rows[selected];
            if (pressed & 0x40) {
                if (pc->view >= 2) {
                    p4_settings_native_activate(selected,
                        row->kind == P4_SETTINGS_NATIVE_VALUE ||
                        row->kind == P4_SETTINGS_NATIVE_TOGGLE ? direction : 0);
                    goto native_done;
                }
                if (row->kind == P4_SETTINGS_NATIVE_CONFIRM) {
                    cmpConfigSyncFlags(configWork);
                    cmpConfigSetMode(configWork, 1);
                    *phase = 2;
                    nLineSelectSet(0x22);
                    if (((CmpConfigWork *)configWork)->mode == 0)
                        resrcMngUnsuppressRender();
                    sdkSndStartStream(0, 0, 0, 1);
                    goto native_done;
                }
                if (row->kind == P4_SETTINGS_NATIVE_CONTROLS) {
                    pc->nextView = P4_SETTINGS_NATIVE_CONTROL_LIST;
                    cmpConfigSetMode(configWork, 1);
                    *phase = 6;
                    sdkSndStartStream(0, 0, 0, 1);
                    goto native_done;
                }
                if (row->kind == P4_SETTINGS_NATIVE_RETAIL) {
                    if (direction == 0 ? cmpConfigToggleOption(configWork, row->index)
                        : cmpConfigSetOption(configWork, row->index, direction < 0))
                        sdkSndStartStream(0, 0, 0, 1);
                    goto native_done;
                }
                if (row->kind == P4_SETTINGS_NATIVE_ACTION) direction = 0;
                goto native_adjust;
            }
            if (pressed & 0x20) {
                if (pc->view != P4_SETTINGS_NATIVE_ROOT && !(pc->flags & P4_SETTINGS_NATIVE_HOSTED_TOP)) {
                    pc->nextView = P4_SETTINGS_NATIVE_BACK;
                    cmpConfigSetMode(configWork, 1);
                    *phase = 6;
                } else {
                    cmpConfigSetMode(configWork, 1);
                    *phase = 2;
                    nLineSelectSet(0x22);
                    if (((CmpConfigWork *)configWork)->mode == 0)
                        resrcMngUnsuppressRender();
                }
                sdkSndStartStream(0, 0, 0, 2);
                goto native_done;
            }
            if (pressed & 0x10) {
                if (((CmpConfigWork *)configWork)->mode == 0 && !(pc->flags & P4_SETTINGS_NATIVE_HOSTED)) {
                    ((CmpConfigWork *)configWork)->background = datCommunityRequestExit();
                    *phase = 5;
                }
                goto native_done;
            }
            if (touchRow >= 0) goto native_done;
            sdkListStateConfigure(query, 7, pc->count, selected, pc->scroll);
            sdkListStateSetSecondaryMasks(query, 0x4000, 0x1000, 0, 0);
            hit = 0;
            if (step) {
                for (s32 i = 0; i < (step < 0 ? -step : step); ++i)
                    hit |= step < 0 ? sdkListStateStepBack(query) : sdkListStateStepForward(query);
            } else {
                hit = sdkListStateHandleInput(query);
            }
            if (hit) {
                pc->scroll = ((SdkListState *)query)->scroll;
                cmpConfigSetValue(configWork, 0, ((SdkListState *)query)->cursor);
                sdkSndStartStream(0, 1, 0, 0);
                goto native_done;
            }
            direction = (D_008C0276[0] & 0x8000) ? -1
                : (D_008C0276[0] & 0x2000) ? 1 : 0;
            if (!direction) goto native_done;
            if (row->kind == P4_SETTINGS_NATIVE_RETAIL) {
                if (cmpConfigSetOption(configWork, row->index, direction < 0))
                    sdkSndStartStream(0, 0, 0, 1);
                goto native_done;
            }
            if (row->kind != P4_SETTINGS_NATIVE_VALUE &&
                row->kind != P4_SETTINGS_NATIVE_TOGGLE &&
                row->kind != P4_SETTINGS_NATIVE_BINDING) goto native_done;
        native_adjust:
            if (pc->view >= 2) {
                p4_settings_native_activate(selected, direction);
                goto native_done;
            }
            if (row->kind == P4_SETTINGS_NATIVE_TOGGLE) {
                s32 before = row->boolean;
                if (direction == 0 || before != (direction < 0)) {
                    p4_settings_model_adjust(row->index, 0);
                }
            } else {
                p4_settings_model_adjust(row->index, direction);
            }
        native_done:
            feedback = p4_settings_native_feedback();
            if (feedback >= 0) sdkSndStartStream(0, 0, 0, feedback);
            break;
        }
        /* measured: retail reloads selected after each option call instead of keeping a typed field
         * address. */
        if (D_008C024E[0] & 0x40) {
            kind = ((CmpConfigWork *)(configWork))->selected;
            if (kind == 6) {
                cmpConfigSyncFlags(configWork);
                cmpConfigSetMode(configWork, 1);
                *phase = 2;
                nLineSelectSet(0x22);
                switch (((CmpConfigWork *)configWork)->mode) {
                case 0:
                    resrcMngUnsuppressRender();
                    break;
                case 1:
                    break;
                }
                sdkSndStartStream(0, 0, 0, 1);
            } else if (cmpConfigToggleOption(configWork, kind) != 0) {
                sdkSndStartStream(0, 0, 0, 1);
            }
        } else if (D_008C024E[0] & 0x20) {
            cmpConfigSetMode(configWork, 1);
            *phase = 2;
            nLineSelectSet(0x22);
            switch (((CmpConfigWork *)configWork)->mode) {
            case 0:
                resrcMngUnsuppressRender();
                break;
            case 1:
                break;
            }
            sdkSndStartStream(0, 0, 0, 2);
        } else if (D_008C024E[0] & 0x10) {
            if (((CmpConfigWork *)configWork)->mode == 0) {
                ((CmpConfigWork *)configWork)->background = datCommunityRequestExit();
                *phase = 5;
            }
        } else {
            sdkListStateConfigure(query, 7, 7, ((CmpConfigWork *)(configWork))->selected, 0);
            sdkListStateSetSecondaryMasks(query, 0x4000, 0x1000, 0, 0);
            if (sdkListStateHandleInput(query) != 0) {
                cmpConfigSetValue(configWork, 0, ((SdkListState *)query)->cursor);
                sdkSndStartStream(0, 1, 0, 0);
            } else {
                kind = MEMBER(s32, configWork, CmpConfigWork, selected);
                if (kind != 6) {
                    hit = 0;
                    if (D_008C0276[0] & 0x8000) {
                        hit = cmpConfigSetOption(configWork, kind, 1);
                    } else if (D_008C0276[0] & 0x2000) {
                        hit = cmpConfigSetOption(configWork, kind, 0);
                    }
                    if (hit != 0) {
                        sdkSndStartStream(0, 0, 0, 1);
                    }
                }
            }
        }
        break;
    case 6:
        if (pc && cmpConfigAllRowsMet(configWork)) {
            p4_settings_native_view((u32)configWork, pc->nextView,
                ((CmpConfigWork *)configWork)->selected);
            cmpConfigSetValue(configWork, 0, pc->cursor);
            cmpConfigSetMode(configWork, 0);
            *phase = 7;
        }
        break;
    case 7:
        if (pc && cmpConfigAllRowsMet(configWork)) {
            cmpConfigSetMode(configWork, 2);
            *phase = 4;
        }
        break;
    case 5:
        if ((++((CmpConfigWork *)configWork)->fadeFrame & 0xFFFF) >= 3) {
            resultFlag = 1;
        } else {
            /* measured: retail reloads fadeFrame after the post-increment. */
            alpha = (1.0f - ((f32)MEMBER(u16, configWork, CmpConfigWork, fadeFrame) / 3.0f)) * 255.0f;
            value = (u8)alpha;
            ((CmpConfigWork *)configWork)->alpha = value;
            resultFlag = 0;
        }
        if (resultFlag != 0) {
            return 2;
        }
        break;
    }
    return 0;
}