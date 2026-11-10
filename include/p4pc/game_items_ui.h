#ifndef P4PC_GAME_ITEMS_UI_H
#define P4PC_GAME_ITEMS_UI_H
#include "p4pc/game_items.h"
#include "p4pc/mod_resources.h"
/* UI300-only product. Metadata retains stable image references; this owner
 * resolves the exact actual Resources200 service and pins its real tokens.
 * Items/resources outlive this product. No active/global service fallback. */
typedef struct P4GameItemsUi P4GameItemsUi;
/* *out must be NULL. CPU-sealed items suffice; no guest binding is consulted.
 * Empty models succeed without a product/allocation. */
int p4_game_tables_items_ui_prepare(const P4GameItems *items, const P4Resources *resources, P4GameItemsUi **out);
void p4_game_tables_items_ui_destroy(P4GameItemsUi *ui);
const P4GameItems *p4_game_tables_items_ui_model(const P4GameItemsUi *ui);
uint32_t p4_game_tables_items_ui_item(const P4GameItemsUi *ui, uint32_t id);
uint32_t p4_game_tables_items_ui_category(const P4GameItemsUi *ui, uint32_t id);
/* Runtime preparation only; canonical item commit publishes it with the exact
 * model. The core detaches native users before destroying this owned product. */
int p4_game_tables_items_ui_stage(P4GameItemsUi *ui);
/* Reports only whether this exact product is borrowed by the native item
 * owner; it is NOT proof that tasks, queues, graphics or the image quiesced. */
int p4_game_tables_items_ui_detached(const P4GameItemsUi *ui);
/* Call only after task, queue and graphics quiescence and ending the native
 * item session. Refuses to release pins while the exact UI product is borrowed. */
int p4_game_tables_items_ui_boot_session_end(void);
#endif
