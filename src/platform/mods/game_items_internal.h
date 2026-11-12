#ifndef P4PC_GAME_ITEMS_INTERNAL_H
#define P4PC_GAME_ITEMS_INTERNAL_H
#include "p4pc/game_items.h"
/* Internal O(1) slot. Native consumers update bytes without copying a public
 * serializer record. row is the immutable one-based descriptor index. */
typedef struct P4GameItemMutable { uint16_t row; uint8_t quantity, availability; } P4GameItemMutable;
P4GameItemMutable *p4_game_tables_items_mutable(const P4GameItems *items, uint32_t id);
typedef int (*P4GameItemModelCheck)(void *user, uint32_t right, uint32_t left, uint32_t category_mask);
int p4_game_tables_items_check_models(const P4GameItems *items, P4GameItemModelCheck check, void *user);
int p4_game_tables_items_action_dispatch(P4GameItems *items, uint32_t id);
int p4_game_tables_items_action_bound(const P4GameItems *items, uint32_t category);
/* Actual materialized bindings only; CPU definitions never contain streams. */
uint32_t p4_game_tables_items_text_binding(const P4GameItems *items,
    uint32_t table, uint32_t id, uint32_t role);
int p4_game_tables_items_check_prefix(const P4GameItems *items, const uint8_t *bytes, size_t size);
int p4_game_tables_items_text_runtime_use(const P4GameItems *items, uint32_t table, uint32_t id, uint32_t role);
/* Owning save decoder validates immutable equipment before any commit. */
int p4_game_tables_items_equipment_check(const P4GameItems *items,
    uint32_t unit, uint32_t slot, uint32_t id);
/* Only validated, canonical expanded records; exact materialized view must
 * outlive the transaction. Allocation-free commit, never retail arrays. */
void p4_game_tables_items_state_apply(P4GameItems *items,
    const P4GameItemState *rows, size_t count, const uint32_t equipment[10][3]);
#endif
