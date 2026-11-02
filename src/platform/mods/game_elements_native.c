#include "game_elements_action_internal.h"
#include "game_tables_runtime_internal.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_items.h"
#include "p4pc/gmem.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/package_boot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int32_t datCalcGetAffinity(uint8_t *unit, int32_t element);
extern int32_t datCalcGetElementAffinity(uint8_t *unit, int32_t element);

typedef struct RequiredPair { uint32_t leaf, channel; } RequiredPair;
static int pair_compare(const void *left, const void *right);
typedef struct ElementStage {
    const P4GameTables *tables;
    const P4GameSkills *skills;
    const P4GameItems *items;
    int expanded, prepared;
    RequiredPair *required; /* Prepared battle requirement, sorted and unique. */
    size_t required_count;
} ElementStage;
static ElementStage staged;
static const P4GameTables *active_tables;
static const P4GameSkills *active_skills;
static const P4GameItems *active_items;
static int route_active;
static int prepare_complete(void *user);
static void commit_complete(void *user);
/* The element-route participant is this build's complete native battle
 * call-site cutover (amount/formula, hit, critical, applicability, status/KO,
 * endurance, presentation, follow-up and knowledge-gate routes). It consumes
 * only the element owner's prepared tuple, which the shared order prepares
 * first, and its commit is what turns routing on. */
