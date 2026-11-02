/* Expanded item metadata/state. Retail saves and arrays retain their PS2 layout. */
#ifndef P4PC_GAME_ITEMS_H
#define P4PC_GAME_ITEMS_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_tables.h"
#include "p4pc/game_text.h"
#include "p4pc/resource_ref.h"
#include "p4pc/game_items_native.h"


typedef struct P4ItemRow {
    uint32_t kind, flags;
    uint16_t primary_value, secondary_value;
    uint8_t opaque_0c[2], property, opaque_0f;
    uint16_t middle_band_value, last_band_value;
    int16_t stat_bonus[5]; /* strength, endurance, magic, agility, luck */
    uint8_t opaque_1e[6];
    uint32_t granted_skill;
    uint16_t value, accessory_percent, element, opaque_2e;
    uint32_t category_mask, price;
    int8_t available_month, available_day;
    uint8_t opaque_3a[2];
    uint32_t first_moon_gate, second_moon_gate;
} P4ItemRow;
_Static_assert(sizeof(P4ItemRow) == P4_ITEM_ROW_SIZE, "retail item stride");
_Static_assert(offsetof(P4ItemRow, stat_bonus) == 0x14, "retail item signed stat bonuses");
_Static_assert(offsetof(P4ItemRow, granted_skill) == 0x24, "retail item granted skill");
_Static_assert(offsetof(P4ItemRow, element) == 0x2c, "retail item element");
_Static_assert(offsetof(P4ItemRow, category_mask) == 0x30, "retail item category mask");

typedef struct P4GameItemDesc {
    uint32_t id, revision;
    const char *key;
    P4ResourceRef icon;
    P4ItemRow metadata;
} P4GameItemDesc;
typedef struct P4GameItemRequirement { uint32_t item_id, quantity; } P4GameItemRequirement;
#define P4_ITEM_MODEL_NONE UINT32_MAX
/* CPU-sealed new-row definitions contain no guest stream or process token. */
typedef struct P4GameItemView {
    uint32_t id, revision;
    uint32_t requirements_mask;
    P4GameItemRequirement requirements[2];
    uint32_t models_registered, right_model_id, left_model_id;
    char owner[64], key[P4_TABLE_KEY_SIZE];
    P4ResourceRef icon;
    P4ItemRow metadata;
} P4GameItemView;
/* Stable declaration identity, not a VM reference or a readiness flag.
 * All-zero means an explicitly sourced compiled-native category action.
 * Otherwise key is owner:export-key, resolved by the real script-action owner. */
typedef struct P4GameItemActionRef {
    char owner[64], key[P4_TABLE_KEY_SIZE];
    uint32_t revision;
} P4GameItemActionRef;
typedef struct P4GameItemCategoryDesc {
    uint32_t id, revision, filter_a, filter_b;
    uint32_t stock_policy;
    const char *key;
    P4ResourceRef icon;
    P4GameItemActionRef action;
} P4GameItemCategoryDesc;
typedef struct P4GameItemCategoryView {
    uint32_t id, revision, filter_a, filter_b;
    uint32_t stock_policy;
    char owner[64], key[P4_TABLE_KEY_SIZE];
    P4ResourceRef icon;
    P4GameItemActionRef action;
} P4GameItemCategoryView;
typedef struct P4GameItemDefinition {
    uint32_t id, revision;
    const char *owner; /* NULL for the exact supplied retail prefix. */
    const P4ItemRow *metadata;
} P4GameItemDefinition;
typedef struct P4GameItemState {
    uint32_t id;
    uint8_t quantity, availability;
} P4GameItemState;
typedef struct P4GameItems P4GameItems;
/* C-only source preparation, after a category declaration and before its CPU
 * seal. Install a real C action/owned declared slot with action_register; a
 * returned-ready flag is insufficient. Copy the borrowed reference and retain
 * no movable preseal row. No VM execution or late table-ID mutation. */
typedef int (*P4GameItemActionPrepareFn)(P4GameItems *exact_items,
    const char *category_owner, uint32_t category, const P4GameItemActionRef *reference,
    void *context);
