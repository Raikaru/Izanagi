/* 002e24a0: retail allocation stays0x1810; expanded output and sorting
 * scratch grow together in the same work allocation, never past retail bounds. */
#include "p4pc/game_items_native.h"
void yListCreate(s32 filterA, s32 filterB, s8 kind, s8 order) {
    u8 *work;

    if (iGpffffb58c != NULL) {
        yListDestroy();
    }
    chkmemAllocSiteTag(D_0063FC48, 0x67);
    work = D_008873F4[0](1, p4_game_tables_items_native_work_bytes(), 0x40000);
    if (!work) p4_game_tables_items_native_refuse(P4_TABLE_NOMEM);
    iGpffffb58c = (u8 *)(s32)kwlnTaskCreate((const void *)(D_0063FC58), 0xF, 0, 0, comuDataApplyListRebuild, comuDataDestroyListRequest, (u8 *)(work));
    ((YListItemWork *)(work))->filterA = filterA;
    ((YListItemWork *)(work))->filterB = filterB;
    ((YListItemWork *)(work))->kind = kind;
    ((YListItemWork *)(work))->enabled = 1;
    ((YListItemWork *)(work))->order = order;
}
