/* Native table metadata. No retail RAM/save layout is enlarged by this API. */
#ifndef P4PC_GAME_TABLES_H
#define P4PC_GAME_TABLES_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/table_result.h"
#include "p4pc/game_elements_native.h"
#include "p4pc/resource_ref.h"
/* Native allocator only, never script pointers. Successful allocations return
 * a writable host mapping and its guest address for the same size bytes until
 * release. The core owns mapping validation/admission. Empty snapshots request
 * no guest allocation. */
typedef struct P4GameTableAllocator {
    void *user;
    void *(*allocate)(void *user, uint32_t size, uint32_t *guest);
    void (*release)(void *user, uint32_t guest, void *host, uint32_t size);
} P4GameTableAllocator;
/* Actual native source label/borrowed managed bytes; source_key is unchanged
 * (item: "init/itemtbl.bin"), never a guessed catalog path. No gameplay Lua or
 * yielding. The core validates winning bytes/claims and freezes already-staged
 * owned bindings before publication. Same callback/user is idempotent; conflicts
 * are refused. Missing preparation is PREPARE_REQUIRED, never a successful no-op.
 * Clear only for fully quiesced RW/image/session teardown, not GL loss. */
typedef int (*P4GameTablePrepareFn)(void *user, const char *table_key,
    const char *source_key, uint32_t original_rows, const uint8_t *retail_bytes, size_t size);
int p4_game_tables_prepare_register(P4GameTablePrepareFn prepare, void *user);
/* Exact owner-scoped pre-exposure rollback; never removes another callback. */
int p4_game_tables_prepare_unregister(P4GameTablePrepareFn prepare, void *user);
int p4_game_tables_prepare(const char *table_key, const char *source_key,
    uint32_t original_rows, const uint8_t *retail_bytes, size_t size);
int p4_game_tables_prepare_clear(void);
/* Shared actual0010d480 completion order: item, skill, persona, enemy, table-source,
 * element, element-ai, element-knowledge, element-ui. Expanded native activation
 * requires actual source/AI-carrier/knowledge/UI participants, not claim bits.
 * Register only expanding participants. Prepare validates/freezes without
 * publishing RAM/count/active state; commit only publishes prevalidated state
 * and cannot fail/yield. All prepares succeed before any commit.
 * Each real reload revalidates/rebinds; registration locks at first completion.
 * Unpublished allocations remain caller-owned on failure; no fake rollback.
 * Core clears only after associated tasks/queues are quiesced, never GL loss. */
typedef int (*P4GameTableCompletePrepareFn)(void *user);
typedef void (*P4GameTableCompleteCommitFn)(void *user);
int p4_game_tables_battle_complete_register(const char *table_key,
    P4GameTableCompletePrepareFn prepare, P4GameTableCompleteCommitFn commit, void *user);
/* Owner-scoped EARLY rollback only, before first completion locks the graph.
 * Exact callback/context must match. Removes only this participant and its own
 * requirements, never sibling slots/edges or an inferred native image lifetime.
 * Missing slot is idempotent; callers first prove their owned raw hooks unused. */
int p4_game_tables_battle_complete_unregister(const char *table_key,
    P4GameTableCompletePrepareFn prepare, P4GameTableCompleteCommitFn commit, void *user);
/* Presence prerequisite for an already registered owner; the required family
 * may register later. It does not change prepare order or publish capability.
 * Missing prerequisites refuse before ANY prepare/commit; self-dependency is
 * invalid. As with registration, requirements lock at first completion. */
int p4_game_tables_battle_complete_require(const char *owner_key, const char *required_key);
int p4_game_tables_battle_complete_clear(void);

#define P4_TABLE_KEY_SIZE 96
#define P4_TABLE_NAME_SIZE 96

typedef enum P4ChannelKind { P4_CHANNEL_PERCENT = 0, P4_CHANNEL_KO = 1 } P4ChannelKind;
typedef enum P4KoAffinity { P4_KO_NORMAL = 0, P4_KO_VULNERABLE = 1, P4_KO_RESIST = 2, P4_KO_NULL = 3 } P4KoAffinity;

