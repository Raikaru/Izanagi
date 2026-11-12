#include "p4pc/native_rows.h"

/* 002e90d0: fclCombineMenuRoot, retail; the fusion card list (sub-states 0x1A and 0x1C) reads its
 * rows through p4_native_rows_velvet_row so a tenth (side) row works, and the mod card's
 * confirmation hosts its page (sub-state P4_NATIVE_ROWS_VELVET_PAGE, include/p4pc/native_rows.h).
 * Without a mod card the row reads are exactly retail's. */
void fclCombineMenuRoot(u8 *task)
{
    FclCombineReward *rewardEntry;
    extern u8 D_00641870[];
    extern s32 fclBustupIsActive(u8 *);
    extern void fclCombineDrawSlotRows(u8 *, s64);
    extern void fclCombineStepSlotSelection(u8 *, s8);
    extern void fclCombineBuildRows(u8 *);
    extern void fclCombineMarkRow(s16);
    extern void fclCombineUpdateEntryHighlight(u8 *, s8, s8, s16, s16);
    extern s8 fclCombineMapSlotKind(s8);
    extern void fclCombineSetSlotColors(u8 *, s16, FclDrawColor, FclDrawColor, FclDrawColor);
    FclDrawColor color0;
    FclDrawColor color1;
    FclDrawColor color2;
    FclDrawColor color3;
    FclDrawColor color4;
    FclDrawColor color5;
    FclDrawColor color6;
    FclDrawColor color7;
    FclDrawColor color8;
    FclDrawColor color9;
    FclDrawColor color10;
    FclDrawColor color11;
    FclDrawColor color12;
    FclDrawColor color13;
    FclDrawColor color14;
    FclDrawColor color15;
    FclDrawColor color16;
    FclDrawColor color17;
    FclVec2 slotPos0;
    s32 msgArgs0[2];
    s32 msgArgs1[2];
    FclVec2 pos0;
    FclVec2 pos1;
    FclVec2 pos2;
    FclVec2 pos3;
    FclVec2 pos4;
    FclVec2 pos5;
    FclVec2 pos6;
    FclVec2 pos7;
    FclVec2 pos8;
    FclVec2 pos9;
    FclVec2 pos10;
    FclVec2 pos11;
    FclVec2 pos12;
    FclVec2 pos13;
    FclVec2 pos14;
    FclVec2 slotPos1;
    FclVec2 slotPos2;
    char text[0x88];
    s32 off;
    s8 selected2;
    s16 k2;
    s16 menu;
    u8 *te;
    s16 n;
    s8 *p;
    s8 selected;
    s16 j;
    FclVec2 *vec;
    u16 buttons;
    s16 k;
    u8 *entry;

    p = (s8*)((KwlnTask *)task)->work;
    switch ((u8)p[1]) {
    case 0x11:
        p[1] = 0x12;
        break;
    case 0x12:
        p[1] = 0x14;
        if (datGetFlag(0x131D) == 0) {
            p[0xD] = nmMesWindowAcquire((void *)fclCombineGetMesWindow());
            nmMesWindowShowSlotRecord(p[0xD], 0x4C);
            datSetFlag(0x131D, 1);
            p[1] = 0x13;
        }
        break;
    case 0x13:
        if (nmMesWindowSlotStatus(p[0xD]) != 0) {
            nmMesWindowStepSlotRelease(p[0xD]);
            break;
        }
        nmMesWindowDestroySlot(p[0xD]);
        p[1] = 0x12;
        break;
    case 0x14:
        selected = -1;
        for (k = 0; k < 2; k++) {
            entry = D_00641870 + k * 8;
            rewardEntry = (FclCombineReward *)entry;
            if (datGetFlag(((FclCombineReward *)entry)->flag) == 0) {
                if ((f32)*(s8 *)entry <= 100.0f * datGetAverageRecordScore()) {
                    selected = k;
                }
            }
        }
        p[0x13F] = selected;
        if (p[0x13F] != -1) {
            entry = D_00641870 + p[0x13F] * 8;
            rewardEntry = (FclCombineReward *)entry;
            p[0xD] = nmMesWindowAcquire((void *)fclCombineGetMesWindow());
            sprintf(text, &iGpffffa8a0, *(s8 *)entry);
            nmMesWindowSetSlotString(p[0xD], 0, text);
            if (datGetAverageRecordScore() >= 1.0f) {
                nmMesWindowShowSlotRecord(p[0xD], 8);
            } else {
                nmMesWindowShowSlotRecord(p[0xD], 7);
            }
            p[1] = 0x15;
        } else {
            p[1] = 0x17;
        }
        break;
    case 0x15:
        if (nmMesWindowSlotStatus(p[0xD]) != 0) {
            nmMesWindowStepSlotRelease(p[0xD]);
            break;
        }
        nmMesWindowDestroySlot(p[0xD]);
        entry = D_00641870 + p[0x13F] * 8;
        rewardEntry = (FclCombineReward *)entry;
        p[0xD] = nmMesWindowAcquire((void *)fclCombineGetMesWindow());
        msgArgs0[0] = ((FclCombineReward *)entry)->item;
        msgArgs0[1] = 0;
        nmMesWindowSetSlotString(p[0xD], 0, msgArgs0);
        nmMesWindowShowSlotRecord(p[0xD], 9);
        datItemSetCount(rewardEntry->item, (u8)((u8)datItemCount(rewardEntry->item) + 1));
        p[1] = 0x16;
        break;
    case 0x16:
        if (nmMesWindowSlotStatus(p[0xD]) != 0) {
            nmMesWindowStepSlotRelease(p[0xD]);
            break;
        }
        nmMesWindowDestroySlot(p[0xD]);
        datSetFlag(((FclCombineReward *)D_00641870)[p[0x13F]].flag, 1);
        selected2 = -1;
        for (k2 = 0; k2 < 2; k2++) {
            entry = D_00641870 + k2 * 8;
            rewardEntry = (FclCombineReward *)entry;
            if (datGetFlag(((FclCombineReward *)entry)->flag) == 0) {
                if ((f32)*(s8 *)entry <= 100.0f * datGetAverageRecordScore()) {
                    selected2 = k2;
                }
            }
        }
        p[0x13F] = selected2;
        if (p[0x13F] == -1) {
            p[1] = 0x17;
            break;
        }
        entry = D_00641870 + p[0x13F] * 8;
        rewardEntry = (FclCombineReward *)entry;
        p[0xD] = nmMesWindowAcquire((void *)fclCombineGetMesWindow());
        msgArgs1[0] = ((FclCombineReward *)entry)->item;
        msgArgs1[1] = 0;
        nmMesWindowSetSlotString(p[0xD], 0, msgArgs1);
        nmMesWindowShowSlotRecord(p[0xD], 9);
        datItemSetCount(rewardEntry->item, (u8)((u8)datItemCount(rewardEntry->item) + 1));
        break;
    case 0x17:
        if (fclBustupIsActive(((FclCombineWork *)p)->bustupTask) == 0) {
            break;
        }
        ((FclCombineWork *)p)->drawTask.handle = fclDrawTaskCreate(task);
        fclCombineCreateSlotIcons(task, 0);
        fclCombineDrawSlotRows(task, 0);
        pos0 = fclMakeVec2(-78.0f, -82.0f);
        fclSpriteCreate(0x80, pos0, 220.0f, 0x3F);
        color0 = fclMakeDrawColor(0x49, 0x72, 0xFF, 0xFF);
        ((FclCombineRowSprite *)fclSpriteEntry(0x80))->color = color0;
        fclSpriteStartTweenAt(0x80, 0, 0x5A0, 0, 0.0f, -360.0f);
        fclSpriteSetFlag(0x80, 6, 0);
        fclSpriteEntry(0x80)[0xDB] = 1;
        pos1 = fclMakeVec2(2.0f, -2.0f);
        pos2 = fclMakeVec2(-78.0f, -82.0f);
        fclSpriteStartPositionAt(0x80, pos1, pos2, 1, 0xA, 0);
        fclSpriteStartAlphaAt(0x80, 0, 0xFF, 2, 0xA, 0);
        pos3 = fclMakeVec2(540.0f, (f32)0x15B);
        fclSpriteCreate(0x20C, pos3, 220.0f, 0x3F);
        color1 = fclMakeDrawColor(0x49, 0x72, 0xFF, 0xFF);
        ((FclCombineRowSprite *)fclSpriteEntry(0x20C))->color = color1;
        fclSpriteStartTweenAt(0x20C, 0, 0x5A0, 0, 0.0f, -360.0f);
        fclSpriteSetFlag(0x20C, 6, 0);
        fclSpriteEntry(0x20C)[0xDB] = 1;
        pos4 = fclMakeVec2(460.0f, (f32)0x10B);
        pos5 = fclMakeVec2(540.0f, (f32)0x15B);
        fclSpriteStartPositionAt(0x20C, pos4, pos5, 1, 0xA, 0);
        fclSpriteStartAlphaAt(0x20C, 0, 0xFF, 2, 0xA, 0);
        pos6 = fclMakeVec2(-190.0f, 200.0f);
        fclSpriteCreate(0x81, pos6, 215.0f, 0x56);
        color2 = fclMakeDrawColor(0x49, 0x72, 0xFF, 0xFF);
        ((FclCombineRowSprite *)fclSpriteEntry(0x81))->color = color2;
        fclSpriteStartAlphaAt(0x81, 0, 0xFF, 0, 0xA, 0);
        ((FclCombineMenuSprite *)fclSpriteEntry(0x81))->value = 90.0f;
        vec = (FclVec2 *)D_00640C50;
        pos7 = fclMakeVec2(vec->x, vec->y);
        fclSpriteCreate(0x84, pos7, 217.0f, 0x40);
        fclSpriteStartAlphaAt(0x84, 0, 0xFF, 0, 0xA, 0);
        vec = (FclVec2 *)D_00640C58;
        pos8 = fclMakeVec2(vec->x, vec->y);
        fclSpriteCreate(0x85, pos8, 218.0f, 0x40);
        fclSpriteStartAlphaAt(0x85, 0, 0xFF, 0, 0xA, 0);
        fclCombineLayoutDetailSprites(task, 0);
        fclCombineSlideInSlotSprites(task, 0x92, 0);
        fclCombineFadeLabelSprites(task, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0);
        p[0x138] = 0;
        if (datGetFlag(0x1305) != 0 && datGetFlag(0x1308) == 0) {
            p[0x138] |= 2;
        }
        if (fclCombineClaimSlot(task) == 0) {
            p[1] = 0x18;
        }
        break;
    case 0x18:
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(0x1F4))->flags, 1) == 1) {
            break;
        }
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(0x1F6))->flags, 1) == 1) {
            break;
        }
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(0x1F8))->flags, 1) == 1) {
            break;
        }
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(0x1FA))->flags, 1) == 1) {
            break;
        }
        if (fclCombineRowsBusy(task) == 1) {
            break;
        }
        buttons = D_008C027A[0];
        if (buttons & 0x1000) {
            fclCombineStepSlotSelection(task, 1);
            break;
        }
        if (buttons & 0x4000) {
            fclCombineStepSlotSelection(task, 0);
            break;
        }
        buttons = D_008C024E[0];
        if (buttons & 0x40) {
            sdkSndStartStream(0, 0, 0, 1);
            for (n = 0; n < 3; n++) {
                off = n * 2;
                pos9 = fclMakeVec2(26.0f, (f32)(n * 34 + 87));
                fclCombineDrawListRow(task, n, pos9, ((FclCombineWork *)p)->selectedIds[n], (s16)off, 1);
            }
            switch (p[0xB3]) {
            case 0:
                if (datGetFlag(0x131E) == 0) {
                    p[0xD] = nmMesWindowAcquire((void *)fclCombineGetMesWindow());
                    nmMesWindowShowSlotRecord(p[0xD], 0x4D);
                    datSetFlag(0x131E, 1);
                    p[1] = 0x22;
                } else {
                    fclCombineBuildRows(task);
                }
                break;
            case 1:
                if (datGetFlag(0x131B) == 0) {
                    p[0xD] = nmMesWindowAcquire((void *)fclCombineGetMesWindow());
                    nmMesWindowShowSlotRecord(p[0xD], 0x4F);
                    datSetFlag(0x131B, 1);
                    p[1] = 0x21;
                } else {
                    vec = (FclVec2 *)D_00641660;
                    pos10 = fclMakeVec2(vec->x, vec->y);
                    fclSpriteCreate(0x1C6, pos10, 242.0f, 0x3D);
                    fclSpriteStartAlphaAt(0x1C6, 0, 0x64, 0, 0xA, 0);
                    p[0xB6] = 0;
                    fclCombineDrawActionList(task);
                    fclCombineSlideInSlotSprites(task, 0x96, 0x92);
                    p[1] = 0x1B;
                }
                break;
            case 2:
                fclCombineLayoutDetailSprites(task, 1);
                fclCombineCreateSlotIcons(task, 1);
                fclCombineSlideInSlotSprites(task, 0, 0x92);
                fclCombineFadeLabelSprites(task, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0);
                fclSpriteStartAlphaAt(0x81, 0xFF, 0, 0, 6, 0);
                fclSpriteStartAlphaAt(0x80, 0xFF, 0, 2, 0xA, 0);
                fclSpriteStartAlphaAt(0x20C, 0xFF, 0, 2, 0xA, 0);
                fclSpriteStartAlphaAt(0x84, 0xFF, 0, 0, 6, 0);
                fclSpriteStartAlphaAt(0x85, 0xFF, 0, 0, 6, 0);
                for (n = 0; n < 3; n++) {
                    pos11 = fclMakeVec2(26.0f, (f32)(n * 34 + 87));
                    fclCombineDrawListRow(task, n, pos11, ((FclCombineWork *)p)->selectedIds[n], 0, 1);
                }
                p[1] = 0x19;
                break;
            }
        } else if (buttons & 0x20) {
            fclCombineLayoutDetailSprites(task, 1);
            fclCombineCreateSlotIcons(task, 1);
            fclCombineSlideInSlotSprites(task, 0, 0x92);
            fclCombineFadeLabelSprites(task, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0);
            fclSpriteStartAlphaAt(0x81, 0xFF, 0, 0, 6, 0);
            fclSpriteStartAlphaAt(0x80, 0xFF, 0, 2, 0xA, 0);
            fclSpriteStartAlphaAt(0x20C, 0xFF, 0, 2, 0xA, 0);
            fclSpriteStartAlphaAt(0x84, 0xFF, 0, 0, 6, 0);
            fclSpriteStartAlphaAt(0x85, 0xFF, 0, 0, 6, 0);
            for (n = 0; n < 3; n++) {
                pos12 = fclMakeVec2(26.0f, (f32)(n * 34 + 87));
                fclCombineDrawListRow(task, n, pos12, ((FclCombineWork *)p)->selectedIds[n], 0, 1);
            }
            p[1] = 0x19;
            sdkSndStartStream(0, 0, 0, 2);
        }
        break;
    case 0x22:
        if (nmMesWindowSlotStatus(p[0xD]) != 0) {
            nmMesWindowStepSlotRelease(p[0xD]);
            break;
        }
        nmMesWindowDestroySlot(p[0xD]);
        fclCombineBuildRows(task);
        break;
    case 0x21:
        if (nmMesWindowSlotStatus(p[0xD]) != 0) {
            nmMesWindowStepSlotRelease(p[0xD]);
            break;
        }
        nmMesWindowDestroySlot(p[0xD]);
        vec = (FclVec2 *)D_00641660;
        pos13 = fclMakeVec2(vec->x, vec->y);
        fclSpriteCreate(0x1C6, pos13, 242.0f, 0x3D);
        fclSpriteStartAlphaAt(0x1C6, 0, 0x64, 0, 0xA, 0);
        p[0xB6] = 0;
        fclCombineDrawActionList(task);
        fclCombineSlideInSlotSprites(task, 0x96, 0x92);
        p[1] = 0x1B;
        break;
    case 0x19:
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(0x1F4))->flags, 1) == 1) {
            break;
        }
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(0x92))->flags, 1) == 1) {
            break;
        }
        p[0] = 0xB;
        break;
    case 0x97:
        if (nmMesWindowSlotStatus(p[0xD]) != 0) {
            nmMesWindowStepSlotRelease(p[0xD]);
            break;
        }
        nmMesWindowDestroySlot(p[0xD]);
        if (fclCombineClaimSlot(task) == 0) {
            p[1] = 0x18;
        }
        break;
    case 0x1A:
        if (fclCombineRowsBusy(task) == 1) {
            break;
        }
        fclSpriteSetHidden(((FclCombineWork *)p)->headerTask.task, 0);
        fclSpriteSetHidden(((FclCombineWork *)p)->arrowTask.task, 0);
        buttons = D_008C027A[0];
        if (buttons & 0x8000) {
            p[0xB4] = fclSubClamped(p[0xB4], 1, 0, p[0xB5] - 1, 2);
            fclCombineMarkRow((*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, p[0xB4])).id);
            sdkSndStartStream(0, 0, 0, 0);
            for (n = 0; n < p[0xB5]; n++) {
                fclCombineUpdateEntryHighlight(task, n, (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, n)).id, (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, n)).x, (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, n)).y);
            }
        } else if (buttons & 0x2000) {
            p[0xB4] = fclAddClamped(p[0xB4], 1, p[0xB5] - 1, 0, 2);
            fclCombineMarkRow((*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, p[0xB4])).id);
            sdkSndStartStream(0, 0, 0, 0);
            for (n = 0; n < p[0xB5]; n++) {
                fclCombineUpdateEntryHighlight(task, n, (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, n)).id, (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, n)).x, (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, n)).y);
            }
        } else {
            buttons = D_008C024E[0];
            if (buttons & 0x40) {
                fclSpriteSetFlag(0x2F2, 0, 1);
                sdkSndStartStream(0, 0, 0, 1);
                if ((*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, p[0xB4])).id == 8) {
                    if (fclCombineClaimSlot(task) == 0) {
                        p[1] = 0x18;
                    }
                    fclCombineLayoutTransition(task, 1);
                    fclCombineDrawSlotRows(task, 0);
                    fclCombineSlideInSlotSprites(task, 0x92, 0x96);
                    fclSpriteStartAlphaAt(0x1C6, fclSpriteEntry(0x1C6)[0x6E], 0, 0, 0xA, 0);
                } else {
                    p[1] = 0x1C;
                }
            } else if (buttons & 0x20) {
                fclSpriteSetFlag(0x2F2, 0, 1);
                if (fclCombineClaimSlot(task) == 0) {
                    p[1] = 0x18;
                }
                fclCombineLayoutTransition(task, 1);
                sdkSndStartStream(0, 0, 0, 2);
                fclCombineDrawSlotRows(task, 0);
                fclCombineSlideInSlotSprites(task, 0x92, 0x96);
                fclSpriteStartAlphaAt(0x1C6, fclSpriteEntry(0x1C6)[0x6E], 0, 0, 0xA, 0);
            }
        }
        break;
    case 0x1C:
        if ((*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, p[0xB4])).id == P4_NATIVE_ROWS_VELVET_ENTRY) {
            /* The mod card: its page runs hosted in this task's Config child
             * while the root waits; a page that cannot open refuses. */
            if (p4_native_rows_velvet_open(task) != 0) {
                sdkSndStartStream(0, 0, 0, 8);
                p[1] = 0x1A;
                break;
            }
            fclCombineLayoutTransition(task, 1);
            p[1] = P4_NATIVE_ROWS_VELVET_PAGE;
            break;
        }
        fclCombineLayoutTransition(task, 1);
        p[0] = (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, p[0xB4])).state0;
        p[1] = (*(FclCombineRow *)p4_native_rows_velvet_row((u8 *)p, p[0xB4])).state1;
        break;
    case P4_NATIVE_ROWS_VELVET_PAGE:
        /* The root reads no pad until the page is closed and destroyed, then
         * the cards slide back in with the cursor on the mod card. */
        if (p4_native_rows_velvet_step()) {
            break;
        }
        fclCombineBuildRows(task);
        break;
    case 0x1B:
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry((p[0xB7] + 3) * 2 + 0x1F4))->flags, 1) == 1) {
            break;
        }
        buttons = D_008C027A[0];
        if (buttons & 0x1000) {
            sdkSndStartStream(0, 0, 0, 0);
            menu = fclCombineMapSlotKind(p[0xB6]);
            color3 = fclMakeDrawColor(0, 0, 0x66, 0xFF);
            color4 = fclMakeDrawColor(0xCC, 0xFF, 0xFF, 0xFF);
            color5 = fclMakeDrawColor(0x25, 0x2F, 0x94, 0xFF);
            fclCombineSetSlotColors(task, menu, color3, color4, color5);
            if (datGetFlag(0x1306) != 0) {
                p[0xB6] = fclSubClamped(p[0xB6], 1, 0, (s16)(p[0xB7] - 2), 2);
            } else {
                p[0xB6] = fclSubClamped(p[0xB6], 1, 0, (s16)(p[0xB7] - 1), 2);
            }
            menu = fclCombineMapSlotKind(p[0xB6]);
            color6 = fclMakeDrawColor(0xC6, 0xEE, 1, 0xFF);
            color7 = fclMakeDrawColor(0x2D, 0x2D, 0x2D, 0xFF);
            color8 = fclMakeDrawColor(0x92, 0xC8, 7, 0xFF);
            fclCombineSetSlotColors(task, menu, color6, color7, color8);
            break;
        }
        if (buttons & 0x4000) {
            sdkSndStartStream(0, 0, 0, 0);
            menu = fclCombineMapSlotKind(p[0xB6]);
            color9 = fclMakeDrawColor(0, 0, 0x66, 0xFF);
            color10 = fclMakeDrawColor(0xCC, 0xFF, 0xFF, 0xFF);
            color11 = fclMakeDrawColor(0x25, 0x2F, 0x94, 0xFF);
            fclCombineSetSlotColors(task, menu, color9, color10, color11);
            if (datGetFlag(0x1306) != 0) {
                p[0xB6] = fclAddClamped(p[0xB6], 1, (s16)(p[0xB7] - 2), 0, 2);
            } else {
                p[0xB6] = fclAddClamped(p[0xB6], 1, (s16)(p[0xB7] - 1), 0, 2);
            }
            menu = fclCombineMapSlotKind(p[0xB6]);
            color12 = fclMakeDrawColor(0xC6, 0xEE, 1, 0xFF);
            color13 = fclMakeDrawColor(0x2D, 0x2D, 0x2D, 0xFF);
            color14 = fclMakeDrawColor(0x92, 0xC8, 7, 0xFF);
            fclCombineSetSlotColors(task, menu, color12, color13, color14);
            break;
        }
        buttons = D_008C024E[0];
        if (buttons & 0x40) {
            sdkSndStartStream(0, 0, 0, 1);
            if (p[0xB6] == p[0xB7] - 1) {
                for (j = 0; j < p[0xB7]; j++) {
                    slotPos0 = ((FclCombineRowSprite *)fclSpriteEntry((j + 4) * 2 + 0x1F4))->position;
                    fclCombineDrawListRow(task, j + 4, slotPos0, ((FclCombineWork *)p)->selectedIds[j], (s16)(j * 2), 1);
                }
                fclCombineDrawSlotRows(task, 3);
                fclCombineSlideInSlotSprites(task, 0x92, 0x96);
                fclSpriteStartAlphaAt(0x1C6, fclSpriteEntry(0x1C6)[0x6E], 0, 0, 0xA, 0);
                if (fclCombineClaimSlot(task) == 0) {
                    p[1] = 0x18;
                }
            } else {
                for (j = 0; j < p[0xB7]; j++) {
                    slotPos2 = ((FclCombineRowSprite *)fclSpriteEntry((j + 4) * 2 + 0x1F4))->position;
                    fclCombineDrawListRow(task, j + 4, slotPos2, ((FclCombineWork *)p)->selectedIds[j], (s16)(j * 2), 1);
                }
                switch (p[0xB6]) {
                case 0:
                    ((u8 *)p)[1] = 0x8A;
                    p[0] = 0xF;
                    fclCombineSlideInSlotSprites(task, 0x117, 0x96);
                    break;
                case 1:
                    ((u8 *)p)[1] = 0x8A;
                    p[0] = 0x10;
                    fclCombineSlideInSlotSprites(task, 0x118, 0x96);
                    break;
                case 2:
                    if (datGetFlag(0x1305) != 0) {
                        p[0] = 8;
                        p[1] = 0x75;
                    } else {
                        p[0] = 0xA;
                        ((u8 *)p)[1] = 0xC2;
                    }
                    break;
                case 3:
                    if (datGetFlag(0x1305) != 0) {
                        p[0] = 0xA;
                        ((u8 *)p)[1] = 0xC2;
                    } else {
                        fclCombineDrawSlotRows(task, 3);
                        fclCombineSlideInSlotSprites(task, 0x92, 0x96);
                        if (fclCombineClaimSlot(task) == 0) {
                            p[1] = 0x18;
                        }
                    }
                    break;
                case 4:
                    if (datGetFlag(0x1305) != 0) {
                        fclCombineDrawSlotRows(task, 3);
                        fclCombineSlideInSlotSprites(task, 0x92, 0x96);
                        if (fclCombineClaimSlot(task) == 0) {
                            p[1] = 0x18;
                        }
                    }
                    break;
                }
            }
        } else if (buttons & 0x20) {
            for (j = 0; j < p[0xB7]; j++) {
                slotPos1 = ((FclCombineRowSprite *)fclSpriteEntry((j + 4) * 2 + 0x1F4))->position;
                fclCombineDrawListRow(task, j + 4, slotPos1, ((FclCombineWork *)p)->selectedIds[j], (s16)(j * 2), 1);
            }
            fclCombineDrawSlotRows(task, 3);
            fclSpriteStartAlphaAt(0x1C6, fclSpriteEntry(0x1C6)[0x6E], 0, 0, 0xA, 0);
            fclCombineSlideInSlotSprites(task, 0x92, 0x96);
            if (fclCombineClaimSlot(task) == 0) {
                p[1] = 0x18;
            }
            sdkSndStartStream(0, 0, 0, 2);
        }
        break;
    case 0x20:
        if (nmMesWindowSlotStatus(p[0xD]) != 0) {
            nmMesWindowStepSlotRelease(p[0xD]);
            break;
        }
        nmMesWindowDestroySlot(p[0xD]);
        for (j = 0; j < p[0xB7]; j++) {
            off = j * 2;
            pos14 = fclMakeVec2(26.0f, (f32)(j * 34 + 87));
            fclCombineDrawListRow(task, j + 4, pos14, ((FclCombineWork *)p)->selectedIds[j], (s16)off, 0);
        }
        color15 = fclMakeDrawColor(0xC6, 0xEE, 1, 0xFF);
        te = fclSpriteEntry((p[0xB6] + 4) * 2 + 0x1F5);
        ((FclCombineRowSprite *)te)->color = color15;
        ((FclCombineRowSprite *)fclSpriteEntry((p[0xB6] + 4) * 2 + 0x1F4))->color = MEMBER(FclDrawColor, te, FclCombineRowSprite, color);
        color16 = fclMakeDrawColor(0x2D, 0x2D, 0x2D, 0xFF);
        ((FclCombineRowSprite *)fclSpriteEntry(((s16 *)(p + 0xB8))[p[0xB6]]))->color = color16;
        color17 = fclMakeDrawColor(0x92, 0xC8, 7, 0xFF);
        ((FclCombineRowSprite *)fclSpriteEntry(p[0xB6] + 0x2FF))->color = color17;
        p[1] = 0x1B;
        break;
    }
}