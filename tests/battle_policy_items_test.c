/* The policy must use the native owner's item rows, not the inline panel. */
#include "../src/platform/battle_sim/policy_items.h"
#undef NDEBUG
#include <assert.h>

static uint32_t extra;
static P4GameItemBattleRow *native_rows;
uint32_t p4_game_tables_items_native_row_count(void) { return P4_ITEM_RETAIL_COUNT + extra; }
P4GameItemBattleRow *p4_game_tables_items_native_battle_rows(uint8_t *panel)
{
    (void)panel;
    return native_rows;
}

int main(void)
{
    uint8_t panel[0x5a8] = {0};
    P4GameItemBattleRow inline_rows[256] = {{0}}, relocated[262] = {{0}};
    inline_rows[0].id = 769;
    relocated[0].id = 770;
    native_rows = inline_rows;
    assert(p4_battle_policy_item_rows(panel, 1)[0].id == 769);
    assert(!p4_battle_policy_item_rows(panel, 257));
    extra = 6;
    native_rows = relocated;
    assert(p4_battle_policy_item_rows(panel, 1)[0].id == 770);
    relocated[261].id = 1700;
    assert(p4_battle_policy_item_rows(panel, 262)[261].id == 1700);
    assert(!p4_battle_policy_item_rows(panel, 263));
    return 0;
}