/* All descriptor strings are copied. Stable keys are owner:local-name.
 * id is explicit, so a mod/core can restore the exact namespace assignment.
 * Semantic element/family/channel IDs and affinity references use the full u32
 * domain, including UINT32_MAX. Packed retail fields and affinity value/scalar
 * encodings retain their separate limits; registration is not a native-route
 * capability or permission to narrow an ID into such a field.
 * Icons are stable owner/key/revision references, never process tokens. Real
 * resource preparation resolves them; UI products own validated token pins. */
typedef struct P4ElementDesc {
    uint32_t id, flags, family, order, revision;
    char key[P4_TABLE_KEY_SIZE];
    char display_name[P4_TABLE_NAME_SIZE], short_name[P4_TABLE_NAME_SIZE];
    P4ResourceRef icon;
} P4ElementDesc;
typedef struct P4AffinityChannelDesc {
    uint32_t id, kind, revision;
    char key[P4_TABLE_KEY_SIZE];
    char display_name[P4_TABLE_NAME_SIZE];
} P4AffinityChannelDesc;

/* Fixed-width side rows. Entity identity and active profile are independent:
 * enemy id2 aliases never replace enemy identity. value is decoded native
 * flags|percentage for percent channels, P4KoAffinity for KO channels, whose
 * independent native susceptibility scalar is in scalar (0..65535).
 * No implicit profile/row fallback: callers declare every desired projection. */
typedef struct P4AffinityRow {
    uint32_t entity_kind, entity_id, profile_id, element_id, channel_id;
    uint32_t value, scalar;
} P4AffinityRow;

/* Source identities are table-domain keys, never package ordinals or guest
 * addresses. Component identity is part of the binding key, not element alone.
 * A player weapon binding is by equipped item ID, not character.
 * Stable keys/schema/revisions allow the core to persist ownership without
 * serializing pointers or registration order. Registration is not admission. */
typedef enum P4ActionElementSource {
    P4_ACTION_ELEMENT_SKILL = 0, P4_ACTION_ELEMENT_WEAPON = 1,
    P4_ACTION_ELEMENT_ENEMY_ATTACK = 2
} P4ActionElementSource;
typedef struct P4ActionElementBinding {
    uint32_t source_kind, source_id, component, element_id, channel_id, schema, revision;
    char key[P4_TABLE_KEY_SIZE];
} P4ActionElementBinding;
/* Native profile is an explicit physical selector: an enemy's current id2
 * affinity-row selector, or zero for its current persona. It is not enemy ID
 * or semantic profile ID. Distinct enemies sharing id2 keep distinct aliases.
 * Knowledge identity is explicit and independent of affinity-row identity. */
typedef struct P4AffinityProfileAlias {
    uint32_t entity_kind, entity_id, native_profile, profile_id, knowledge_id;
    uint32_t schema, revision;
    char key[P4_TABLE_KEY_SIZE];
} P4AffinityProfileAlias;
/* Additional native affinity effects, not a second resolver or a replacement
 * hook. One owner claims a source's added coverage set. Rows merge into the
 * canonical native passive precedence; base affinity and existing effect
 * coverage remain separate. A KO pair cannot reflect or drain. */
typedef enum P4AffinityEffectSource {
    P4_AFFINITY_EFFECT_SKILL = 0, P4_AFFINITY_EFFECT_EQUIPPED_ITEM = 1
} P4AffinityEffectSource;
typedef struct P4AffinityEffectBinding {
    uint32_t source_kind, source_id, element_id, channel_id, flags, schema, revision;
    char key[P4_TABLE_KEY_SIZE];
} P4AffinityEffectBinding;

typedef struct P4GameTables P4GameTables;
/* Actual runtime consumption transport, declared before seal. Root binds this
 * exact view to its captured immutable ledger/context; the declaration itself
 * is NOT origin/native capability. No pure lookup/seal/serialization invokes
 * it. Domain selectors are P4TableSourceDomain in game_table_sources.h.
 * runtime_use_row accepts only the exact issued row pointer in this frozen
 * view, constructs its canonical ids without a second lookup, and authorizes
 * BEFORE its semantic fields are used. Explicit unowned builtin element/
 * channel descriptors need no authored origin; absent/foreign rows never do. */