static int route_prepare(void *user)
{
    (void)user;
    return p4_game_elements_prepared_snapshot() ? P4_TABLE_OK : P4_TABLE_NOT_READY;
}
static void route_commit(void *user)
{
    (void)user;
    route_active = 1;
}
static _Noreturn void refuse(int result)
{
    char reason[192];
    snprintf(reason, sizeof reason, "Native element route refused (%d): %s.", result, p4_game_tables_error(result));
    p4_package_boot_runtime_failed(reason);
}
_Noreturn void p4_game_elements_native_calculation_refuse(int32_t result)
{ refuse(result); }
void p4_game_elements_native_skill_use(uint32_t skill_id)
{
    if (!active_tables) refuse(P4_TABLE_NOT_READY);
    int result = p4_game_skills_runtime_use(active_skills, skill_id);
    if (result) refuse(result);
}
int p4_game_elements_boot_stage(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items)
{
    if (!tables) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    if (active_tables) return P4_TABLE_FROZEN;
    if (staged.tables) return staged.tables == tables && staged.skills == skills && staged.items == items ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
    int result = p4_game_tables_battle_complete_register("element", prepare_complete, commit_complete, NULL);
    if (result) return result;
    const int requested = p4_game_elements_requested(tables);
    if (requested) {
        result = p4_game_tables_battle_complete_register("element-route", route_prepare, route_commit, NULL);
        if (!result) result = p4_game_tables_battle_complete_require("element-route", "element");
        if (!result) result = p4_game_tables_battle_complete_require("element", "table-source");
        if (!result) result = p4_game_tables_battle_complete_require("element", "element-route");
        if (!result) result = p4_game_tables_battle_complete_require("element", "element-ai");
        if (!result) result = p4_game_tables_battle_complete_require("element", "element-knowledge");
        if (!result) result = p4_game_tables_battle_complete_require("element", "element-ui");
        if (result) {
            int route_rollback = p4_game_tables_battle_complete_unregister("element-route", route_prepare, route_commit, NULL);
            int rollback = p4_game_tables_battle_complete_unregister("element", prepare_complete, commit_complete, NULL);
            return route_rollback ? route_rollback : rollback ? rollback : result;
        }
    }
    staged = (ElementStage){tables, skills, items, requested, 0, NULL, 0};
    return P4_TABLE_OK;
}
const P4GameTables *p4_game_elements_active(void) { return active_tables; }
const P4GameTables *p4_game_elements_prepared_snapshot(void) { return staged.prepared && staged.expanded ? staged.tables : NULL; }
const P4GameSkills *p4_game_elements_prepared_skills(void) { return staged.prepared && staged.expanded ? staged.skills : NULL; }
int p4_game_elements_prepared_required_pair(uint32_t leaf, uint32_t channel)
{
    if (!staged.prepared || !staged.expanded) return P4_TABLE_NOT_READY;
    const RequiredPair key = {leaf, channel};
    return staged.required_count &&
        bsearch(&key, staged.required, staged.required_count, sizeof key, pair_compare) != NULL;
}
int p4_game_elements_native_enabled(void) { return active_tables != NULL && route_active; }
int p4_game_elements_native_weapon_element(uint32_t item_id, uint32_t component, P4GameActionElement *element)
{
    return active_tables ? p4_game_elements_resolve_runtime_weapon(active_tables, active_items, item_id, component, element) :
        P4_TABLE_NOT_READY;
}
int p4_game_elements_enemy_metadata_flags(uint32_t enemy_id, uint32_t *flags)
{
    if (!flags) return P4_TABLE_INVALID;
    /* Actual001F0950 status metadata: leadingu16, stride0x3c, not+18. Added
     * enemies are readable once the enemy completion published their banks. */
    if (enemy_id >= p4_game_enemies_native_limit()) return P4_TABLE_NOT_READY;
    const uint32_t metadata = P4_G32(0x007644b4u);
    if (!metadata) return P4_TABLE_NOT_READY;
    *flags = P4_G16(metadata + enemy_id * 60u);
    return P4_TABLE_OK;
}
static int enemy_context(const P4GameTables *tables, void *unit, P4GameEnemyContext *context, int runtime)
{
    if (!tables || !unit || !context) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    P4GameUnitIdentity identity;
    int result = p4_game_elements_unit_identity(unit, &identity);
    if (result) return result;
    if (identity.entity_kind != P4_ENTITY_ENEMY) return P4_TABLE_INVALID;
    uint32_t metadata_flags;
    result = p4_game_elements_enemy_metadata_flags(identity.entity_id, &metadata_flags);
    if (result) return result;
    const P4AffinityProfileAlias *alias = p4_game_tables_find_affinity_alias(tables,
        P4_ENTITY_ENEMY, identity.entity_id, identity.native_profile);
    if (!alias) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ALIAS, alias);
        if (result) return result;
    }
    *context = (P4GameEnemyContext){identity.entity_id, identity.native_profile,
        alias->profile_id, alias->knowledge_id, metadata_flags};
    return P4_TABLE_OK;
}
int p4_game_elements_enemy_context(const P4GameTables *tables, void *unit, P4GameEnemyContext *context)
{ return enemy_context(tables, unit, context, 0); }
int p4_game_elements_native_enemy_context(void *unit, P4GameEnemyContext *context)
{
    return active_tables ? enemy_context(active_tables, unit, context, 1) : P4_TABLE_NOT_READY;
}
static int skill_source(uint32_t id, int action)
{
    if (id < P4_SKILL_RETAIL_COUNT)
        return !action || id < P4_SKILL_RETAIL_ACTIVE_COUNT ? P4_TABLE_OK : P4_TABLE_INVALID;
    if (!staged.skills || !p4_game_skills_revision(staged.skills)) return P4_TABLE_NOT_READY;
    const P4GameSkillView *row = p4_game_skills_find(staged.skills, id);
    if (!row) return P4_TABLE_NOT_FOUND;
    return !action || row->desc.active ? P4_TABLE_OK : P4_TABLE_INVALID;
}
static int item_source(uint32_t id, int weapon)
{
    if (id < P4_ITEM_RETAIL_COUNT) return P4_TABLE_OK;
    if (!staged.items || !p4_game_tables_items_revision(staged.items)) return P4_TABLE_NOT_READY;
    const P4ItemRow *row = p4_game_tables_items_metadata(staged.items, id);
    if (!row) return P4_TABLE_NOT_FOUND;
    return !weapon || row->kind == 0u ? P4_TABLE_OK : P4_TABLE_INVALID;
}
static int pair_compare(const void *left, const void *right)
{
    const RequiredPair *a = left, *b = right;
    if (a->leaf != b->leaf) return a->leaf < b->leaf ? -1 : 1;
    return a->channel == b->channel ? 0 : a->channel < b->channel ? -1 : 1;
}
static void require_pair(RequiredPair *pairs, size_t *count, uint32_t leaf, uint32_t channel)
{
    const P4ElementDesc *element = p4_game_tables_find_element(staged.tables, leaf);
    if (element->flags & P4_ELEMENT_AFFINITY) pairs[(*count)++] = (RequiredPair){leaf, channel};
}
static int validate_graph(RequiredPair *pairs, size_t *count)
{
    const size_t actions = p4_game_tables_action_element_count(staged.tables);
    for (size_t i = 0; i < actions; ++i) {
        const P4ActionElementBinding *binding = p4_game_tables_action_element_at(staged.tables, i);
        if (binding->component > P4_ACTION_COMPONENT_STATUS) return P4_TABLE_NOT_READY;
        int result;
        if (binding->source_kind == P4_ACTION_ELEMENT_SKILL) result = skill_source(binding->source_id, 1);
        else if (binding->source_kind == P4_ACTION_ELEMENT_WEAPON) result = item_source(binding->source_id, 1);
        else {
            /* Expanded enemy action metadata needs its actual table-family
             * implementation, not a semantic registration or profile alias. */
            result = binding->source_id < P4_ENEMY_RETAIL_COUNT ? P4_TABLE_OK : P4_TABLE_NOT_READY;
        }
        if (result) return result;
        require_pair(pairs, count, binding->element_id, binding->channel_id);
    }
    const size_t effects = p4_game_tables_affinity_effect_count(staged.tables);
    for (size_t i = 0; i < effects; ++i) {
        const P4AffinityEffectBinding *effect = p4_game_tables_affinity_effect_at(staged.tables, i);
        int result = effect->source_kind == P4_AFFINITY_EFFECT_SKILL ?
            skill_source(effect->source_id, 0) : item_source(effect->source_id, 0);
        if (result) return result;
        require_pair(pairs, count, effect->element_id, effect->channel_id);
    }
    const size_t skills = staged.skills ? p4_game_skills_count(staged.skills) : 0;
    if (skills && !p4_game_skills_revision(staged.skills)) return P4_TABLE_NOT_READY;
    for (size_t i = 0; i < skills; ++i) {
        const P4GameSkillView *row = p4_game_skills_at(staged.skills, i);
        if (!row->desc.active) continue;
        P4GameActionFrame frame;
        int result = p4_game_elements_resolve_primary_frame(staged.tables, staged.skills, staged.items, NULL, row->desc.id, 0, &frame);
        /* An unbound weapon-derived action requires its real actor later.
         * Its explicit weapon/enemy bindings were validated above. */
        if (result == P4_TABLE_NOT_READY && (row->desc.metadata[0] & 2u)) continue;
        if (result) return result;
        if (!frame.element.valid) continue;
        if (!frame.owned_binding && frame.element.component == P4_ACTION_COMPONENT_DAMAGE &&
            (frame.element.element_id == 6u || frame.element.element_id == 7u))
            return P4_TABLE_NOT_READY; /* New Light/Dark damage needs explicit channel ownership. */
        if (!frame.owned_binding && frame.element.element_id >= P4_ELEMENT_RETAIL_COUNT &&
            frame.element.component != P4_ACTION_COMPONENT_DAMAGE)
            return P4_TABLE_NOT_READY; /* Actual status/KO payload needs its component binding. */
        if (frame.owned_binding || frame.element.element_id >= P4_ELEMENT_RETAIL_COUNT ||
            frame.element.channel_id >= 3u)
            require_pair(pairs, count, frame.element.element_id, frame.element.channel_id);
    }
    return P4_TABLE_OK;
}
static int prepare_complete(void *user)
{
    (void)user;
    if (staged.prepared) return P4_TABLE_OK; /* Same immutable graph on every loader rebind. */
    if (!staged.expanded) { staged.prepared = 1; return P4_TABLE_OK; }
    int source_result = p4_game_tables_validate_runtime_sources(staged.tables);
    if (source_result) return source_result;
    size_t capacity = p4_game_tables_action_element_count(staged.tables);
    const size_t effects = p4_game_tables_affinity_effect_count(staged.tables);
    const size_t skills = staged.skills ? p4_game_skills_count(staged.skills) : 0;
    if (effects > SIZE_MAX - capacity) return P4_TABLE_NOMEM;
    capacity += effects;
    if (skills > SIZE_MAX - capacity) return P4_TABLE_NOMEM;
    capacity += skills;
    if (capacity > SIZE_MAX / sizeof(RequiredPair)) return P4_TABLE_NOMEM;
    RequiredPair *pairs = capacity ? malloc(capacity * sizeof *pairs) : NULL;
    if (capacity && !pairs) return P4_TABLE_NOMEM;
    size_t count = 0;
    int result = validate_graph(pairs, &count);
    if (!result && count) {
        qsort(pairs, count, sizeof *pairs, pair_compare);
        size_t unique = 0;
        for (size_t i = 0; i < count; ++i)
            if (!unique || pair_compare(pairs + i, pairs + unique - 1)) {
                if (unique != i) pairs[unique] = pairs[i];
                ++unique;
            }
        count = unique;
    }
    const size_t aliases = p4_game_tables_affinity_alias_count(staged.tables);
    if (!result && count && !aliases) result = P4_TABLE_NOT_READY;
    for (size_t i = 0; !result && i < aliases; ++i) {
        const P4AffinityProfileAlias *alias = p4_game_tables_affinity_alias_at(staged.tables, i);
        if (alias->entity_id > 65535u || alias->native_profile > 65535u ||
            (alias->entity_kind == P4_ENTITY_PERSONA && alias->native_profile)) {
            result = P4_TABLE_NOT_READY;
            break;
        }
        for (size_t pair = 0; pair < count; ++pair)
            if (!p4_game_tables_find_affinity(staged.tables, alias->entity_kind, alias->entity_id,
                alias->profile_id, pairs[pair].leaf, pairs[pair].channel)) {
                result = P4_TABLE_NOT_READY;
                break;
            }
    }
    /* datUnitInit 0x00231AF0 starts every enemy row with id2 = id; only an AI
     * command changes it later. datPersonaInitAllRecords 0x0010FFA0 creates a
     * record for every persona 1..255 (0 is none), any of which a party
     * defender or the compendium can present. Each must answer the battle
     * requirement before publication (declared aliases' pairs were checked
     * above), so a mid-battle missing alias is limited to an undeclared
     * AI-scripted enemy id2 form. Added enemies and personas the staged
     * snapshots declare are covered the same way (an added persona reaches a
     * party defender through fusion or Shuffle Time). */
    const uint32_t enemies = p4_game_enemies_native_declared_limit();
    for (uint32_t enemy = 0; !result && count && enemy < enemies; ++enemy)
        if (!p4_game_tables_find_affinity_alias(staged.tables, P4_ENTITY_ENEMY, enemy, enemy))
            result = P4_TABLE_NOT_READY;
    const uint32_t personas = p4_game_personas_native_declared_limit();
    for (uint32_t persona = 1; !result && count && persona < personas; ++persona)
        if (!p4_game_tables_find_affinity_alias(staged.tables, P4_ENTITY_PERSONA, persona, 0))
            result = P4_TABLE_NOT_READY;
    if (result) {
        free(pairs);
        return result;
    }
    staged.required = pairs;
    staged.required_count = count;
    staged.prepared = 1;
    return P4_TABLE_OK;
}
static void commit_complete(void *user)
{
    (void)user;
    if (!staged.expanded) return;
    active_skills = staged.skills;
    active_items = staged.items;
    active_tables = staged.tables;
}
int p4_game_elements_session_end(void)
{
    /* The core owns task/queue quiescence and the shared coordinator clear.
     * No guest array/base/count belongs to this borrowed semantic tuple. */
    active_tables = NULL; active_skills = NULL; active_items = NULL; route_active = 0;
    free(staged.required);
    memset(&staged, 0, sizeof staged);
    return P4_TABLE_OK;
}
int p4_game_elements_native_preview_primary_frame(void *unit, uint32_t skill_id,
    uint32_t native_options, P4GameActionFrame *frame)
{
    if (!active_tables) return P4_TABLE_NOT_READY;
    return p4_game_elements_resolve_primary_frame(active_tables, active_skills, active_items,
        unit, skill_id, native_options, frame);
}
void p4_game_elements_native_primary_frame(void *unit, uint32_t skill_id,
    uint32_t native_options, P4GameActionFrame *frame)
{
    if (!active_tables) refuse(P4_TABLE_NOT_READY);
    int result = p4_game_elements_resolve_runtime_primary_frame(active_tables, active_skills, active_items, unit, skill_id, native_options, frame);
    if (result) refuse(result);
}
void p4_game_elements_native_applicability_frame(void *unit, uint32_t skill_id, P4GameActionFrame *frame)
{
    if (!active_tables) refuse(P4_TABLE_NOT_READY);
    int result = p4_game_elements_resolve_runtime_applicability_frame(active_tables, active_skills, active_items, unit, skill_id, frame);
    if (result) refuse(result);
}
/* Presentation-only consumers ask the same owning frame as the action. An
 * actorless query keeps the retail skill-table fallback of 0023D8E0(NULL). */
