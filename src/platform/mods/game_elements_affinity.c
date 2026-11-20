#include "p4pc/game_elements.h"
#include "p4pc/game_passive_grant.h"
#include "game_tables_runtime_internal.h"
#include <stdio.h>

/* Existing native game interfaces, with the same host-pointer transport used
 * by the accepted native battle adapters. No synthetic unit or RNG service. */
extern uint16_t datPersonaGetRecordId(uint16_t party_id);
extern uint32_t datCalcGetTableLength(void *unit);
extern uint8_t *datCalcSkillList(void *unit);
extern int16_t datGetEquippedItemByUnit(int16_t party_id, int16_t slot);
extern const uint8_t *p4_game_tables_items_native_metadata(uint32_t id);
extern int32_t datCalcGetAffinity(uint8_t *unit, int32_t element);

static uint16_t le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | (uint16_t)bytes[1] << 8);
}
int p4_game_elements_unit_identity(void *unit, P4GameUnitIdentity *identity)
{
    if (!unit || !identity) return P4_TABLE_INVALID;
    const uint8_t *record = unit;
    const uint32_t kind = (le16(record) & 4u) ? P4_ENTITY_ENEMY : P4_ENTITY_PERSONA;
    *identity = (P4GameUnitIdentity){kind,
        kind == P4_ENTITY_ENEMY ? le16(record + 2) : datPersonaGetRecordId(le16(record + 2)),
        kind == P4_ENTITY_ENEMY ? le16(record + 4) : 0};
    return P4_TABLE_OK;
}
static int affinity_base(const P4GameTables *tables, uint32_t kind, uint32_t id, uint32_t native_profile,
    const P4GameActionElement *element, const P4AffinityRow **base, int runtime)
{
    if (!tables || !element || !base || kind > P4_ENTITY_ENEMY || element->valid != 1u)
        return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    const P4AffinityProfileAlias *alias = p4_game_tables_find_affinity_alias(tables, kind, id, native_profile);
    if (!alias) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        int result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ALIAS, alias);
        if (result) return result;
    }
    const P4AffinityRow *row = p4_game_tables_find_affinity(tables,
        kind, id, alias->profile_id, element->element_id, element->channel_id);
    if (!row) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        int result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_AFFINITY, row);
        if (result) return result;
    }
    *base = row;
    return P4_TABLE_OK;
}
static int source_effect(const P4GameTables *tables, uint32_t kind,
    uint32_t source, const P4GameActionElement *element, uint32_t *flags, int runtime)
{
    const P4AffinityEffectBinding *effect = p4_game_tables_find_affinity_effect(tables,
        kind, source, element->element_id, element->channel_id);
    if (!effect) { *flags = 0; return P4_TABLE_OK; }
    if (runtime) {
        int result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_EFFECT, effect);
        if (result) return result;
        /* One diagnostic line per effect row a battle hit actually resolves
         * (mod-witness evidence); retail sessions have no effect rows. */
        static const P4AffinityEffectBinding *reported[16];
        static unsigned reported_count;
        unsigned seen = 0;
        while (seen < reported_count && reported[seen] != effect) ++seen;
        if (seen == reported_count && reported_count < 16u) {
            reported[reported_count++] = effect;
            fprintf(stderr, "p4pc: affinity effect of %s %u resolves element leaf %u flags 0x%08x\n",
                kind == P4_AFFINITY_EFFECT_EQUIPPED_ITEM ? "equipped item" : "skill", (unsigned)source,
                (unsigned)effect->element_id, (unsigned)effect->flags);
        }
    }
    *flags = effect->flags;
    return P4_TABLE_OK;
}
typedef struct GrantVisit {
    P4GameUnitSourceFn visit;
    void *user;
} GrantVisit;
static int grant_visit(void *user, uint32_t skill)
{
    const GrantVisit *grant = user;
    return grant->visit(grant->user, P4_AFFINITY_EFFECT_SKILL, skill);
}
int p4_game_elements_unit_sources(const uint8_t *skills, uint32_t skill_count, uint32_t enemy, uint32_t unit_id,
    P4GameUnitSourceFn visit, void *user)
{
    if (!skills || !visit) return P4_TABLE_INVALID;
    for (uint32_t i = 0; i < skill_count; ++i) {
        int result = visit(user, P4_AFFINITY_EFFECT_SKILL, le16(skills + i * 2u));
        if (result) return result;
    }
    GrantVisit grant = {visit, user};
    if (enemy) return p4_game_passive_unit_sources(1u, unit_id, skills, skill_count, 0u, grant_visit, &grant);
    const int16_t party = (int16_t)unit_id;
    uint32_t accessory_skill = 0;
    for (int16_t slot = 0; slot < 3; ++slot) {
        const int16_t item = datGetEquippedItemByUnit(party, slot);
        if (item < 0) continue; /* The real native empty-equipment sentinel. */
        int result = visit(user, P4_AFFINITY_EFFECT_EQUIPPED_ITEM, (uint32_t)item);
        if (result) return result;
        if (slot == 2) {
            /* This is the SAME loaded/expanded metadata getter as actions.
             * Native accessory-granted skills occupy the actual u16+24. */
            const uint8_t *metadata = p4_game_tables_items_native_metadata((uint32_t)item);
            accessory_skill = le16(metadata + 0x24);
            result = visit(user, P4_AFFINITY_EFFECT_SKILL, accessory_skill);
            if (result) return result;
        }
    }
    /* Then the unit's granted passives, each once and none it already has. */
    return p4_game_passive_unit_sources(0u, unit_id, skills, skill_count, accessory_skill, grant_visit, &grant);
}
typedef struct OffensiveVisit {
    const P4GameTables *tables;
    uint32_t element, flags;
    int runtime;
} OffensiveVisit;
static int offensive_visit(void *user, uint32_t kind, uint32_t source)
{
    OffensiveVisit *visit = user;
    const P4OffensiveEffectBinding *row =
        p4_game_tables_find_offensive_effect(visit->tables, kind, source, visit->element);
    if (!row) return P4_TABLE_OK;
    if (visit->runtime) {
        int result = p4_game_tables_runtime_use_row(visit->tables, P4_TABLE_SOURCE_OFFENSE, row);
        if (result) return result;
    }
    visit->flags |= row->flags;
    return P4_TABLE_OK;
}
int p4_game_elements_offensive_effects(const P4GameTables *tables, void *unit,
    uint32_t element, int runtime, uint32_t *flags)
{
    if (!tables || !unit || !flags || (runtime != 0 && runtime != 1)) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    if (!p4_game_tables_has_offensive_effect_leaf(tables, element)) { *flags = 0; return P4_TABLE_OK; }
    const uint8_t *record = unit;
    OffensiveVisit visit = {tables, element, 0, runtime};
    int result = p4_game_elements_unit_sources(datCalcSkillList(unit), datCalcGetTableLength(unit) & 65535u,
        (le16(record) & 4u) != 0, le16(record + 2), offensive_visit, &visit);
    if (result) return result;
    *flags = visit.flags;
    return P4_TABLE_OK;
}
typedef struct EffectVisit {
    const P4GameTables *tables;
    const P4GameActionElement *element;
    uint32_t flags;
    int runtime;
} EffectVisit;
static int effect_visit(void *user, uint32_t kind, uint32_t source)
{
    EffectVisit *visit = user;
    uint32_t selected;
    int result = source_effect(visit->tables, kind, source, visit->element, &selected, visit->runtime);
    if (!result) visit->flags |= selected;
    return result;
}
static int additional_effects(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *out, int runtime)
{
    if (!p4_game_tables_has_affinity_effect_pair(tables, element->element_id, element->channel_id)) {
        *out = 0; return P4_TABLE_OK;
    }
    const uint8_t *record = unit;
    EffectVisit visit = {tables, element, 0, runtime};
    int result = p4_game_elements_unit_sources(datCalcSkillList(unit), datCalcGetTableLength(unit) & 65535u,
        (le16(record) & 4u) != 0, le16(record + 2), effect_visit, &visit);
    if (result) return result;
    *out = visit.flags;
    return P4_TABLE_OK;
}
/* This is ONLY a built-in effect-coverage selector. It is never returned as the
 * true leaf or used by knowledge, action classification, icons or scripts.
 * Unknown channels have no implied native built-in coverage; their explicit
 * effect rows still participate in the same final precedence. */
