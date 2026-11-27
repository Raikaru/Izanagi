#ifndef P4PC_GOLDEN_ITEMS_NATIVE_H
#define P4PC_GOLDEN_ITEMS_NATIVE_H
#include <stdint.h>
typedef enum P4GoldenItemRoute {
    P4_GOLDEN_ITEM_UNAVAILABLE, P4_GOLDEN_ITEM_EQUIPMENT,
    P4_GOLDEN_ITEM_FIELD, P4_GOLDEN_ITEM_RETURN,
    P4_GOLDEN_ITEM_TEACH, P4_GOLDEN_ITEM_CONTEXT
} P4GoldenItemRoute;
/* Scalar native bridges; no host pointers are stored in guest work. */
int32_t p4_golden_item_native_category(uint32_t item);
int32_t p4_golden_item_native_aux_index(uint32_t item);
int32_t p4_golden_item_native_route(uint32_t item);
uint16_t p4_golden_item_native_inventory(void *rows, uint32_t capacity);
int p4_golden_item_native_teach(uint32_t item, uint16_t *persona_slots,
    int complete, int success, uint16_t old_skill);
_Noreturn void p4_golden_item_native_return_pending(void);
void p4_golden_items_reset(void);
#endif