typedef enum P4GameItemUseTable { P4_ITEM_USE_ROW = 0, P4_ITEM_USE_CATEGORY = 1 } P4GameItemUseTable;
typedef int (*P4GameItemUseFn)(const P4GameItems *exact_view, uint32_t table, uint32_t id, void *owning_source_context);
/* Immutable preseal runtime transport only. The root owns actual ITEM span/
 * selected origin proof; this declaration is not source/native readiness. */
int p4_game_tables_items_bind_runtime_use(P4GameItems *items, P4GameItemUseFn use, void *owning_source_context);
int p4_game_tables_items_validate_runtime_sources(const P4GameItems *items);
int p4_game_tables_items_runtime_use(const P4GameItems *items, uint32_t id);
int p4_game_tables_items_category_runtime_use(const P4GameItems *items, uint32_t category);
P4GameItems *p4_game_tables_items_create(const P4GameTableAllocator *allocator);
void p4_game_tables_items_destroy(P4GameItems *items);
/* Copies descriptor, keys and metadata. Explicit IDs must exceed the retail
 * domain and fit the signed native list/equipment consumers. No implicit IDs. */
int p4_game_tables_items_register(P4GameItems *items, const char *owner, const P4GameItemDesc *row);
/* Optional full-width dependencies for gates0/1 on an owned new row. Quantity
 * remains0..255; legacy metadata bits are never repacked. The row's owner/key
 * and revision own these fields, exposed by the frozen view's mask/array. */
int p4_game_tables_items_requirement_register(P4GameItems *items, const char *owner,
    uint32_t id, uint32_t gate, uint32_t required_id, uint32_t quantity);
/* Explicit existing native type7 keys, not item IDs. Right0..255; the unit2
 * left namespace is1000..1255. No authored-model loader is implied.
 * Weapons require right; category_mask bit(unit+4) defines compatibility.
 * Unit2 requires left; unit3 requires right0x70..0x95 and both native seasons. */
int p4_game_tables_items_models_register(P4GameItems *items, const char *owner,
    uint32_t id, uint32_t right_model_id, uint32_t left_model_id);
/* New categories are explicit, never inferred from an item ID's high byte.
 * Skill cards use 6; the native 32-bit category/filter domain ends at 31. */
int p4_game_tables_items_category_register(P4GameItems *items, const char *owner,
    const P4GameItemCategoryDesc *category);
/* Handler owns custom-category semantics and consumption. HANDLED permits a
 * native list rebuild; DEFERRED leaves it unchanged for the real UI owner.
 * No guest work pointer is exposed, and native consumable logic never follows.
 * The core must keep user/fn alive through snapshot/task quiescence. */
int p4_game_tables_items_action_register(P4GameItems *items, const char *owner,
    uint32_t category, const P4GameItemActionRef *reference, P4GameItemActionFn action, void *user);
/* Owning preseal traversal for script-backed declarations. Immutable at/find
 * getters intentionally remain unavailable until semantic seal. Missing
 * preparer refuses script refs; compiled-native categories are left alone.
 * Callback may bind actions only, never add/change IDs or descriptors. */
int p4_game_tables_items_prepare_actions(P4GameItems *items, P4GameItemActionPrepareFn prepare, void *context);
/* CPU-only semantic validation/seal. Exact ITEMTBL bytes supply the retail
 * prefix; the source owner retains/authenticates them and both text snapshots.
 * Text must be semantically sealed RAW names/MESSAGE descriptions. No guest,
 * resource or VM work occurs here; semantic revision is not source authority
 * or native readiness. Empty staging needs no prefix/text and allocates none.
 * A repeated identical seal revision succeeds; a different revision conflicts. */
int p4_game_tables_items_seal(P4GameItems *items, const uint8_t *retail_file, size_t size,
    const P4GameText *names, const P4GameText *descriptions, uint32_t revision);
uint32_t p4_game_tables_items_semantic_revision(const P4GameItems *items);
int p4_game_tables_items_definition_find(const P4GameItems *items, uint32_t id,
    P4GameItemDefinition *out);
