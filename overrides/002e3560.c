/* 002e3560: same dynamic native rebuild as the global-list entrypoint. The
 * original wide order argument is narrowed by the shared body exactly once. */
#include "p4pc/game_items_native.h"
void yListRebuildFor(u8 *task, s32 filterA, s32 filterB, s8 kind, s32 order)
{
    p4_game_tables_items_native_rebuild(((KwlnTask *)task)->work, filterA, filterB, kind, order);
}