typedef int (*P4GameTableRegistryUseFn)(const P4GameTables *exact_view,
    uint32_t domain, const uint32_t ids[5], void *owning_source_context);
int p4_game_tables_bind_runtime_use(P4GameTables *tables,
    P4GameTableRegistryUseFn use, void *owning_source_context);
int p4_game_tables_validate_runtime_sources(const P4GameTables *tables);
int p4_game_tables_runtime_use_row(const P4GameTables *tables, uint32_t domain, const void *issued_row);
/* A staging registry starts with all retail leaves and three standard channels.
 * Creating it has no effect on the game. No allocation occurs during lookups. */
P4GameTables *p4_game_tables_create(void);
void p4_game_tables_destroy(P4GameTables *tables);
int p4_game_tables_element(P4GameTables *tables, const char *owner, const P4ElementDesc *desc);
int p4_game_tables_channel(P4GameTables *tables, const char *owner, const P4AffinityChannelDesc *desc);
int p4_game_tables_affinity(P4GameTables *tables, const char *owner, const P4AffinityRow *row);
/* A side row whose value is the entity's own retail affinity entry, read from
 * the user's selected UNIT banks at seal: row retail_row and packed column
 * retail_element (0..15) of the 32-byte enemy (UNIT1) or persona (UNIT2) bank
 * of the row's entity kind. Keys, conflicts and ownership are those of an
 * authored row; value/scalar are ignored. A percentage channel stores the exact
 * p4_affinity_decode word with its low16 as scalar; a KO channel stores the
 * p4_affinity_project_ko projection. Until resolved the row is pending and
 * freeze refuses NOT_READY. Mods ship no retail value this way. */
int p4_game_tables_affinity_retail(P4GameTables *tables, const char *owner, const P4AffinityRow *row,
    uint32_t retail_row, uint32_t retail_element);
size_t p4_game_tables_affinity_retail_pending(const P4GameTables *tables);
/* Resolve every pending retail row from the actual sealed bank bytes. A row or
 * column outside the given bank refuses NOT_FOUND and resolves nothing. */
int p4_game_tables_affinity_resolve_retail(P4GameTables *tables,
    const uint8_t *enemy_bank, size_t enemy_size, const uint8_t *persona_bank, size_t persona_size);
int p4_game_tables_action_element(P4GameTables *tables, const char *owner, const P4ActionElementBinding *binding);
int p4_game_tables_affinity_alias(P4GameTables *tables, const char *owner, const P4AffinityProfileAlias *alias);
int p4_game_tables_affinity_effect(P4GameTables *tables, const char *owner, const P4AffinityEffectBinding *effect);
/* Resolve one explicitly selected source domain against a frozen registry.
 * declared is the actual metadata result, including an explicit validity tag,
 * not an invented fallback. An owned binding takes precedence. Classification
 * comes from that same registry snapshot; no ID bit is a status/category mask.
 * This is a data query, not a native action-capability or publication proof. */
int p4_game_tables_resolve_action_element(const P4GameTables *tables,
    uint32_t source_kind, uint32_t source_id, uint32_t component, const P4GameActionElement *declared,
    P4GameActionElement *resolved);
/* Classify an explicit valid/invalid declaration against the same frozen
 * descriptors without a second source-binding lookup. Output is atomic. */
int p4_game_tables_classify_action_element(const P4GameTables *tables,
    const P4GameActionElement *declared, P4GameActionElement *resolved);
/* Freeze validates all references and sorts once; failure publishes nothing.
 * The caller owns atomic safe-boundary publication/lifetime of this immutable
 * snapshot. Never destroy a snapshot while a native consumer holds it. */
