#include "p4pc/game_items.h"
#include "p4pc/gmem.h"

extern int32_t datItemCount(int16_t item);
extern int32_t datCalcIsFlaggedPersonaSkill(int32_t item);

_Static_assert(sizeof(P4GameItemBattleRow) == 4, "native item row ABI");

uint32_t p4_game_tables_items_native_battle_work_bytes(void)
{
    /* 002011c0 owns and 00200f30 frees the entire UI task work. The panel
     * ends at650, but render nodes and other components follow it througha20:
     * append after the WHOLE work allocation, not after its inline items. */
    const uint32_t extras = (uint32_t)p4_game_tables_items_count(p4_game_tables_items_active());
    return extras ? 0xa20u + (256u + extras) * sizeof(P4GameItemBattleRow) : 0xa20u;
}
P4GameItemBattleRow *p4_game_tables_items_native_battle_rows(uint8_t *panel)
{
    /* panel is taskWork+9c. Retail items start at1a6; expanded storage is at
     * taskWork+a20. Publication is boot-only, so this choice cannot change
     * while an owning UI task/selection or queued row callback is alive. */
    return (P4GameItemBattleRow *)(panel + (p4_game_tables_items_native_expanded() ? 0xa20u - 0x9cu : 0x1a6u));
}
static uint16_t append(P4GameItemBattleRow *rows, uint16_t count, uint32_t id)
{
    if (datItemCount((int16_t)id) && datCalcIsFlaggedPersonaSkill((int32_t)id)) {
        rows[count].id = (int16_t)id;
        /* Keep the original second quantity query after the ID store. */
        rows[count].quantity = (uint8_t)datItemCount((int16_t)id);
        ++count;
    }
    return count;
}
uint16_t p4_game_tables_items_native_battle_refresh(uint8_t *panel)
{
    P4GameItemBattleRow *rows = p4_game_tables_items_native_battle_rows(panel);
    uint16_t count = 0;
    for (uint32_t id = 0x300u; id < 0x400u; ++id) count = append(rows, count, id);
    const uint32_t total = p4_game_tables_items_native_row_count();
    for (uint32_t index = P4_ITEM_RETAIL_COUNT; index < total; ++index)
        count = append(rows, count, p4_game_tables_items_native_id_at(index));
    return count;
}
