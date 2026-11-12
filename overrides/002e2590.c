/* 002e2590: child lists use the same dynamic output/scratch capacity as the
 * global list, with identical retail task construction and work header. */
#include "p4pc/game_items_native.h"
void *yListCreateChild(s32 parent, s32 filterA, s32 filterB, s8 kind, s8 order) {
    u8 *work;
    void *handle;

    chkmemAllocSiteTag(D_0063FC48, 0x82);
    work = D_008873F4[0](1, p4_game_tables_items_native_work_bytes(), 0x40000);
    if (!work) p4_game_tables_items_native_refuse(P4_TABLE_NOMEM);
    handle = (void *)(s32)kwlnTaskCreateChild((void *)(parent), (const void *)(D_0063FC58), 0xF, 0, 0, comuDataApplyOwnedListRebuild, comuDataDestroyListRequest, (u8 *)(work));
    ((YListItemWork *)(work))->filterA = filterA;
    ((YListItemWork *)(work))->filterB = filterB;
    ((YListItemWork *)(work))->kind = kind;
    ((YListItemWork *)(work))->enabled = 1;
    ((YListItemWork *)(work))->order = order;
    return handle; /* 002e2624 retains v0 across the final work-header stores. */
}
