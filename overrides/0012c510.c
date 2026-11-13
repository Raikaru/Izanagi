#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"
#include "p4pc/mod_status.h"
#include "p4pc/ui_overlay.h"

/* 0012c510..0012cc4c: retail Camp dispatcher, verified against the ELF.
 * An accepted field request selects the real enabled System command only after
 * the native title has settled; all task, nLine, SFX and exit paths stay native. */
s32 cmpMainTaskStep(u8 *unusedTask) {
    u8 *work;
    CmpMainWork *menu;
    u32 state;
    s32 result;
    s32 itemChanged;
    s32 recipientChanged;
    s32 equipmentChanged;
    s32 personaChanged;
    s32 otherChanged;
    s32 systemChanged;
    s32 communityChanged;
    s32 commandOffset;
    u32 commandAddress;
    u8 inputState[0x30];

    work = iGpffffb1ec;
    menu = (CmpMainWork *)work;
    menu->nodeNext = 0;
    menu->nodeTail = 0;
    PrimAppendNode((u8 *)D_00793E80 + menu->tableIndex * 0x30, work + 0x970C);
    state = menu->state.value;
    switch (state) {
    case 0:
        *(s32 *)&menu->title.x = 0;
        *(s32 *)&menu->title.y = 0;
        menu->title.alpha = 0xFF;
        menu->title.selection = 0;
        menu->state.signedValue = 1;
        sdkSndStartStream(0, 2, 0, 3);
        break;
    case 1:
        menu->activeMenus = 1;
        menu->state.signedValue = 2;
        titleSlotTableInit(work + 0x18);
        nLineSelectSet(1);
        p4_ui_overlay_context_begin(P4_UI_CONTEXT_CAMP);
        break;
    case 2:
        menu->activeMenus = 1;
        if ((titleSlotsSettled(work + 0x18) != 0) && (menu->activeMenus == 1)) {
            s32 shortcut = 0;
            p4_mod_status_draw();
            p4_ui_overlay_draw(P4_UI_CONTEXT_CAMP);
            if (p4_settings_native_camp_route((u32)work)) {
                for (s32 slot = 0; slot < menu->title.slotCount; ++slot) {
                    if (menu->title.commandIds[slot] == 6) {
                        titleSelectSlot(work + 0x18, slot);
                        shortcut = 1;
                        break;
                    }
                }
            }
            if (shortcut || (D_008C024E[0] & 0x40)) {
                menu->detailState = 0;
                menu->state.signedValue = 3;
                commandOffset = menu->title.selection * 4;
                commandAddress = commandOffset + (u32)work;
                switch (((CmpMainWork *)commandAddress)->title.commandIds[0]) {
                case 1:
                    menu->nextState = 4;
                    break;
                case 0:
                    menu->nextState = 5;
                    datGridEffectCreate(0, iGpffff9cb0, 0x14, 8, 1, 2, 0xC7, 0xB1);
                    break;
                case 2:
                    menu->nextState = 6;
                    break;
                case 3:
                    menu->nextState = 7;
                    break;
                case 4:
                    menu->nextState = 8;
                    break;
                case 5:
                    menu->nextState = 9;
                    datGridEffectCreate(0, iGpffff9cb4, 0x14, 8, 1, 2, 0xC7, 0xB1);
                    break;
                case 6:
                    menu->nextState = 0xA;
                    break;
                default:
                    kAssertFail(D_005E5890, 0xFD);
                    break;
                }
                titleSlotSetMode(work + 0x18, 3);
                nLineSelectSet(2);
                sdkSndStartStream(0, 2, 0, 3);
            } else if ((D_008C024E[0] & 0x20) || (D_008C024E[0] & 0x10)) {
                titleSlotSetMode(work + 0x18, 3);
                menu->state.signedValue = 3;
                menu->nextState = 0xD;
                menu->closingDone = 0;
                nLineSelectSet(2);
                sdkSndStartStream(0, 2, 0, 4);
            } else {
                sdkListStateConfigure(&inputState, menu->title.slotCount, menu->title.slotCount,
                              menu->title.selection, 0);
                sdkListStateSetSecondaryMasks(&inputState, 0x4000, 0x1000, 0, 0);
                if (sdkListStateHandleInput(&inputState) != 0) {
                    titleSelectSlot(work + 0x18, ((SdkListState *)&inputState)->cursor);
                }
            }
        }
        break;
    case 13:
        if ((titleSlotFramesElapsed(work + 0x18) != 0) && (menu->closingDone != 0)) {
            return -1;
        }
        break;
    case 3:
        result = menu->nextState;
        if ((result < 4) || (result > 0xD)) {
            kAssertFail(D_005E5890, 0x133);
        }
        if (titleSlotFramesElapsed(work + 0x18) != 0) {
            titleFreeSpriteNodes(work + 0x18);
            menu->state.signedValue = menu->nextState;
        }
        break;
    case 4:
        result = radarItemMenuUpdate(work + 4, &itemChanged, work + 0x2BC);
        switch (result) {
        case 0:
            if (itemChanged != 0) {
                menu->activeMenus = 2;
            }
            break;
        case 1:
            menu->detailState = 0;
            menu->state.signedValue = 1;
            break;
        case 2:
            menu->activeMenus = 0;
            return -1;
        }
        break;
    case 5:
        result = radarRecipientMenuUpdate((u32 *)(work + 4), &recipientChanged, work + 0x1EA4);
        switch (result) {
        case 0:
            if (recipientChanged != 0) {
                menu->activeMenus = 4;
            }
            break;
        case 1:
            menu->detailState = 0;
            menu->state.signedValue = 1;
            break;
        case 2:
            menu->activeMenus = 0;
            return -1;
        }
        break;
    case 6:
        result = radarTransferMenuUpdate(work + 4, &equipmentChanged, work + 0x31DC);
        switch (result) {
        case 0:
            if (equipmentChanged != 0) {
                menu->activeMenus = 0x10;
            }
            break;
        case 1:
            menu->detailState = 0;
            menu->state.signedValue = 1;
            break;
        case 2:
            menu->activeMenus = 0;
            return -1;
        }
        break;
    case 7:
        result = radarPersonaMenuUpdate(work + 4, &personaChanged, work + 0x5A8C);
        switch (result) {
        case 0:
            if (personaChanged != 0) {
                menu->activeMenus = 0x20;
            }
            break;
        case 1:
            menu->detailState = 0;
            menu->state.signedValue = 1;
            break;
        case 2:
            menu->activeMenus = 0;
            return -1;
        }
        break;
    case 8:
        result = radarUpdateState((u32 *)(work + 4), &otherChanged, work + 0x7750);
        switch (result) {
        case 0:
            if (otherChanged != 0) {
                menu->activeMenus = 0x40;
            }
            break;
        case 1:
            menu->detailState = 0;
            menu->state.signedValue = 1;
            break;
        case 2:
            menu->activeMenus = 0;
            return -1;
        }
        break;
    case 10:
        result = cmpSystemMenuUpdate((u32 *)(work + 4), &systemChanged, work + 0x9000);
        switch (result) {
        case 0:
            if (systemChanged != 0) {
                menu->activeMenus = 0x80;
            }
            break;
        case 1:
            menu->detailState = 0;
            menu->state.signedValue = 1;
            break;
        case 2:
            menu->activeMenus = 0;
            return -1;
        }
        break;
    case 9:
        result = datCommunityMenuStep(work + 4, &communityChanged, work + 0x4774);
        switch (result) {
        case 0:
            if (communityChanged != 0) {
                menu->activeMenus = 8;
            }
            break;
        case 1:
            menu->detailState = 0;
            menu->state.signedValue = 1;
            break;
        case 2:
            menu->activeMenus = 0;
            return -1;
        }
        break;
    }
    return 0;
}
