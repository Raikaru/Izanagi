/* 002e2a10: output, row iteration, flags and sort storage share one native
 * implementation; retail filtering/order/clamp behavior remains unchanged. */
#include "p4pc/game_items_native.h"
void yListRebuild(s32 filterA, s32 filterB, s8 kind, s8 order)
{
    u8 *task = iGpffffb58c;
    if (task == NULL) return;
    p4_game_tables_items_native_rebuild(((KwlnTask *)task)->work, filterA, filterB, kind, order);
}
