#include "game_elements_action_internal.h"
#include "game_tables_runtime_internal.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_items.h"
#include "p4pc/gmem.h"

static uint16_t le16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | (uint16_t)bytes[1] << 8);
}
static uint32_t le32(const uint8_t *bytes)
{
    return (uint32_t)le16(bytes) | (uint32_t)le16(bytes + 2) << 16;
}
typedef struct SkillElementFacts {
    uint32_t valid, leaf, active, flags;
    const uint8_t *metadata;
} SkillElementFacts;
static int skill_facts(const P4GameSkills *skills, uint32_t id, SkillElementFacts *facts)
{
    P4GameSkillDefinition definition;
    int result = p4_game_skills_definition_find(skills, id, &definition);
    if (result) return result;
    if ((!definition.authored && !definition.retail_element) || (definition.active && !definition.metadata))
        return P4_TABLE_NOT_FOUND;
    const uint32_t raw = definition.authored ? definition.authored->element_id : definition.retail_element[0];
    if (!definition.authored && raw > 127u && raw != P4_SKILL_LEGACY_ELEMENT_INVALID) return P4_TABLE_INVALID;
    *facts = (SkillElementFacts){definition.authored || raw != P4_SKILL_LEGACY_ELEMENT_INVALID,
        raw, definition.active, definition.active ? definition.metadata[0] : 0, definition.metadata};
    return P4_TABLE_OK;
}
static int weapon_facts(const P4GameTables *tables, const P4GameItems *items,
    uint32_t item_id, uint32_t component, int runtime, P4GameActionElement *declared,
    const P4ActionElementBinding **binding)
{
    if (runtime) {
        int result = p4_game_tables_items_runtime_use(items, item_id);
        if (result) return result;
    }
    P4GameItemDefinition definition;
    int result = p4_game_tables_items_definition_find(items, item_id, &definition);
    if (result) return result;
    *binding = p4_game_tables_find_action_element(tables, P4_ACTION_ELEMENT_WEAPON, item_id, component);
    if (!*binding) {
        declared->element_id = definition.metadata->element;
        declared->valid = declared->element_id != UINT16_MAX;
    }
    return P4_TABLE_OK;
}
static int status_leaf(uint32_t payload, uint32_t *leaf)
{
    switch (payload) {
    case 0x2u: *leaf = 11; break;
    case 0x4u: *leaf = 10; break;
    case 0x8u: *leaf = 15; break;
    case 0x10u: *leaf = 8; break;
    case 0x1u: *leaf = 12; break;
    case 0x20u: *leaf = 9; break;
    case 0x40u: *leaf = 13; break;
    case 0x80u: *leaf = 14; break;
    case 0x100u:
    case 0x100000u: *leaf = 5; break;
    default: return P4_TABLE_INVALID;
    }
    return P4_TABLE_OK;
}
enum { APPLICABILITY_PRIMARY = 2 };
static int resolve_action(const P4GameTables *tables, const P4GameSkills *skills, const P4GameItems *items,
    void *unit, uint32_t skill_id, int primary, uint32_t component,
    uint32_t status_mask, P4GameActionElement *element, uint32_t native_options, P4GameActionFrame *frame, int runtime)
{
    if (!tables || (!element && !frame)) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    if (runtime) {
        int result = p4_game_skills_runtime_use(skills, skill_id);
        if (result) return result;
    }
    SkillElementFacts facts;
    int result = skill_facts(skills, skill_id, &facts);
    if (result) return result;
    if (!facts.active) return P4_TABLE_INVALID;
    if (primary == APPLICABILITY_PRIMARY) {
        /* The native predicate classifies the skill element, not a later
         * status payload, even for a pure KO action. */
        component = P4_ACTION_COMPONENT_DAMAGE;
    } else if (primary) {
        /* Select only the pre-hit role. Random payload choice stays at the
         * original later native call, and a mixed damage/KO skill has two frames. */
        status_mask = facts.metadata[0x24] == 13u ? 0x80000u :
            le32(facts.metadata + 0x1c) & 0xffffffu;
        component = facts.metadata[0x11] || facts.metadata[0x14] ? P4_ACTION_COMPONENT_DAMAGE :
            status_mask == 0x80000u ? P4_ACTION_COMPONENT_INSTANT_KO :
            status_mask ? P4_ACTION_COMPONENT_STATUS : P4_ACTION_COMPONENT_DAMAGE;
    }
    if (component > P4_ACTION_COMPONENT_STATUS) return P4_TABLE_NOT_READY;
    if (!primary) {
        if (component == P4_ACTION_COMPONENT_INSTANT_KO && !(status_mask & 0x80000u)) return P4_TABLE_INVALID;
        if (component == P4_ACTION_COMPONENT_STATUS &&
            (!status_mask || ((status_mask & 0x80000u) && !(facts.valid && facts.leaf == 6u))))
            return P4_TABLE_INVALID;
    }
    uint32_t kind = P4_ACTION_ELEMENT_SKILL, source = skill_id;
    const P4ActionElementBinding *binding = p4_game_tables_find_action_element(tables, kind, source, component);
    P4GameActionElement declared = {.valid = facts.valid, .element_id = facts.leaf, .component = component};
    if (!binding) {
        if (!primary && component != P4_ACTION_COMPONENT_DAMAGE) {
            declared.valid = 1;
            if (component == P4_ACTION_COMPONENT_INSTANT_KO) {
                declared.element_id = facts.valid && facts.leaf == 6u ? 6u : 7u;
                declared.channel_id = P4_CHANNEL_INSTANT_KO;
            } else {
                if (facts.valid && facts.leaf == 6u) declared.element_id = 6u;
                else {
                    result = status_leaf(status_mask, &declared.element_id);
                    if (result) return result;
                }
                declared.channel_id = P4_CHANNEL_STATUS;
            }
        } else {
            declared.channel_id = component == P4_ACTION_COMPONENT_INSTANT_KO ? P4_CHANNEL_INSTANT_KO :
                component == P4_ACTION_COMPONENT_STATUS ? P4_CHANNEL_STATUS : P4_CHANNEL_DAMAGE;
            /* Applicability permits a null source: retail then uses the skill's
             * own element even when the skill normally derives a weapon kind. */
            if ((facts.flags & 2u) && (unit || primary != APPLICABILITY_PRIMARY)) {
                if (!unit) return P4_TABLE_NOT_READY;
                const uint8_t *record = unit;
                const uint32_t id = le16(record + 2);
                if (le16(record) & 4u) {
                    kind = P4_ACTION_ELEMENT_ENEMY_ATTACK; source = id;
                    binding = p4_game_tables_find_action_element(tables, kind, source, component);
                    if (!binding) {
                        /* Only a known retail source may index the packed bank. */
                        if (id >= 336u) return P4_TABLE_NOT_FOUND;
                        const uint32_t metadata = P4_G32(0x007644b4u);
                        if (!metadata) return P4_TABLE_NOT_READY;
                        declared.valid = 1; declared.element_id = P4_G8(metadata + id * 60u + 0x38u);
                    }
                } else {
                    if (!id || id > 10u || !items || !p4_game_tables_items_revision(items)) return P4_TABLE_NOT_READY;
                    uint32_t item;
                    result = p4_game_tables_items_equipment_get(items, id, 0, &item);
                    if (result == P4_TABLE_NOT_FOUND) {
                        const int32_t retail = p4_game_tables_items_retail_equipment((int32_t)id, 0);
                        if (retail < 0) return P4_TABLE_NOT_FOUND;
                        item = (uint32_t)retail;
                    } else if (result) return result;
                    kind = P4_ACTION_ELEMENT_WEAPON; source = item;
                    result = weapon_facts(tables, items, source, component, runtime, &declared, &binding);
                    if (result) return result;
                }
            }
        }
    }
    if (binding) {
        if (runtime) {
            result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ACTION, binding);
            if (result) return result;
        }
        declared.valid = 1; declared.element_id = binding->element_id; declared.channel_id = binding->channel_id;
    }
    P4GameActionElement resolved;
    result = runtime ? p4_game_tables_classify_runtime_action_element(tables, &declared, frame ? &resolved : element) :
        p4_game_tables_classify_action_element(tables, &declared, frame ? &resolved : element);
    if (result) return result;
    if (frame) *frame = (P4GameActionFrame){.element = resolved, .skill_id = skill_id,
        .source_kind = kind, .source_id = source, .native_options = native_options, .owned_binding = binding != NULL};
    return P4_TABLE_OK;
}

