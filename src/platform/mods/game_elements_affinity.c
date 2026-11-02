#include "p4pc/game_elements.h"
#include "game_tables_runtime_internal.h"

/* Existing native game interfaces, with the same host-pointer transport used
 * by the accepted native battle adapters. No synthetic unit or RNG service. */
extern uint16_t datPersonaGetRecordId(uint16_t party_id);
extern uint32_t datCalcGetTableLength(void *unit);
extern uint8_t *datCalcSkillList(void *unit);
extern int16_t datGetEquippedItemByUnit(int16_t party_id, int16_t slot);
extern const uint8_t *p4_game_tables_items_native_metadata(uint32_t id);

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
    }
    *flags = effect->flags;
    return P4_TABLE_OK;
}
static int additional_effects(const P4GameTables *tables, void *unit,
    const P4GameActionElement *element, uint32_t *out, int runtime)
{
    if (!p4_game_tables_has_affinity_effect_pair(tables, element->element_id, element->channel_id)) {
        *out = 0; return P4_TABLE_OK;
    }
    uint32_t flags = 0, selected;
    const uint32_t count = datCalcGetTableLength(unit) & 65535u;
    const uint8_t *skills = datCalcSkillList(unit);
    for (uint32_t i = 0; i < count; ++i) {
        int result = source_effect(tables, P4_AFFINITY_EFFECT_SKILL, le16(skills + i * 2u), element, &selected, runtime);
        if (result) return result;
        flags |= selected;
    }
    const uint8_t *record = unit;
    if (!(le16(record) & 4u)) {
        const int16_t party = (int16_t)le16(record + 2);
        for (int16_t slot = 0; slot < 3; ++slot) {
            const int16_t item = datGetEquippedItemByUnit(party, slot);
            if (item < 0) continue; /* The real native empty-equipment sentinel. */
            int result = source_effect(tables, P4_AFFINITY_EFFECT_EQUIPPED_ITEM, (uint32_t)item, element, &selected, runtime);
            if (result) return result;
            flags |= selected;
            if (slot == 2) {
                /* This is the SAME loaded/expanded metadata getter as actions.
                 * Native accessory-granted skills occupy the actual u16+24. */
                const uint8_t *metadata = p4_game_tables_items_native_metadata((uint32_t)item);
                result = source_effect(tables, P4_AFFINITY_EFFECT_SKILL, le16(metadata + 0x24), element, &selected, runtime);
                if (result) return result;
                flags |= selected;
            }
        }
    }
    *out = flags;
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
    const P4GameActionElement *element, uint32_t *native_value, int runtime)
{
    if (!tables || !unit || !native_value || !element || element->valid != 1u) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    P4GameUnitIdentity identity;
    int result = p4_game_elements_unit_identity(unit, &identity);
    if (result) return result;
    const P4AffinityRow *base;
    result = affinity_base(tables, identity.entity_kind, identity.entity_id,
        identity.native_profile, element, &base, runtime);
    if (result) return result;
    const P4AffinityChannelDesc *channel = p4_game_tables_find_channel(tables, element->channel_id);
    if (!channel) return P4_TABLE_NOT_FOUND;
    if (runtime) {
        result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_CHANNEL, channel);
        if (result) return result;
    }
    uint32_t value = channel->kind == P4_CHANNEL_KO ? ko_word(base) : base->value;
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
