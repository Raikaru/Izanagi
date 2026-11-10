#include "p4pc/mod_entry.h"

/* 002e7ac0: the Velvet Room menu's mode dispatcher. A p4.authorize "fusion"
 * denial of kind 6 (roster pair fusion, the root menu's third row with flag
 * 0x1305) turns its entry back to the root menu before any of its state runs;
 * with no fusion authorizer the one mask test leaves the retail dispatch. */
s32 fclCombineMenuUpdate(u8 *task)
{
    void fclCombineMenuRoot(u8 *task);
    s32 sdkSndStartStream(s16 index, s16 stream, s16 value, s16 flags);
    u8 *fclSpriteEntry(s16 index);
    s16 fclSpriteFlagBit(s16 task, s16 );
    s32 fclPackRgba(u8, u8, u8, u8);
    u32 datPersonaNamePart1(s16 unitId);
    
    void fclDrawSetFlag(u8 *task, s8 );
    void fclCombineMenuFusion(u8 *task);
    void fclCombineMenuFusionTriple(u8 *task);
    void fclCombineMenuMaterialPick(u8 *task);
    void fclCombineMenuPreview(u8 *task);
    void fclCombineMenuPriceInfo(u8 *task);
    void fclCombineMenuDetailPanels(u8 *task);
    void fclCombineMenuRosterFusion(u8 *task);
    void fclCombineMenuResult(u8 *task);
    void fclCombineMenuMaterialGrid(u8 *task);
    void fclCombineMenuOptionPick(u8 *task);
    void fclCombineMenuResultGrowth(u8 *task);
    void fclCombineMenuFinish(u8 *task);
    extern u8 D_00794E70[];
    u8 *work;
    u8 state;

    work = ((struct KwlnTask *)task)->work;
    state = (u8)((FclCombineWork *)work)->f0;
    if (state == 0xD8) {
        goto done;
    }
    if (state == 0x0B) {
        goto return_minus_one;
    }
    if (state == 0x0E) {
        goto case_0e;
    }
    if (state == 0x0D) {
        goto case_0d;
    }
    if (state == 0x0A) {
        goto case_0a;
    }
    if (state == 0x09) {
        goto case_09;
    }
    if (state == 0x0C) {
        goto case_0c;
    }
    if (state == 0x10) {
        goto case_10;
    }
    if (state == 0x0F) {
        goto case_10;
    }
    if (state == 0x08) {
        goto case_08;
    }
    if (state == 0x07) {
        goto case_07;
    }
    if (state == 0x06) {
        goto case_06;
    }
    if (state == 0x05) {
        goto case_05;
    }
    if (state == 0x04) {
        goto case_04;
    }
    if (state == 0x03) {
        goto case_03;
    }
    if (state == 0x02) {
        goto case_02;
    }
    if (state == 0x01) {
        goto case_01;
    }
    switch (state) {
    case 0:
        fclCombineMenuRoot(task);
        break;
    default:
        break;
    }
    goto done;

case_01:
    P4_002E7AC0_DRAW();
    fclDrawSetFlag(((FclCombineWork *)work)->f148, 0);
    ((FclCombineWork *)work)->fusionCount = 2;
    fclCombineMenuFusion(task);
    goto done;
case_02:
    P4_002E7AC0_DRAW();
    fclDrawSetFlag(((FclCombineWork *)work)->f148, 0);
    ((FclCombineWork *)work)->fusionCount = 3;
    fclCombineMenuFusionTriple(task);
    goto done;
case_03:
    P4_002E7AC0_DRAW();
    ((FclCombineWork *)work)->fusionCount = 4;
    fclDrawSetFlag(((FclCombineWork *)work)->f148, 0);
    fclCombineMenuMaterialPick(task);
    goto done;
case_04:
    P4_002E7AC0_DRAW();
    ((FclCombineWork *)work)->fusionCount = 5;
    fclDrawSetFlag(((FclCombineWork *)work)->f148, 0);
    fclCombineMenuMaterialPick(task);
    goto done;
case_05:
    P4_002E7AC0_DRAW();
    ((FclCombineWork *)work)->fusionCount = 6;
    fclDrawSetFlag(((FclCombineWork *)work)->f148, 0);
    fclCombineMenuMaterialPick(task);
    goto done;
case_06:
    P4_002E7AC0_DRAW();
    ((FclCombineWork *)work)->fusionCount = 7;
    fclDrawSetFlag(((FclCombineWork *)work)->f148, 0);
    fclCombineMenuPreview(task);
    goto done;
case_07:
    fclCombineMenuDetailPanels(task);
    goto done;
case_08:
    /* Roster pair fusion (flag 0x1305) writes a fused Persona and removes both
     * sources. A denied fusion never enters it: the root menu resumes its
     * input loop (0x1B) with the cancel cue, as the mode's own exit does. */
    if (((FclCombineWork *)work)->f1 == 0x75 && (p4_mod_authorize_mask & P4_MOD_AUTH_FUSION_BIT) &&
        !p4_mod_authorize_fusion(6, 2)) {
        ((FclCombineWork *)work)->f0 = 0;
        ((FclCombineWork *)work)->f1 = 0x1B;
        sdkSndStartStream(0, 0, 0, 2);
        goto done;
    }
    P4_002E7AC0_DRAW();
    fclCombineMenuRosterFusion(task);
    goto done;
case_10:
    fclDrawSetFlag(((FclCombineWork *)work)->f148, 1);
    fclCombineMenuPriceInfo(task);
    goto done;
case_0c:
    fclCombineMenuResultGrowth(task);
    goto done;
case_09:
    fclCombineMenuMaterialGrid(task);
    goto done;
case_0a:
    fclCombineMenuOptionPick(task);
    goto done;
case_0d:
    fclCombineMenuResult(task);
    goto done;
case_0e:
    fclCombineMenuFinish(task);
    goto done;
return_minus_one:
    return -1;
done:
    return 0;
}
