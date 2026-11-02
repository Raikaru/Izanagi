#include "p4pc/game_items.h"
#include <stddef.h>

typedef struct ItemListHeader {
    uint8_t enabled;
    int8_t kind;
    int16_t count;
    int32_t filter_a, filter_b;
    int8_t order;
    uint8_t opaque_0d;
} ItemListHeader;
typedef struct ItemListRow { int16_t id, quantity; } ItemListRow;
_Static_assert(offsetof(ItemListHeader, filter_a) == 4, "retail list filter offset");
_Static_assert(offsetof(ItemListHeader, order) == 12, "retail list order offset");

extern int32_t datItemCount(int16_t);
extern uint32_t datEntryGetFlags(int16_t);
extern uint32_t datEntryCategoryMask(int16_t);
extern uint32_t datItemMoonGateFirst(int16_t);
extern uint32_t datItemMoonGateSecond(int16_t);
extern int32_t datEntryEvaluateFlags(int16_t);
extern uint32_t datGetFlag(int32_t);
extern int32_t fclMoonPhaseAtLeast(int32_t, int32_t);
extern int32_t fclItemIsDateAvailable(int16_t);
extern int32_t fclAddClamped(int32_t, int32_t, int32_t, int32_t, int8_t);
extern uint8_t clndGetMoonPhase(int32_t);
extern void datSetStateTableEntry(int32_t, uint8_t);
extern int32_t datItemPriceCompare(const void *, const void *);
extern void func_00440bb8(char *, uint32_t, uint32_t, int (*)(const void *, const void *));

/* Same predicate as y_list.c's yListAcceptsItem, now shared by both native
 * rebuild consumers. No mod-specific category or registry branch. */
