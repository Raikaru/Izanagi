/* Battle policy view of the native, possibly relocated, item menu. */
#ifndef P4_BATTLE_POLICY_ITEMS_H
#define P4_BATTLE_POLICY_ITEMS_H
#include "p4pc/game_items_native.h"
#include <stddef.h>

static inline const P4GameItemBattleRow *p4_battle_policy_item_rows(uint8_t *panel, unsigned count)
{
    const uint32_t total = p4_game_tables_items_native_row_count();
    const uint32_t capacity = 256u + (total > P4_ITEM_RETAIL_COUNT ? total - P4_ITEM_RETAIL_COUNT : 0u);
    return count <= capacity ? p4_game_tables_items_native_battle_rows(panel) : NULL;
}
#endif
