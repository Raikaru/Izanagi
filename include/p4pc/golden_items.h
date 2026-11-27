/* RE-3 item classification and transactions. No game data or UI admission rules. */
#ifndef P4PC_GOLDEN_ITEMS_H
#define P4PC_GOLDEN_ITEMS_H
#include "p4pc/golden_domains.h"
#include "p4pc/golden_source.h"
#include "p4pc/golden_items_native.h"

int32_t p4_golden_item_category(uint32_t kind);
uint32_t p4_golden_item_category_count(uint32_t category);
int32_t p4_golden_item_category_id(uint32_t category, uint32_t index);
int32_t p4_golden_item_aux_index(uint32_t kind, uint32_t id);
int32_t p4_golden_item_shop_category(uint32_t selector);
int32_t p4_golden_item_camp_group(uint32_t exact_flags);
int p4_golden_item_sell_group(uint32_t group);
int32_t p4_golden_item_help(uint32_t flags);
P4GoldenItemRoute p4_golden_item_route(uint32_t kind, uint32_t golden_skill,
                                     const uint8_t *skill1);
/* Exactly one decrement only on success. Return items require the separate
 * field-state/refund result; contextual interactions are not camp effects. */
int p4_golden_item_consume(uint8_t *quantity, unsigned successes, int refund);
/* Caller supplies an admitted persona/task result (U1/M11). Pending, cancel,
 * missing old skill and invalid data never mutate either store. Skill IDs and
 * persona slots are already port IDs; never remap a converted skill twice.
 * Append at the first zero if available; full slots replace first matching old.
 * Returns slot 0..7, -1 cancel/pending/not found, -2 structural invalid input. */
int p4_golden_item_teach(const P4GoldenDomainMap *map, uint32_t kind,
    uint32_t port_skill, uint16_t *slots, uint8_t *quantity,
    int complete, int success, uint16_t old_skill);

typedef struct P4GoldenItemSortGroup {
    uint32_t flags;
    P4GoldenSpan ranges; /* inclusive little-endian u16 pairs, in source order */
} P4GoldenItemSortGroup;
typedef struct P4GoldenItemSort {
    P4GoldenItemSortGroup camp[12], equipment[4];
} P4GoldenItemSort;
/* Borrowed spans: keep the verified cmpTable cache bytes alive. */
int p4_golden_item_sort_open(P4GoldenItemSort *, const uint8_t *, size_t);
typedef uint32_t (*P4GoldenItemRead)(void *, uint32_t);
typedef int (*P4GoldenItemEmit)(void *, uint32_t, uint32_t);
/* selector -1 camp, 0..5 generic sell selectors. Permission/factor helpers
 * remain U2: callers must filter quantity for sell permission before emitting.
 * Materials prepend (UINT32_MAX,0), an aggregate row, not an owned item.
 * Emit failure stops; no sorting/flattening of game-owned range records. */
int p4_golden_item_list(const P4GoldenItemSort *, int selector,
    P4GoldenItemRead flags, P4GoldenItemRead quantity, P4GoldenItemEmit emit, void *);
#endif