/* Pure pre-raw native prerequisite, never an allocation or origin attestation.
 * CPU-only NULL-allocator definitions remain valid; added rows cannot enter
 * native staging without a real allocator. Zero-added prefix views need none. */
int p4_game_tables_items_native_admit(const P4GameItems *items);
/* Late native allocation/copy and private bindings only, after the actual text
 * materializers and guest heap. No semantic reparse/sort/declaration changes.
 * Failed allocation retains sealed definitions with native revision0. */
int p4_game_tables_items_materialize(P4GameItems *items);
/* CPU preparation records authored rows/categories OR a genuine retained
 * original-prefix proof view before raw copy exposure (not an invented empty).
 * The canonical source preparer authenticates actual loaded bytes/provenance
 * before the retail copy; any exposed-source failure uses the core's real
 * controlled failure frontend, never a guessed owner or deferred empty table. */
int p4_game_tables_items_source_stage(const P4GameItems *items);
/* Early Tables100 stages the exact CPU-sealed model and registers one canonical
 * native item participant, without allocating guest rows or publishing a base.
 * UI300 stages its separate exact product. Late canonical prepare verifies
 * actual original copies/models and materializes privately after real text/
 * heap readiness; only the shared finalizer may publish the prepared result. */
int p4_game_tables_items_boot_stage(P4GameItems *items);
/* EARLY candidate rollback only. Refuses actual raw load, active/materialized
 * state or another exact view; unregisters only the owned item participant.
 * No guest access, image_live inference, sibling/global service clearing. */
int p4_game_tables_items_boot_unstage(P4GameItems *items);
P4GameItems *p4_game_tables_items_active(void);
/* Core quiesces associated tasks/queued work and supplies actual image lifetime.
 * While image_live, restore only a GP slot still referencing our owned arena.
 * Clear all native/UI borrows before destroying products and guest allocation.
 * Does not clear shared source/completion services owned by the aggregate. */
int p4_game_tables_items_session_end(int image_live);
uint32_t p4_game_tables_items_revision(const P4GameItems *items);
size_t p4_game_tables_items_count(const P4GameItems *items);
const P4GameItemView *p4_game_tables_items_at(const P4GameItems *items, size_t index);
const P4GameItemView *p4_game_tables_items_find(const P4GameItems *items, uint32_t id);
size_t p4_game_tables_items_category_count(const P4GameItems *items);
const P4GameItemCategoryView *p4_game_tables_items_category_at(const P4GameItems *items, size_t index);
const P4GameItemCategoryView *p4_game_tables_items_category_find(const P4GameItems *items, uint32_t id);
const P4ItemRow *p4_game_tables_items_metadata(const P4GameItems *items, uint32_t id);
/* Dense guest index limit differs from registered-row count when IDs have gaps. */
uint32_t p4_game_tables_items_limit(const P4GameItems *items);
uint32_t p4_game_tables_items_base(const P4GameItems *items);
/* Mutable expanded state only; retail inventory/availability are never reached.
 * Sidecar code serializes these fields, not a struct blob or transient address. */
int p4_game_tables_items_state_get(const P4GameItems *items, uint32_t id, P4GameItemState *state);
int p4_game_tables_items_state_set(P4GameItems *items, uint32_t id, uint32_t quantity, uint32_t availability);
/* Expanded equipment references are side state, never new IDs in the retail
 * three-halfword equipment fields. Unit IDs are 1..10; slots are 0..2.
 * New references must match metadata kind/slot and the unit compatibility bit.
 * get returns NOT_FOUND when the original retail field is authoritative.
 * clear removes the overlay; it does not choose/substitute a retail item. */
int p4_game_tables_items_equipment_get(const P4GameItems *items, uint32_t unit, uint32_t slot, uint32_t *id);
int p4_game_tables_items_equipment_set(P4GameItems *items, uint32_t unit, uint32_t slot, uint32_t id);
int p4_game_tables_items_equipment_clear(P4GameItems *items, uint32_t unit, uint32_t slot);
void p4_game_tables_items_state_clear(P4GameItems *items);
#endif
