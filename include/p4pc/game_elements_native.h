#ifndef P4PC_GAME_ELEMENTS_NATIVE_H
#define P4PC_GAME_ELEMENTS_NATIVE_H
#include <stdint.h>
/* Scalar-only native game-unit boundary: no host registry, resource-owner or
 * allocator structures enter the translated units' foreign-field census. */
typedef enum P4AffinityKind { P4_ENTITY_PERSONA = 0, P4_ENTITY_ENEMY = 1 } P4AffinityKind;
#define P4_ELEMENT_RETAIL_COUNT 19u
/* Enemy rows datUnitInit 0x00231AF0 accepts (id < 0x150); it starts id2 = id. */
#define P4_ENEMY_RETAIL_COUNT 336u
/* Persona records datPersonaInitAllRecords 0x0010FFA0 creates: IDs 1..255. */
#define P4_PERSONA_RETAIL_COUNT 256u
#define P4_CHANNEL_DAMAGE 0u
#define P4_CHANNEL_INSTANT_KO 1u
#define P4_CHANNEL_STATUS 2u
#define P4_ELEMENT_PHYSICAL 1u
#define P4_ELEMENT_OFFENSIVE 2u
#define P4_ELEMENT_AFFINITY 4u
#define P4_ELEMENT_VISIBLE 8u
/* A resolved action's delivery (hardmode-design 8.1) rides in its element's
 * flags, so the 44-byte frame and the AI candidate guest layouts stay as they
 * are. Neither bit set: the delivery follows the element (P4_ELEMENT_PHYSICAL).
 * An authored <skill delivery> sets one; registry element flags never do. */
#define P4_ELEMENT_DELIVERY_PHYSICAL 0x100u
#define P4_ELEMENT_DELIVERY_MAGIC 0x200u
#define P4_DELIVERY_DEFAULT 0u
#define P4_DELIVERY_PHYSICAL 1u
#define P4_DELIVERY_MAGIC 2u
#define P4_AFF_NULL 0x01000000u
#define P4_AFF_REPEL 0x02000000u
#define P4_AFF_DRAIN 0x04000000u
#define P4_AFF_WEAK 0x08000000u
#define P4_AFF_RESIST 0x10000000u
#define P4_AFF_FLAGS 0x1f000000u
/* Effect-component roles are not channel IDs or native HP/SP amount channels.
 * A mixed skill resolves each actual payload independently. */
#define P4_ACTION_COMPONENT_DAMAGE 0u
#define P4_ACTION_COMPONENT_INSTANT_KO 1u
#define P4_ACTION_COMPONENT_STATUS 2u
/* valid is independent of all ID bits. UINT32_MAX is a legal semantic leaf,
 * never the retail signed -1 sentinel. Keep this value local to its owning
 * query/action; no process-global last-element state exists. */
typedef struct P4GameActionElement {
    uint32_t valid, element_id, channel_id, flags, family, component;
} P4GameActionElement;
/* One caller-owned action/component frame. Source identity, metadata ownership
 * and actual native options survive reflection/multi-target loops without a
 * process-global last action. An owned binding is distinct from classification. */
typedef struct P4GameActionFrame {
    P4GameActionElement element;
    uint32_t skill_id, source_kind, source_id, native_options, owned_binding;
} P4GameActionFrame;
typedef struct P4GameUnitIdentity {
    uint32_t entity_kind, entity_id, native_profile;
} P4GameUnitIdentity;
/* Success supplies an exact alias and actual loaded enemy metadata flags.
 * Missing aliases or an unsupported metadata source return an error rather
 * than a guessed profile, truncated ID or zero-flags reveal-policy bypass. */
typedef struct P4GameEnemyContext {
    uint32_t enemy_id, native_profile, profile_id, knowledge_id, metadata_flags;
} P4GameEnemyContext;
/* A prediction query is explicit and target-local. An unknown-affinity AI
 * policy may supply native neutral100 without reading the hidden real row. */
#define P4_AFFINITY_QUERY_ACTUAL 0u
#define P4_AFFINITY_QUERY_PROVIDED 1u
typedef struct P4GameAffinityQuery {
    uint32_t mode, value;
} P4GameAffinityQuery;
static inline int p4_game_action_element_is_physical(const P4GameActionElement *element)
{
    return element->valid && (element->flags & P4_ELEMENT_PHYSICAL) != 0;
}
/* Physical delivery: the crit roll, the counter roll and the physical-only
 * damage term. Affinity, shields, Boost/Amp and evasion passives keep asking
 * the element. Reflected and countered hits carry their frame, so they keep
 * the original delivery. */
static inline int p4_game_action_frame_physical(const P4GameActionFrame *frame)
{
    const uint32_t flags = frame->element.flags;
    if (flags & P4_ELEMENT_DELIVERY_PHYSICAL) return frame->element.valid;
    if (flags & P4_ELEMENT_DELIVERY_MAGIC) return 0;
    return p4_game_action_element_is_physical(&frame->element);
}
static inline int p4_game_action_element_is_offensive(const P4GameActionElement *element)
{
    return element->valid && (element->flags & P4_ELEMENT_OFFENSIVE) != 0;
}
static inline int p4_game_action_element_has_affinity(const P4GameActionElement *element)
{
    return element->valid && (element->flags & P4_ELEMENT_AFFINITY) != 0;
}
/* The translated datCalc unit owns this one canonical native precedence body.
 * unit_pointer is its live guest-backed native unit view. effect_selector is
 * an independently qualified legacy coverage selector, NEVER a narrowed leaf.
 * Callers resolve the base first; no row lookup or side-table fallback occurs. */
