#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"

/* 003627b0..00362efc: native System task. A valid Camp shortcut selects
 * the existing Config command at the ordinary interactive phase. Keep its
 * nLine transitions, cmpConfigCreate child, flag synchronization and cleanup.
 * Circle still wins, cancels the shortcut and follows the retail back path. */
s32 cmpSystemMenuUpdate(u32 *phase, s32 *active, u8 *work)
{
    u8 query[0x30];
    s32 resultFlag = 1;
    s32 inputResult;
    s16 selection;
    s32 shortcut;
    f32 alpha;

    /* Cases jump to a shared finish label instead of returning early, which follows the retail
     * control flow. */
    *active = 1;
    switch (*phase) {
    case 0:
        ((RadarMenuWork *)work)->x.s = 0;
        ((RadarMenuWork *)work)->y.s = 0;
        ((RadarMenuWork *)work)->alpha = 0xFF;
        cmpSystemMenuInit(work);
        *phase = 1;
        *active = 0;
        goto finish;
    case 1:
        if ((datCommunityPanelOpenStepAlt(work) != 0) && (nLineIsIdle() != 0)) {
            *phase = 3;
            nLineSelectSet(0x1D);
        } else {
            *active = 0;
        }
        goto finish;
    case 2:
        if (radarRowsSettled(work) != 0) {
            cmpSystemMenuRelease(work);
            return 1;
        }
        goto finish;
    case 3:
        if (radarRowsSettled(work) != 0) {
            cmpSystemSetMode(work, 2);
            *phase = 4;
        }
        goto finish;
    case 4:
        shortcut = p4_settings_native_system_route((u32)work,
            (D_008C024E[0] & 0x20) == 0);
        if (D_008C024E[0] & 0x20) {
            cmpSystemSetMode(work, resultFlag);
            *phase = 2;
            nLineSelectSet(0x1E);
            sdkSndStartStream(0, 2, 0, 4);
        } else if (shortcut || (D_008C024E[0] & 0x40)) {
            if (shortcut) cmpSystemSetSelection(work, 0, 1);
            selection = ((RadarMenuWork *)work)->selectedHeader;
            switch (selection) {
            case 0:
                cmpSystemSetMode(work, resultFlag);
                *phase = 5;
                nLineSelectSet(0x1E);
                break;
            case 1:
                cmpSystemSetMode(work, resultFlag);
                *phase = 8;
                nLineSelectSet(0x1E);
                break;
            case 2:
                nmMesWindowShowSlotRecord(((RadarMenuWork *)(work))->messageSlot, 4);
                nmMesWindowSetSlotParam(MEMBER(s8, work, RadarMenuWork, messageSlot), 0);
                nmMesWindowRequestRefresh();
                nmMesWindowSetSlotFlag(MEMBER(s8, work, RadarMenuWork, messageSlot), 1);
                *phase = 0xA;
                break;
            }
            sdkSndStartStream(0, 0, 0, 1);
        } else if (D_008C024E[0] & 0x10) {
            ((RadarMenuWork *)work)->background = datCommunityRequestExit();
            *phase = 0xC;
        } else {
            sdkListStateConfigure(query, 3, 3, ((RadarMenuWork *)work)->selectedHeader, 0);
            sdkListStateSetSecondaryMasks(query, 0x4000, 0x1000, 0, 0);
            if (sdkListStateHandleInput(query) != 0) {
                selection = ((SdkListState *)query)->cursor;
                cmpSystemSetSelection(work, 0, selection);
                sdkSndStartStream(0, 1, 0, 0);
            }
        }
        goto finish;
    case 5:
        if (radarRowsSettled(work) != 0) {
            cmpSystemFocusFlaggedRow(work);
            cmpSystemSetMode(work, 3);
            *phase = 6;
            nLineSelectSet(0x1F);
        }
        goto finish;
    case 6:
        if (radarRowsSettled(work) != 0) {
            cmpSystemSetMode(work, 4);
            *phase = 7;
            resrcMngSuppressRender();
        }
        goto finish;
    case 7:
        if (D_008C024E[0] & 0x20) {
            cmpSystemSetMode(work, 5);
            *phase = 1;
            nLineSelectSet(0x20);
            resrcMngUnsuppressRender();
            sdkSndStartStream(0, 0, 0, 2);
        } else if (D_008C024E[0] & 0x10) {
            ((RadarMenuWork *)work)->background = datCommunityRequestExit();
            *phase = 0xC;
        } else {
            sdkListStateConfigure(query, 6, ((RadarMenuWork *)work)->rowCount,
                          ((RadarMenuWork *)work)->selectedRow, ((RadarMenuWork *)(work))->topRow);
            sdkListStateSetSecondaryMasks(query, 0x4000, 0x1000, 0x2000, 0x8000);
            if ((inputResult = sdkListStateHandleInput(query)) > 0) {
                selection = ((SdkListState *)query)->cursor;
                cmpSystemSetSelection(work, 1, selection);
                selection = ((SdkListState *)query)->scroll;
                cmpSystemSetSelection(work, 2, selection);
                if (MEMBER(s16, work, RadarMenuWork, topRow) != ((RadarMenuWork *)work)->previousTopRow) {
                    radarResetSlideDirection(work);
                }
                datCommunityCursorFeedback(inputResult);
            }
        }
        goto finish;
    case 8:
        if (nLineIsIdle() != 0) {
            ((RadarMenuWork *)(work))->configTask.handle = cmpConfigCreate(0, 0);
            *phase = 9;
        }
        goto finish;
    case 9:
        if (cmpConfigIsStepDone(((RadarMenuWork *)(work))->configTask.node) != 0) {
            if (cmpConfigIsStepCancelled(MEMBER(u8 *, work, RadarMenuWork, configTask.node)) != 0) {
                return 2;
            }
            kwlnTaskDestroyWithHierarchy(MEMBER(s32, work, RadarMenuWork, configTask.handle));
            MEMBER(s32, work, RadarMenuWork, configTask.handle) = 0;
            *phase = 1;
        }
        goto finish;
    case 10:
        nmMesWindowStepSlot(((RadarMenuWork *)(work))->messageSlot, resultFlag);
        if (nmMesWindowSlotStatus(MEMBER(s8, work, RadarMenuWork, messageSlot)) == 0) {
            switch (nmMesWindowSlotResult(MEMBER(s8, work, RadarMenuWork, messageSlot))) {
            case 0:
                fadeBeginTransition(1, 0x1E);
                sdkSndStopChannel(0x1E);
                sdkSndFadeOutChannelFrames(3, 0, 0x1E);
                *phase = 0xB;
                break;
            case 1:
                *phase = 4;
                break;
            }
            nmMesWindowCloseSlot((s8)MEMBER(s8, work, RadarMenuWork, messageSlot), 1);
        }
        goto finish;
    case 11:
        if (fadeStatus() != 0) {
            sequenceRequest(0x1B, 0, 0, 0xA);
        }
        goto finish;
    case 12:
        if ((++((RadarMenuWork *)work)->fadeFrame & 0xFFFF) >= 3) {
            resultFlag = 1;
        } else {
            alpha = (1.0f - ((f32)MEMBER(u16, work, RadarMenuWork, fadeFrame) / 3.0f)) * 255.0f;
            inputResult = (u8)alpha;
            ((RadarMenuWork *)work)->alpha = inputResult;
            resultFlag = 0;
        }
        break;
    default:
        goto finish;
    }
    if (resultFlag != 0) {
        return 2;
    }
finish:
    return 0;
}