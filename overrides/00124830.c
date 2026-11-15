#include "p4pc/vu0.h"
#include "p4pc/settings_native.h"
extern s32 sdkListStateSeek(u8 *state, s32 value);

/* 00124830..0012496c: despite its decomp name, this is the three-entry
 * title-menu selector, not a music selector. Returning 3 invokes title state
 * 14's existing cmpConfigCreate(task, 1) path. The no-request path preserves
 * the retail list, cursor history, return values and COMSE calls. */
s32 titleSelectMenu(u8 *context)
{
    u8 listState[0x30];
    s32 previousCursor;
    s32 inputResult;
    s32 cursor;
    s32 confirmed;
    s32 shortcut;
    TitleScene *titleScene = (TitleScene *)context;

    shortcut = p4_settings_native_title_request();
    confirmed = 0;
    sdkListStateConfigure(listState, 3, 3, titleScene->cursor, 0);
    sdkListStateSetSecondaryMasks(listState, 0x4000, 0x1000, 0, 0);
    if (shortcut) {
        sdkListStateSeek(listState, 2);
    } else {
        inputResult = sdkListStateHandleInput(listState);
        switch (inputResult) {
        case 1:
        case 2:
            confirmed = 1;
            break;
        }
    }
    previousCursor = titleScene->cursor;
    cursor = sdkListStateGetCursor(listState);
    titleScene->cursor = cursor;
    if (cursor != previousCursor) {
        titleScene->previousCursor = previousCursor;
        titleScene->cursorFixed = previousCursor << 16;
    }
    if (shortcut || (D_008C024E[0] & 0x40)) {
        sdkSndStartStream(0, 0, 0, 1);
        return titleScene->cursor + 1;
    }
    if (confirmed != 0) {
        sdkSndStartStream(0, 0, 0, 0);
        return 0;
    }
    return -1;
}
