#include "p4pc/mod_entry.h"

/* 002ecfc0: retail fusion command rows. A fusion kind whose p4.authorize
 * "fusion" authorizer denies it is not offered; with no fusion authorizer the
 * one mask test leaves exactly the retail rows. Browsing rows (6, 7) and Back
 * (8) are never gated. */
static s32 fclCombineFusionRowAllowed(s32 kind, s32 materials);
void fclCombineBuildRows(u8 *task) {
    FclCombineWork *rowMenu;
    FclVec2 position;
    u8 *menuWork;
    FclVec2f *base;

    menuWork = ((u8*)((KwlnTask *)task)->work);
    rowMenu = (FclCombineWork *)menuWork;
    base = (FclVec2f *)D_00641660;
    position = fclMakeVec2(base->x, base->y);
    fclSpriteCreate(0x1C6, position, 242.0f, 0x3D);
    fclSpriteStartAlphaAt(0x1C6, 0, 0x64, 0, 0xA, 0);
    ((FclCombineWork *)(menuWork))->commandCursor = 0;
    ((FclCombineWork *)(menuWork))->commandCount = 0;
    if (fclCombineFusionRowAllowed(0, 2)) {
        rowMenu->commandRows[rowMenu->commandCount].state0 = 1;
        rowMenu->commandRows[rowMenu->commandCount].state1 = 0x23;
        rowMenu->commandRows[rowMenu->commandCount].id = 0;
        ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    }
    if (fclCombineFusionRowAllowed(1, 3)) {
        rowMenu->commandRows[rowMenu->commandCount].state0 = 2;
        rowMenu->commandRows[rowMenu->commandCount].state1 = 0x38;
        rowMenu->commandRows[rowMenu->commandCount].id = 1;
        ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    }
    if (datGetFlag(0x1301) != 0 && fclCombineFusionRowAllowed(2, 4)) {
        rowMenu->commandRows[rowMenu->commandCount].state0 = 3;
        rowMenu->commandRows[rowMenu->commandCount].state1 = 0x51;
        rowMenu->commandRows[rowMenu->commandCount].id = 2;
        ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    }
    if (datGetFlag(0x1302) != 0 && fclCombineFusionRowAllowed(3, 5)) {
        rowMenu->commandRows[rowMenu->commandCount].state0 = 4;
        rowMenu->commandRows[rowMenu->commandCount].state1 = 0x51;
        rowMenu->commandRows[rowMenu->commandCount].id = 3;
        ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    }
    if (datGetFlag(0x1303) != 0 && fclCombineFusionRowAllowed(4, 6)) {
        rowMenu->commandRows[rowMenu->commandCount].state0 = 5;
        rowMenu->commandRows[rowMenu->commandCount].state1 = 0x51;
        rowMenu->commandRows[rowMenu->commandCount].id = 4;
        ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    }
    if (datGetFlag(0x1304) != 0 && fclCombineFusionRowAllowed(5, 12)) {
        rowMenu->commandRows[rowMenu->commandCount].state0 = 6;
        rowMenu->commandRows[rowMenu->commandCount].state1 = 0x63;
        rowMenu->commandRows[rowMenu->commandCount].id = 5;
        ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    }
    rowMenu->commandRows[rowMenu->commandCount].state0 = 7;
    rowMenu->commandRows[rowMenu->commandCount].state1 = 0x6E;
    rowMenu->commandRows[rowMenu->commandCount].id = 6;
    ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    rowMenu->commandRows[rowMenu->commandCount].state0 = 9;
    rowMenu->commandRows[rowMenu->commandCount].state1 = 0xC2;
    rowMenu->commandRows[rowMenu->commandCount].id = 7;
    ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    rowMenu->commandRows[rowMenu->commandCount].state0 = 0xB;
    rowMenu->commandRows[rowMenu->commandCount].state1 = 0xD8;
    rowMenu->commandRows[rowMenu->commandCount].id = 8;
    ((FclCombineWork *)(menuWork))->commandCount = ((FclCombineWork *)(menuWork))->commandCount + 1;
    fclCombineLayoutTransition(task, 0);
    fclCombineSlideInSlotSprites(task, 0x96, 0x92);
    ((FclCombineWork *)(menuWork))->f1 = 0x1A;
}

static s32 fclCombineFusionRowAllowed(s32 kind, s32 materials)
{
    return !(p4_mod_authorize_mask & P4_MOD_AUTH_FUSION_BIT) || p4_mod_authorize_fusion(kind, materials);
}
