#include "p4pc/mod_entry.h"

/* 002e7ac0: the Velvet Room menu's mode dispatcher. A p4.authorize "fusion"
 * denial of kind 6 (roster pair fusion, the root menu's third row with flag
 * 0x1305) turns its entry back to the root menu before any of its state runs;
 * with no fusion authorizer the one mask test leaves the retail dispatch.
 *
 * A mod's Velvet command (p4.velvet.command, include/p4pc/mod_velvet.h): while
 * the root's three-row list waits for input (f0 0, f1 0x18, its row sprites
 * settled and no row busy), its hint is drawn under the list and Square, which
 * no root state reads, opens the command's page in the real Config child of
 * this task (f0 0x40, a state retail never uses). The root reads no input
 * until the page is done and its child destroyed; then the root resumes its
 * input loop. With no command registered the one mask test leaves the retail
 * dispatch. */
#define VELVET_HOSTED 0x40

static s32 velvetIdle(u8 *task, u8 *work)
{
    u8 *fclSpriteEntry(s16 index);
    s16 fclSpriteFlagBit(s16 flags, s16 bit);
    s32 fclCombineRowsBusy(u8 *task);
    s16 sprite;

    if (((FclCombineWork *)work)->f1 != 0x18) {
        return 0;
    }
    for (sprite = 0x1F4; sprite <= 0x1FA; sprite += 2) {
        if ((s16)fclSpriteFlagBit(((FclCombineRowSprite *)fclSpriteEntry(sprite))->flags, 1) == 1) {
            return 0;
        }
    }
    return fclCombineRowsBusy(task) != 1;
}

/* At the idle root with the command offered: draws its hint and returns 1 on
 * a Square edge. The offer's verdict is cached in port code from the first idle
 * frame until rearm (any frame off the idle root), so the weapon_fusion offer
 * authorizer runs once per idle streak, not every frame. */
static s32 velvetCommand(u8 *task, u8 *work)
{
    extern u16 D_008C024E[];
    extern u8 D_00795E60[];
    s32 fclPackRgba(u8, u8, u8, u8);
    int frFontExDrawTextDeferredOutB(f32, f32, f32, int, s8, int, const char *, int, int, void *, int);

    if (!velvetIdle(task, work)) {
        p4_mod_velvet_rearm();
        return 0;
    }
    if (!p4_mod_velvet_offer(P4_H2G(task))) {
        return 0;
    }
    frFontExDrawTextDeferredOutB(40.0f, 197.0f, 43.0f, fclPackRgba(0xCC, 0xFF, 0xFF, 0xFF), 0, 2,
                                 (const char *)P4_G2H(p4_mod_velvet_hint_text()), 0, 0, D_00795E60, 0x15);
    return (D_008C024E[0] & 0x80) != 0;
}

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
    s32 open;

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
    if (state == VELVET_HOSTED) {
        goto case_velvet;
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
        /* The root runs its frame first (it never reads Square); the page opens
         * only if the root is still idle at the three-row list afterwards. */
        open = p4_mod_velvet_mask && velvetCommand(task, work);
        fclCombineMenuRoot(task);
        if (open && ((FclCombineWork *)work)->f0 == 0 && ((FclCombineWork *)work)->f1 == 0x18 &&
            p4_mod_velvet_open_page(P4_H2G(task)) == 0) {
            sdkSndStartStream(0, 0, 0, 1);
            ((FclCombineWork *)work)->f0 = VELVET_HOSTED;
        }
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
case_velvet:
    /* The hosted page: the root reads nothing until it is done and destroyed. */
    if (p4_mod_velvet_page_step()) {
        goto done;
    }
    ((FclCombineWork *)work)->f0 = 0;
    ((FclCombineWork *)work)->f1 = 0x18;
    goto done;
return_minus_one:
    return -1;
done:
    return 0;
}