uint32_t p4_game_elements_native_affinity_apply(void *unit_pointer,
    int32_t effect_selector, uint32_t base, uint32_t additional_passives);
/* These adapters use only the fully committed native snapshot. enabled means
 * the installed routing cohort is active, not that every arbitrary ID/pair has
 * valid authored data. Missing actual source/profile/pair data refuses safely. */
int p4_game_elements_native_enabled(void);
/* Pure finite-candidate preview using the exact committed Tables/Skills/Items
 * tuple and actual expanded equipment side state. No source use, affinity
 * query, RNG or policy replay. Returns status and leaves output unchanged on
 * failure; only the actual later semantic consumption uses native frames. */
int p4_game_elements_native_preview_primary_frame(void *unit, uint32_t skill_id,
    uint32_t native_options, P4GameActionFrame *frame);
void p4_game_elements_native_primary_frame(void *unit, uint32_t skill_id,
    uint32_t native_options, P4GameActionFrame *frame);
/* Unlike ordinary actions, applicability has a retail actorless skill fallback. */
void p4_game_elements_native_applicability_frame(void *unit, uint32_t skill_id, P4GameActionFrame *frame);
void p4_game_elements_native_action_frame(void *unit, uint32_t skill_id,
    uint32_t component, uint32_t status_mask, uint32_t native_options, P4GameActionFrame *frame);
/* Presentation routes (001A7720 formation/cue/motion, 00199D00 opening motion):
 * the frame's retail leaf, else its declared retail family, else -1 so the
 * caller's default branch runs. Call only when native routing is enabled. */
int32_t p4_game_elements_native_presentation_leaf(void *unit, uint32_t skill_id);
/* 00201B00 hit-display category: the retail 0023D6E0 bit for a retail leaf;
 * a new leaf yields only its ailment-class bit (0x200) or 0. */
uint32_t p4_game_elements_native_hit_class_bits(void *unit, uint32_t skill_id);
/* 001F14F0 eligible enemy-knowledge gate (defender is the target DatUnit given
 * to the row's affinity resolvers). Observes the primary frame and, when this
 * row resolved the uncured KO payload (0x80000), the INSTANT_KO frame; the
 * knowledge owner alone writes the retail bitmap. Refuses on any failure. */
void p4_game_elements_native_learn(void *defender, void *attacker, uint32_t skill_id, uint32_t ko_status);
/* The original amount equation with the caller's owning full frame and
 * optional target-affinity policy; no global last-action state. */
int32_t p4_game_elements_native_amount(uint32_t skill_id, void *source, void *target,
    uint32_t hit_count, uint32_t outcome, uint32_t hit_kind, int32_t options, uint32_t channel,
    const P4GameActionFrame *frame, const P4GameAffinityQuery *query);
_Noreturn void p4_game_elements_native_calculation_refuse(int32_t result);
/* Actual metadata consumption without constructing an unused element frame.
 * Calculation-only terms call this at their real skill-row read, never preview. */
void p4_game_elements_native_skill_use(uint32_t skill_id);
uint32_t p4_game_elements_native_affinity(void *unit, const P4GameActionFrame *frame);
uint32_t p4_game_elements_native_affinity_leaf(void *unit,
    uint32_t leaf, uint32_t channel, uint32_t component);
_Noreturn void p4_game_elements_native_mask_refuse(uint32_t leaf);
int p4_game_elements_unit_identity(void *unit, P4GameUnitIdentity *identity);
int p4_game_elements_native_enemy_context(void *unit, P4GameEnemyContext *context);
uint32_t p4_game_elements_native_element_affinity(void *unit, const P4GameActionFrame *frame);
uint32_t p4_game_elements_native_affinity_query(void *unit,
    const P4GameActionFrame *frame, const P4GameAffinityQuery *query);
int p4_game_elements_native_weapon_element(uint32_t item_id, uint32_t component,
    P4GameActionElement *element);
/* Preparation-safe query over the actual loaded status bank. No active
 * element snapshot or fake DatUnit is needed. Unsupported/missing metadata
 * returns an error without changing output; current retail bound is336. */
int p4_game_elements_enemy_metadata_flags(uint32_t enemy_id, uint32_t *flags);
/* Pure profile queries, not actions or physical-family predicates. Exact side
 * rows win; unchanged natural retail 0..18 pairs
 * (p4_game_elements_profile_natural_pair) alone may use the ORIGINAL native
 * route when absent. Keep the actual caller's base0023A6B0 versus
 * wrapper00242800 role; new/custom pairs never enter retail arrays. */
int p4_game_elements_native_profile_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *native_value); /* Wrapper. */
int p4_game_elements_native_profile_base_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *native_value);
#endif