int p4_game_elements_resolve_action(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items, void *unit, uint32_t skill_id,
    uint32_t component, uint32_t status_mask, P4GameActionElement *element)
{
    return resolve_action(tables, skills, items, unit, skill_id, 0, component, status_mask, element, 0, NULL, 0);
}
int p4_game_elements_resolve_primary_action(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items, void *unit, uint32_t skill_id, P4GameActionElement *element)
{
    return resolve_action(tables, skills, items, unit, skill_id, 1, 0, 0, element, 0, NULL, 0);
}
int p4_game_elements_resolve_action_frame(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items, void *unit, uint32_t skill_id,
    uint32_t component, uint32_t status_mask, uint32_t native_options, P4GameActionFrame *frame)
{
    if (!frame) return P4_TABLE_INVALID;
    return resolve_action(tables, skills, items, unit, skill_id, 0, component, status_mask, NULL, native_options, frame, 0);
}
int p4_game_elements_resolve_primary_frame(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items, void *unit, uint32_t skill_id, uint32_t native_options, P4GameActionFrame *frame)
{
    if (!frame) return P4_TABLE_INVALID;
    return resolve_action(tables, skills, items, unit, skill_id, 1, 0, 0, NULL, native_options, frame, 0);
}
int p4_game_elements_resolve_runtime_primary_frame(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items, void *unit,
    uint32_t skill_id, uint32_t native_options, P4GameActionFrame *frame)
{
    if (!frame) return P4_TABLE_INVALID;
    return resolve_action(tables, skills, items, unit, skill_id, 1, 0, 0, NULL, native_options, frame, 1);
}
int p4_game_elements_resolve_runtime_applicability_frame(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items, void *unit,
    uint32_t skill_id, P4GameActionFrame *frame)
{
    if (!frame) return P4_TABLE_INVALID;
    return resolve_action(tables, skills, items, unit, skill_id, APPLICABILITY_PRIMARY, 0, 0, NULL, 0, frame, 1);
}
int p4_game_elements_resolve_runtime_action_frame(const P4GameTables *tables,
    const P4GameSkills *skills, const P4GameItems *items, void *unit,
    uint32_t skill_id, uint32_t component, uint32_t status_mask,
    uint32_t native_options, P4GameActionFrame *frame)
{
    if (!frame) return P4_TABLE_INVALID;
    return resolve_action(tables, skills, items, unit, skill_id, 0, component, status_mask, NULL, native_options, frame, 1);
}
static int resolve_weapon(const P4GameTables *tables, const P4GameItems *items, uint32_t item_id,
    uint32_t component, P4GameActionElement *element, int runtime)
{
    if (!tables || !element) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    if (component > P4_ACTION_COMPONENT_STATUS) return P4_TABLE_NOT_READY;
    P4GameActionElement declared = {.component = component,
        .channel_id = component == P4_ACTION_COMPONENT_INSTANT_KO ? P4_CHANNEL_INSTANT_KO :
            component == P4_ACTION_COMPONENT_STATUS ? P4_CHANNEL_STATUS : P4_CHANNEL_DAMAGE};
    const P4ActionElementBinding *binding;
    int result = weapon_facts(tables, items, item_id, component, runtime, &declared, &binding);
    if (result) return result;
    if (binding) {
        if (runtime) {
            result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ACTION, binding);
            if (result) return result;
        }
        declared.valid = 1; declared.element_id = binding->element_id; declared.channel_id = binding->channel_id;
    }
    return runtime ? p4_game_tables_classify_runtime_action_element(tables, &declared, element) :
        p4_game_tables_classify_action_element(tables, &declared, element);
}
int p4_game_elements_resolve_weapon(const P4GameTables *tables, const P4GameItems *items,
    uint32_t item_id, uint32_t component, P4GameActionElement *element)
{ return resolve_weapon(tables, items, item_id, component, element, 0); }
int p4_game_elements_resolve_runtime_weapon(const P4GameTables *tables, const P4GameItems *items,
    uint32_t item_id, uint32_t component, P4GameActionElement *element)
{ return resolve_weapon(tables, items, item_id, component, element, 1); }