static int32_t effect_selector(const P4GameActionElement *element)
{
    const uint32_t leaf = element->element_id, family = element->family;
    if (element->channel_id == P4_CHANNEL_DAMAGE) {
        if (p4_game_action_element_is_physical(element)) return 0;
        if (leaf < 6u) return (int32_t)leaf;
        if (family >= 1u && family <= 5u) return (int32_t)family;
        return -1; /* Universal non-Almighty coverage, no element-specific passive. */
    }
    if (element->channel_id == P4_CHANNEL_INSTANT_KO) {
        if (leaf == 6u || leaf == 7u) return (int32_t)leaf;
        if (family == 6u || family == 7u) return (int32_t)family;
    }
    if (element->channel_id == P4_CHANNEL_STATUS && leaf >= 8u && leaf <= 15u) return (int32_t)leaf;
    return 5; /* No built-in coverage, including Masakados, for an undeclared pair. */
}
static uint32_t ko_word(const P4AffinityRow *row)
{
    static const uint32_t flags[] = {0, P4_AFF_WEAK, P4_AFF_RESIST, P4_AFF_NULL};
    return flags[row->value] | row->scalar; /* Frozen KO enum/scalar were validated. */
}
static int affinity_effective(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *native_value, int runtime);
/* One leaf on a standard channel as the native route reads it: without an
 * authored row an unmodified retail component keeps the original resolver. */