int p4_game_tables_freeze(P4GameTables *tables, uint32_t revision);
uint32_t p4_game_tables_revision(const P4GameTables *tables);
/* Pure semantic census, usable before commit/freeze. Includes builtin-only
 * descriptor claims as well as added leaves/channels/side/action/alias/effects.
 * NULL or untouched retail is0; this is a request, never runtime capability. */
int p4_game_elements_requested(const P4GameTables *tables);
const P4ElementDesc *p4_game_tables_find_element(const P4GameTables *tables, uint32_t id);
const P4AffinityChannelDesc *p4_game_tables_find_channel(const P4GameTables *tables, uint32_t id);
const P4ElementDesc *p4_game_tables_element_key(const P4GameTables *tables, const char *key);
const P4AffinityChannelDesc *p4_game_tables_channel_key(const P4GameTables *tables, const char *key);
size_t p4_game_tables_element_count(const P4GameTables *tables);
/* Frozen descriptors in ascending (order,id), including hidden retail leaves. */
const P4ElementDesc *p4_game_tables_element_at(const P4GameTables *tables, size_t index);
const P4AffinityRow *p4_game_tables_find_affinity(const P4GameTables *tables,
    uint32_t kind, uint32_t entity, uint32_t profile, uint32_t element, uint32_t channel);
/* Canonical frozen enumeration for field-wise serialization; never persist a
 * native struct blob. Owner strings share snapshot lifetime; NULL is retail
 * or an invalid id/index. New rows expose the mod that claimed them. */
size_t p4_game_tables_channel_count(const P4GameTables *tables);
size_t p4_game_tables_affinity_count(const P4GameTables *tables);
const P4AffinityChannelDesc *p4_game_tables_channel_at(const P4GameTables *tables, size_t index);
const P4AffinityRow *p4_game_tables_affinity_at(const P4GameTables *tables, size_t index);
const char *p4_game_tables_element_owner(const P4GameTables *tables, uint32_t id);
const char *p4_game_tables_channel_owner(const P4GameTables *tables, uint32_t id);
const char *p4_game_tables_affinity_owner(const P4GameTables *tables, size_t index);
const P4ActionElementBinding *p4_game_tables_find_action_element(const P4GameTables *tables,
    uint32_t source_kind, uint32_t source_id, uint32_t component);
const P4AffinityProfileAlias *p4_game_tables_find_affinity_alias(const P4GameTables *tables,
    uint32_t entity_kind, uint32_t entity_id, uint32_t native_profile);
size_t p4_game_tables_action_element_count(const P4GameTables *tables);
size_t p4_game_tables_affinity_alias_count(const P4GameTables *tables);
const P4ActionElementBinding *p4_game_tables_action_element_at(const P4GameTables *tables, size_t index);
const P4AffinityProfileAlias *p4_game_tables_affinity_alias_at(const P4GameTables *tables, size_t index);
const char *p4_game_tables_action_element_owner(const P4GameTables *tables, size_t index);
const char *p4_game_tables_affinity_alias_owner(const P4GameTables *tables, size_t index);
const P4AffinityEffectBinding *p4_game_tables_find_affinity_effect(const P4GameTables *tables,
    uint32_t source_kind, uint32_t source_id, uint32_t element_id, uint32_t channel_id);
size_t p4_game_tables_affinity_effect_count(const P4GameTables *tables);
const P4AffinityEffectBinding *p4_game_tables_affinity_effect_at(const P4GameTables *tables, size_t index);
const char *p4_game_tables_affinity_effect_owner(const P4GameTables *tables, size_t index);
/* Compiled pair index: an absent pair skips native skill/equipment work. */
int p4_game_tables_has_affinity_effect_pair(const P4GameTables *tables, uint32_t element_id, uint32_t channel_id);
const char *p4_game_tables_error(int result);
/* Exact retail packed-u16 decode, including unflagged zero. */
uint32_t p4_affinity_decode(uint16_t packed);
/* Semantic conversion only; caller supplies entity/profile/pair for the row. */
void p4_affinity_project_ko(uint32_t decoded, uint32_t *kind, uint32_t *scalar);
#endif