static int accepts(const ItemListHeader *work, int16_t id)
{
    if (!work->filter_a && !work->filter_b) return 1;
    if (!work->filter_a && (work->filter_b & datEntryCategoryMask(id))) return 1;
    if (!work->filter_b && (work->filter_a & datEntryGetFlags(id))) return 1;
    return (work->filter_a & datEntryGetFlags(id)) && (work->filter_b & datEntryCategoryMask(id));
}
static void increment(ItemListHeader *work)
{
    work->count = (int16_t)fclAddClamped(work->count, 1, 0, 0, 0);
}
void p4_game_tables_items_native_rebuild(uint8_t *buffer, int32_t filter_a, int32_t filter_b, int8_t kind, int32_t order)
{
    ItemListHeader *work = (ItemListHeader *)buffer;
    ItemListRow *rows = (ItemListRow *)(buffer + 0x0e);
    int16_t *ids = (int16_t *)(buffer + 0x0e);
    P4GameItems *items = p4_game_tables_items_active();
    uint32_t added = items ? (uint32_t)p4_game_tables_items_count(items) : 0;
    const P4GameItemView *new_rows = added ? p4_game_tables_items_at(items, 0) : NULL;
    uint32_t total = kind >= 1 && kind <= 4 ? P4_ITEM_RETAIL_COUNT + added : 0;
    uint32_t capacity = p4_game_tables_items_native_list_capacity();
    /* Expanded constructors reserve one scratch array after their output rows.
     * Retail allocation remains exactly 0x1810; its original scratch stays on
     * the task's guest stack. No per-rebuild heap allocation or duplicate sort. */
    int32_t retail_scratch[added ? 1 : P4_ITEM_RETAIL_COUNT];
    int32_t *scratch = added ? (int32_t *)(buffer + 0x10u + capacity * 4u) : retail_scratch;
    work->filter_a = filter_a; work->filter_b = filter_b; work->kind = kind; work->count = 0;
    if (kind == 4) increment(work);
    for (uint32_t index = 0; index < total; ++index) {
        int16_t id = (int16_t)(index < P4_ITEM_RETAIL_COUNT ? index : new_rows[index - P4_ITEM_RETAIL_COUNT].id);
        switch (kind) {
        case 1:
            rows[index].quantity = 0; rows[index].id = 0;
            if ((datItemCount(id) & 255) > 0 && accepts(work, id) == 1 && !(datEntryCategoryMask(id) & 0x2000)) {
                rows[work->count].id = id; rows[work->count].quantity = (int16_t)(datItemCount(id) & 255);
                increment(work);
            }
            break;
        case 2:
            if (accepts(work, id) == 1) {
                int gear;
                if (id < (int32_t)P4_ITEM_RETAIL_COUNT) {
                    int16_t category = (int16_t)datEntryEvaluateFlags(id);
                    gear = category != 0x10 && category != 0x11 && category != 0x12;
                } else {
                    uint32_t policy = p4_game_tables_items_native_stock_policy((uint16_t)id);
                    if (policy == P4_ITEM_STOCK_NONE) break;
                    gear = policy == P4_ITEM_STOCK_REQUIREMENTS;
                }
                int accepted = 0;
                if (gear) {
                    if (datGetFlag(0x1462) == 0) {
                        int32_t value = (int32_t)p4_game_tables_items_native_requirement_item(id, 0, datItemMoonGateFirst(id));
                        if (fclMoonPhaseAtLeast(value, (int32_t)p4_game_tables_items_native_requirement_quantity(id, 0, datItemMoonGateFirst(id))) == 1) {
                            value = (int32_t)p4_game_tables_items_native_requirement_item(id, 1, datItemMoonGateSecond(id));
                            accepted = fclMoonPhaseAtLeast(value, (int32_t)p4_game_tables_items_native_requirement_quantity(id, 1, datItemMoonGateSecond(id))) == 1;
                        }
                    } else accepted = 1;
                } else if (datGetFlag(0x1462) == 0) accepted = fclItemIsDateAvailable(id) == 1;
                else accepted = 1;
                if (accepted) {
                    rows[index].quantity = 0; rows[work->count].id = id; increment(work);
                }
            }
            break;
        case 3:
            ids[index] = 0;
            if (accepts(work, id) == 1) { ids[work->count] = id; increment(work); }
            break;
        case 4:
            rows[index].quantity = 0; rows[index].id = 0;
            if ((datItemCount(id) & 255) > 0 && accepts(work, id) == 1) {
                rows[work->count].id = id; rows[work->count].quantity = (int16_t)(datItemCount(id) & 255);
                increment(work);
            }
            break;
        default:
            break;
        }
    }
    int8_t mode = (int8_t)order;
    if (mode == 1) {
        int32_t output = 0;
        for (uint32_t i = 0; i < capacity; ++i) scratch[i] = -1;
        for (int32_t i = 0; i < work->count; ++i) {
            if (clndGetMoonPhase(rows[i].id) & 1) {
                scratch[output++] = rows[i].id;
                int32_t value = clndGetMoonPhase(rows[i].id);
                datSetStateTableEntry(rows[i].id, (uint8_t)(value | 2));
                rows[i].id = -1; rows[i].quantity = 1;
            }
        }
        for (int32_t i = 0; i < work->count; ++i) {
            if (rows[i].id != -1) {
                scratch[output++] = rows[i].id; rows[i].quantity = 0;
            }
        }
        for (int32_t i = 0; i < work->count; ++i) rows[i].id = (int16_t)scratch[i];
    }
    if (mode == 2) {
        for (int32_t i = 0; i < work->count; ++i) scratch[i] = rows[i].id;
        /* Preserve the actual retail BSD/newlib qsort and comparison/tie order,
         * not host libc qsort, which can change stable walk/menu results. */
        func_00440bb8((char *)scratch, (uint32_t)work->count, 4, datItemPriceCompare);
        for (int32_t i = 0; i < work->count; ++i) {
            rows[i].id = (int16_t)scratch[i]; rows[i].quantity = 0;
            if (clndGetMoonPhase(rows[i].id) & 1) {
                rows[i].quantity = 1;
                int32_t value = clndGetMoonPhase(rows[i].id);
                datSetStateTableEntry(rows[i].id, (uint8_t)(value | 2));
            }
        }
    }
}