static int leaf_affinity(const P4GameTables *tables, void *unit, uint32_t leaf, uint32_t channel,
    uint32_t component, uint32_t *value, int runtime)
{
    P4GameActionElement declared = {.valid = 1, .element_id = leaf, .channel_id = channel, .component = component};
    P4GameActionFrame frame = {0};
    int result = runtime ? p4_game_tables_classify_runtime_action_element(tables, &declared, &frame.element) :
        p4_game_tables_classify_action_element(tables, &declared, &frame.element);
    if (result) return result;
    if (!p4_game_action_element_has_affinity(&frame.element)) { *value = 100; return P4_TABLE_OK; }
    result = affinity_effective(tables, unit, &frame.element, value, runtime);
    if (result == P4_TABLE_NOT_FOUND && p4_game_elements_original_component(tables, &frame)) {
        *value = (uint32_t)datCalcGetAffinity(unit, (int32_t)leaf);
        return P4_TABLE_OK;
    }
    return result;
}
/* What 0x00238940 treats as no infliction: null, repel, drain or the retail
 * ailment no-effect flag. */
#define AFF_IMMUNE 0x27000000u
/* A side ailment's status-channel base when the unit has no row for it
 * (docs/modding/tables.md "Side ailments"): immune when every retail ailment
 * leaf (8..15) is, else mapped from its default leaf's effective damage
 * affinity: weak 125, resist 50, null/repel/drain immune, otherwise 100.
 * NOT_FOUND when the leaf declares no default. */
static int side_default(const P4GameTables *tables, void *unit, const P4GameActionElement *element,
    uint32_t *value, int runtime)
{
    const P4SideAilmentDef *side = element->channel_id == P4_CHANNEL_STATUS ?
        p4_game_tables_find_side_ailment(tables, element->element_id) : NULL;
    if (!side || side->default_from == P4_SIDE_DEFAULT_NONE) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        int result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_SIDE_AILMENT, side);
        if (result) return result;
    }
    uint32_t from = AFF_IMMUNE;
    int immune = 1;
    for (uint32_t leaf = 8u; immune && leaf <= 15u; ++leaf) {
        int result = leaf_affinity(tables, unit, leaf, P4_CHANNEL_STATUS, P4_ACTION_COMPONENT_STATUS, &from, runtime);
        if (result) return result;
        immune = (from & AFF_IMMUNE) != 0;
    }
    if (!immune) {
        int result = leaf_affinity(tables, unit, side->default_from, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE,
            &from, runtime);
        if (result) return result;
    }
    *value = (from & AFF_IMMUNE) ? P4_AFF_NULL | 100u : (from & P4_AFF_WEAK) ? P4_AFF_WEAK | 125u :
        (from & P4_AFF_RESIST) ? P4_AFF_RESIST | 50u : 100u;
    return P4_TABLE_OK;
}
static int affinity_effective(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *native_value, int runtime)
{
    if (!tables || !unit || !native_value || !element || element->valid != 1u) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    P4GameUnitIdentity identity;
    int result = p4_game_elements_unit_identity(unit, &identity);
    if (result) return result;
    const P4AffinityRow *base = NULL;
    uint32_t value = 0;
    result = affinity_base(tables, identity.entity_kind, identity.entity_id,
        identity.native_profile, element, &base, runtime);
    if (result == P4_TABLE_NOT_FOUND) result = side_default(tables, unit, element, &value, runtime);
    if (result) return result;
    const P4AffinityChannelDesc *channel = p4_game_tables_find_channel(tables, element->channel_id);
    if (!channel) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_CHANNEL, channel);
        if (result) return result;
    }
    if (base) value = channel->kind == P4_CHANNEL_KO ? ko_word(base) : base->value;
    uint32_t added;
    result = additional_effects(tables, unit, element, &added, runtime);
    if (result) return result;
    value = p4_game_elements_native_affinity_apply(unit, effect_selector(element), value, added);
    if (channel->kind == P4_CHANNEL_KO && (value & (P4_AFF_NULL | P4_AFF_REPEL | P4_AFF_DRAIN)))
        value = (value & ~(P4_AFF_NULL | P4_AFF_REPEL | P4_AFF_DRAIN)) | P4_AFF_NULL;
    *native_value = value;
    return P4_TABLE_OK;
}
int p4_game_elements_affinity_base(const P4GameTables *tables,
    uint32_t kind, uint32_t id, uint32_t native_profile,
    const P4GameActionElement *element, const P4AffinityRow **base)
{ return affinity_base(tables, kind, id, native_profile, element, base, 0); }
int p4_game_elements_affinity_effective(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *native_value)
{ return affinity_effective(tables, unit, element, native_value, 0); }
int p4_game_elements_runtime_affinity_effective(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *native_value)
{ return affinity_effective(tables, unit, element, native_value, 1); }