static void presentation_frame(void *unit, uint32_t skill_id, P4GameActionFrame *frame)
{
    if (unit) p4_game_elements_native_primary_frame(unit, skill_id, 0, frame);
    else p4_game_elements_native_applicability_frame(NULL, skill_id, frame);
}
int32_t p4_game_elements_native_presentation_leaf(void *unit, uint32_t skill_id)
{
    P4GameActionFrame frame;
    presentation_frame(unit, skill_id, &frame);
    if (!frame.element.valid) return -1;
    if (frame.element.element_id < P4_ELEMENT_RETAIL_COUNT) return (int32_t)frame.element.element_id;
    /* A new leaf presents as its declared retail family; otherwise every
     * caller's switch takes its default branch. Never narrow the u32 leaf. */
    return frame.element.family < P4_ELEMENT_RETAIL_COUNT ? (int32_t)frame.element.family : -1;
}
uint32_t p4_game_elements_native_hit_class_bits(void *unit, uint32_t skill_id)
{
    P4GameActionFrame frame;
    presentation_frame(unit, skill_id, &frame);
    if (!frame.element.valid) return 1u; /* Retail 0023D6E0(-1). */
    if (frame.element.element_id < P4_ELEMENT_RETAIL_COUNT) return 1u << (frame.element.element_id + 1u);
    /* 00201B00 tests only 0x1FE00, the ailment leaves 8..15: affinity-bearing
     * but not offensive. A new leaf reports that class, never a retail alias. */
    return p4_game_action_element_has_affinity(&frame.element) &&
        !p4_game_action_element_is_offensive(&frame.element) ? 1u << 9 : 0u;
}
void p4_game_elements_native_action_frame(void *unit, uint32_t skill_id,
    uint32_t component, uint32_t status_mask, uint32_t native_options, P4GameActionFrame *frame)
{
    if (!active_tables) refuse(P4_TABLE_NOT_READY);
    int result = p4_game_elements_resolve_runtime_action_frame(active_tables, active_skills, active_items, unit,
        skill_id, component, status_mask, native_options, frame);
    if (result) refuse(result);
}
static uint32_t native_affinity(void *unit, const P4GameActionFrame *frame, int wrapper)
{
    if (!active_tables) refuse(P4_TABLE_NOT_READY);
    if (!frame || frame->element.valid > 1u) refuse(P4_TABLE_INVALID);
    if (!frame->element.valid) return 0;
    if (!p4_game_action_element_has_affinity(&frame->element)) return 100;
    uint32_t value;
    int result = p4_game_elements_runtime_affinity_effective(active_tables, unit, &frame->element, &value);
    if (result == P4_TABLE_NOT_FOUND && p4_game_elements_original_component(active_tables, frame)) {
        /* An unmodified retail component retains its exact original resolver.
         * Merely registering physical leaves must not retune vanilla Light/KO. */
        return wrapper ? (uint32_t)datCalcGetElementAffinity(unit, (int32_t)frame->element.element_id) :
            (uint32_t)datCalcGetAffinity(unit, (int32_t)frame->element.element_id);
    }
    if (result) refuse(result);
    return value;
}
uint32_t p4_game_elements_native_affinity(void *unit, const P4GameActionFrame *frame)
{
    return native_affinity(unit, frame, 0);
}
uint32_t p4_game_elements_native_element_affinity(void *unit, const P4GameActionFrame *frame)
{
    return native_affinity(unit, frame, 1);
}
uint32_t p4_game_elements_native_affinity_query(void *unit,
    const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{
    if (!active_tables) refuse(P4_TABLE_NOT_READY);
    if (!frame || frame->element.valid > 1u) refuse(P4_TABLE_INVALID);
    if (query && query->mode == P4_AFFINITY_QUERY_PROVIDED) return query->value;
    if (query && query->mode != P4_AFFINITY_QUERY_ACTUAL) refuse(P4_TABLE_INVALID);
    return native_affinity(unit, frame, 0);
}
uint32_t p4_game_elements_native_affinity_leaf(void *unit, uint32_t leaf, uint32_t channel, uint32_t component)
{
    if (!active_tables) refuse(P4_TABLE_NOT_READY);
    P4GameActionElement declared = {.valid = 1, .element_id = leaf, .channel_id = channel, .component = component}, element;
    int result = p4_game_tables_classify_runtime_action_element(active_tables, &declared, &element);
    if (result) refuse(result);
    if (!p4_game_action_element_has_affinity(&element)) return 100;
    uint32_t value;
    result = p4_game_elements_runtime_affinity_effective(active_tables, unit, &element, &value);
    if (result) refuse(result);
    return value;
}
_Noreturn void p4_game_elements_native_mask_refuse(uint32_t leaf)
{
    char reason[192];
    snprintf(reason, sizeof reason, "Legacy element-bit query cannot represent semantic leaf %u; use tagged predicates.",
        (unsigned)leaf);
    p4_package_boot_runtime_failed(reason);
}
int p4_game_elements_profile_natural_pair(const P4GameTables *tables, uint32_t leaf, uint32_t channel, uint32_t component)
{
    if (!tables || leaf >= P4_ELEMENT_RETAIL_COUNT) return 0;
    P4GameActionElement declared = {.valid = 1, .element_id = leaf, .channel_id = channel, .component = component}, element;
    if (p4_game_tables_classify_action_element(tables, &declared, &element)) return 0;
    const uint32_t natural_component = leaf < 6u ? P4_ACTION_COMPONENT_DAMAGE :
        leaf < 8u ? P4_ACTION_COMPONENT_INSTANT_KO : P4_ACTION_COMPONENT_STATUS;
    const uint32_t natural_channel = leaf < 6u ? P4_CHANNEL_DAMAGE :
        leaf < 8u ? P4_CHANNEL_INSTANT_KO : P4_CHANNEL_STATUS;
    const uint32_t original_flags = (leaf == 0u ? P4_ELEMENT_PHYSICAL : 0u) | (leaf < 8u ? P4_ELEMENT_OFFENSIVE : 0u) |
        (leaf < 16u ? P4_ELEMENT_AFFINITY : 0u);
    return component == natural_component && channel == natural_channel && element.family == leaf &&
        (element.flags & (P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY)) == original_flags &&
        !p4_game_tables_has_affinity_effect_pair(tables, leaf, channel);
}
static int profile_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *native_value, int wrapper)
{
    if (!native_value) return P4_TABLE_INVALID;
    if (!active_tables) return P4_TABLE_NOT_READY;
    P4GameActionElement declared = {.valid = 1, .element_id = leaf, .channel_id = channel, .component = component}, element;
    int result = p4_game_tables_classify_runtime_action_element(active_tables, &declared, &element);
    if (result) return result;
    if (!p4_game_action_element_has_affinity(&element)) { *native_value = 100; return P4_TABLE_OK; }
    uint32_t value;
    result = p4_game_elements_runtime_affinity_effective(active_tables, unit, &element, &value);
    if (result == P4_TABLE_NOT_FOUND && p4_game_elements_profile_natural_pair(active_tables, leaf, channel, component)) {
        if (!unit) return P4_TABLE_INVALID;
        value = wrapper ? (uint32_t)datCalcGetElementAffinity(unit, (int32_t)leaf) :
            (uint32_t)datCalcGetAffinity(unit, (int32_t)leaf);
        result = P4_TABLE_OK;
    }
    if (result) return result;
    *native_value = value;
    return P4_TABLE_OK;
}
int p4_game_elements_native_profile_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *native_value)
{
    return profile_affinity(unit, leaf, channel, component, native_value, 1);
}
int p4_game_elements_native_profile_base_affinity(void *unit, uint32_t leaf,
    uint32_t channel, uint32_t component, uint32_t *native_value)
{
    return profile_affinity(unit, leaf, channel, component, native_value, 0);
}
