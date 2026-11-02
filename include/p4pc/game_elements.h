#ifndef P4PC_GAME_ELEMENTS_H
#define P4PC_GAME_ELEMENTS_H
#include "p4pc/game_tables.h"
/* Low-level queries over one owned frozen snapshot, NOT activation/capability
 * publication. Missing aliases/profiles/pairs are errors, never legacy-array
 * fallback. Returned rows share snapshot lifetime; failure changes no output.
 * element must be an explicit resolved value from that same snapshot. */
int p4_game_elements_affinity_base(const P4GameTables *tables,
    uint32_t entity_kind, uint32_t entity_id, uint32_t native_profile,
    const P4GameActionElement *element, const P4AffinityRow **base);
/* unit is the actual live guest-backed native DatUnit view. Its current enemy
 * ID/id2 or selected persona supplies identity/profile. New leaves never enter
 * a packed retail row. The same native guard/passive/break/wall body applies
 * only AFTER base injection; custom effects are native skill/item lookups. */
int p4_game_elements_affinity_effective(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *native_value);
/* Actual native/UI semantic consumption of the exact published tuple. Same
 * equation as above, but selected authored alias/base/channel/effect records
 * use Root's real captured source transport BEFORE semantic fields. Never
 * call from metadata planning, binding or finite candidate preview. */
int p4_game_elements_runtime_affinity_effective(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *native_value);
/* An unmodified retail component: retail leaf, standard channel, no owned
 * binding and no added effect pair. When its alias or side pair is absent it
 * keeps the ORIGINAL native resolver; native affinity and knowledge share it. */
static inline int p4_game_elements_original_component(const P4GameTables *tables,
    const P4GameActionFrame *frame)
{
    return frame->element.element_id < P4_ELEMENT_RETAIL_COUNT && frame->element.channel_id < 3u &&
        !frame->owned_binding &&
        !p4_game_tables_has_affinity_effect_pair(tables, frame->element.element_id, frame->element.channel_id);
}
/* Pure profile predicate (no source use): retail leaf 0..18 queried with its
 * natural component and channel, unchanged family/flags and no added effect
 * pair. Such a profile pair keeps the ORIGINAL native affinity when no side row
 * exists; any other pair needs a side row. Displays test coverage with it. */
int p4_game_elements_profile_natural_pair(const P4GameTables *tables, uint32_t leaf,
    uint32_t channel, uint32_t component);
/* Exact live enemy identity/alias plus actual metadata flags for reveal policy.
 * Unsupported metadata providers fail rather than fabricate zero flags. */
int p4_game_elements_enemy_context(const P4GameTables *tables, void *unit,
    P4GameEnemyContext *context);
struct P4GameItems;
/* Pure definition lookup from the exact sealed item snapshot, never a native
 * metadata/source getter or ambient active/pending item substitution. */
int p4_game_elements_resolve_weapon(const P4GameTables *tables, const struct P4GameItems *items,
    uint32_t item_id, uint32_t component, P4GameActionElement *element);
struct P4GameSkills;
/* Pure resolver over genuine sealed skill/item definitions; no source callbacks.
 * Native mode/options remain with the caller's local action frame; they do
 * not replace flags00's weapon-derived meaning or swap the original source
 * when reflected. NULL-actor weapon-derived queries need real actor context.
 * Component and actual status payload are explicit; Light is not itself KO.
 * As with affinity queries, this alone does not publish native capability. */
int p4_game_elements_resolve_action(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items, void *unit, uint32_t skill_id,
    uint32_t component, uint32_t status_mask, P4GameActionElement *element);
/* Resolve the owning pre-hit frame once from HP/SP/status metadata. Mixed
 * damage+KO has a damage pre-hit frame and a separate explicit KO rider.
 * Random status selection remains at the existing native selection call;
 * this query neither selects a payload nor consumes RNG. */
int p4_game_elements_resolve_primary_action(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items, void *unit, uint32_t skill_id, P4GameActionElement *element);
/* Frame variants resolve once, retaining actual source identity and caller
 * options as well as whether an owned component binding selected the leaf.
 * Failure changes no output; frame lifetime belongs to the native caller. */
int p4_game_elements_resolve_action_frame(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items, void *unit, uint32_t skill_id,
    uint32_t component, uint32_t status_mask, uint32_t native_options, P4GameActionFrame *frame);
int p4_game_elements_resolve_primary_frame(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items, void *unit, uint32_t skill_id,
    uint32_t native_options, P4GameActionFrame *frame);
/* The core supplies the exact owned FROZEN semantic registry and borrowed
 * family staging objects before native battle completion. Family preparation
 * freezes its object first; element preparation never treats staging as active.
 * Expanded routes require actual source/AI/knowledge/UI before any prepare.
 * The core retains all objects until quiesced session_end. */
int p4_game_elements_boot_stage(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items);
const P4GameTables *p4_game_elements_active(void);
/* Exact prepared owned view for later ordered cohort prepares (not active
 * gameplay capability). NULL until this element owner's prepare succeeds;
 * no pending/active/global-table substitution, cleared on session_end. */
const P4GameTables *p4_game_elements_prepared_snapshot(void);
/* Same successfully prepared tuple's exact borrowed skill view; not an active
 * or merely staged replacement. NULL also when that tuple has no skill view. */
const struct P4GameSkills *p4_game_elements_prepared_skills(void);
/* Whether the same prepared tuple's battle graph (actions, affinity effects and
 * active skills) requires (leaf, channel) side rows from every declared alias:
 * 1 required, 0 not, or a negative P4_TABLE_* when nothing is prepared. A pure
 * consumer query; displays never add pairs to that battle requirement. */
int p4_game_elements_prepared_required_pair(uint32_t leaf, uint32_t channel);
int p4_game_elements_session_end(void);
#endif
