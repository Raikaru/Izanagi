/* Real Core/VFS skill inputs, item/skill/text compilers and action resolver.
 * Only the original retail equipment expression and runtime source dispatch
 * are probes. This is not battle/menu/save proof or native leaf admission. */
#include "p4pc/game_elements.h"
#include "p4pc/game_items.h"
#include "p4pc/game_table_sources.h"
#include "element_skill_inputs.h"
#include "../src/platform/mods/game_elements_action_internal.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* The published enemy-bank limit; retail until a fixture expands it. */
uint32_t p4_game_enemies_native_limit(void) { return 336; }

static int16_t equipped = 9;
static unsigned retail_equipment_reads;
static P4TestElementSkillInputs inputs;
static const P4GameItems *probe_items;
static unsigned source_calls[3], sequence[16], sequence_count;
static int skill_source_result, item_source_result;
static unsigned registry_uses;
static int registry_source_result;
static int registry_use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(view == inputs.tables && context == &inputs && ids);
    assert(domain == P4_TABLE_SOURCE_ELEMENT || domain == P4_TABLE_SOURCE_ACTION);
    if (domain == P4_TABLE_SOURCE_ELEMENT) assert(ids[0] == 65535u && !ids[1] && !ids[2] && !ids[3] && !ids[4]);
    else assert(ids[0] == P4_ACTION_ELEMENT_WEAPON && ids[1] == 1536 && !ids[2] && !ids[3] && !ids[4]);
    ++registry_uses;
    return registry_source_result; /* Dispatch only, never origin authority. */
}
int32_t p4_game_tables_items_retail_equipment(int32_t party, int32_t slot)
{
    assert(party == 1 && slot == 0 && equipped >= 9 && equipped <= 11);
    ++retail_equipment_reads;
    return equipped;
}
static int original_use(const void *sealed_span_identity, void *actual_input)
{
    const P4GameTableInputSpan *span = sealed_span_identity;
    assert(actual_input == inputs.skill && p4_game_table_input_span_owner(span) == actual_input);
    assert(span == p4_game_table_input_span(inputs.skill, 1));
    ++source_calls[0]; sequence[sequence_count++] = 1;
    return skill_source_result;
}
static int authored_use(const P4GameSkills *view, uint32_t id, void *context)
{
    assert(view == inputs.skills && context == &inputs && id >= 576 && id < 581);
    ++source_calls[1]; sequence[sequence_count++] = 2;
    return skill_source_result;
}
static int item_use(const P4GameItems *view, uint32_t table, uint32_t id, void *context)
{
    assert(view == probe_items && context == &inputs && table == P4_ITEM_USE_ROW && id == 1536);
    ++source_calls[2]; sequence[sequence_count++] = 3;
    return item_source_result;
}
static int text_use(const P4GameText *view, uint32_t table, uint32_t id, uint32_t role, void *context)
{
    (void)view; (void)table; (void)id; (void)role; (void)context;
    assert(!"action frame must not consume name/description bytes");
    return P4_TABLE_INVALID;
}
static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user; void *host = p4t_galloc(size, 16);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user; assert(size && guest == P4_H2G(host));
}
static void put16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value; bytes[1] = (uint8_t)(value >> 8);
}
static void bind(P4GameTables *tables, uint32_t kind, uint32_t source, uint32_t component,
    uint32_t leaf, uint32_t channel, const char *key)
{
    P4ActionElementBinding row = {kind, source, component, leaf, channel, 1, 1, ""};
    strcpy(row.key, key);
    assert(p4_game_tables_action_element(tables, "test", &row) == P4_TABLE_OK);
}
static void text(P4GameText *snapshot, uint32_t table, uint32_t id, uint32_t role)
{
    const char *bytes = role == P4_TEXT_NAME ? "Synthetic action" : "Synthetic action description";
    assert(p4_game_tables_text_register(snapshot, "test", table, id, role, 1,
        (const uint8_t *)bytes, strlen(bytes) + 1) == P4_TABLE_OK);
}
int main(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000u)) return 77;
    uint8_t *metadata = p4t_galloc(P4_SKILL_RETAIL_ACTIVE_COUNT * P4_SKILL_ROW_SIZE, 16);
    uint8_t *elements = p4t_galloc(P4_SKILL_RETAIL_COUNT * P4_SKILL_ELEMENT_SIZE, 16);
    uint8_t *enemies = p4t_galloc(336u * 60u, 16), *unit = p4t_galloc(0x30u, 16);
    uint8_t *retail_items = p4t_galloc(P4_ITEM_RETAIL_FILE_SIZE, 16);
    assert(metadata && elements && enemies && unit && retail_items);
    put16(retail_items, P4_ITEM_RETAIL_COUNT);
    put16(retail_items + P4_ITEM_FILE_HEADER_SIZE + 10u * P4_ITEM_ROW_SIZE + 0x2c, 1);
    put16(retail_items + P4_ITEM_FILE_HEADER_SIZE + 11u * P4_ITEM_ROW_SIZE + 0x2c, UINT16_MAX);
    enemies[6u * 60u + 0x38u] = 1;
    P4_G32(0x007644a4u) = P4_H2G(elements); P4_G32(0x007644a8u) = P4_H2G(metadata);
    P4_G32(0x007644b4u) = P4_H2G(enemies);

    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    assert(!p4_game_tables_bind_runtime_use(tables, registry_use, &inputs));
    const uint32_t leaf_ids[] = {19u, 20u, 65535u};
    for (size_t i = 0; i < sizeof leaf_ids / sizeof leaf_ids[0]; ++i) {
        P4ElementDesc row = {.id = leaf_ids[i], .family = 0, .order = (uint32_t)i,
            .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
            .revision = 1, .display_name = "Physical leaf", .short_name = "Leaf"};
        snprintf(row.key, sizeof row.key, "test:leaf-%u", (unsigned)i);
        assert(p4_game_tables_element(tables, "test", &row) == P4_TABLE_OK);
    }
    bind(tables, P4_ACTION_ELEMENT_SKILL, 120, P4_ACTION_COMPONENT_DAMAGE, 19, P4_CHANNEL_DAMAGE, "test:bash");
    bind(tables, P4_ACTION_ELEMENT_WEAPON, 9, P4_ACTION_COMPONENT_DAMAGE, 20, P4_CHANNEL_DAMAGE, "test:weapon");
    bind(tables, P4_ACTION_ELEMENT_WEAPON, 1536, P4_ACTION_COMPONENT_DAMAGE, 65535u, P4_CHANNEL_DAMAGE, "test:new-weapon");
    bind(tables, P4_ACTION_ELEMENT_ENEMY_ATTACK, 5, P4_ACTION_COMPONENT_DAMAGE, 20, P4_CHANNEL_DAMAGE, "test:enemy");
    bind(tables, P4_ACTION_ELEMENT_SKILL, 577, P4_ACTION_COMPONENT_INSTANT_KO, 6, P4_CHANNEL_INSTANT_KO, "test:mixed-ko");
    bind(tables, P4_ACTION_ELEMENT_SKILL, 577, P4_ACTION_COMPONENT_STATUS, 9, P4_CHANNEL_STATUS, "test:mixed-poison");
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    P4GameTableAllocator allocator = {NULL, allocate, release};
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4TextProfile message = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *names = p4_game_tables_text_create(&allocator, &raw);
    P4GameText *descriptions = p4_game_tables_text_create(&allocator, &message);
    P4GameSkills *skills = p4_game_skills_create(&allocator); assert(names && descriptions && skills);
    assert(!p4_game_tables_text_bind_runtime_use(names, text_use, &inputs));
    assert(!p4_game_tables_text_bind_runtime_use(descriptions, text_use, &inputs));
    P4GameItems *items = p4_game_tables_items_create(&allocator); assert(items);
    probe_items = items;
    assert(!p4_game_skills_bind_runtime_use(skills, original_use, authored_use, &inputs));
    assert(!p4_game_tables_items_bind_runtime_use(items, item_use, &inputs));
    const P4GameItemDesc new_weapon = {.id = 1536, .revision = 1, .key = "test:weapon",
        .icon = {.owner = "test", .key = "test:weapon-icon", .revision = 1},
        .metadata = {.element = UINT16_MAX, .category_mask = 1u << 5}};
    assert(!p4_game_tables_items_register(items, "test", &new_weapon));
    assert(!p4_game_tables_items_models_register(items, "test", 1536, 0, P4_ITEM_MODEL_NONE));
    text(names, P4_TEXT_ITEM, 1536, P4_TEXT_NAME); text(descriptions, P4_TEXT_ITEM, 1536, P4_TEXT_DESCRIPTION);
    for (uint32_t id = 576; id < 581; ++id) {
        P4GameSkillDesc row = {.id = id, .active = id != 578, .element_id = id == 577 ? 65535u : id == 580 ? 16u : 6u,
            .schema = 1, .revision = 1, .inheritance_group = id == 577 || id == 580 ? 0 : 6,
            .delivery = id == 576 ? P4_DELIVERY_PHYSICAL : id == 579 ? P4_DELIVERY_MAGIC : P4_DELIVERY_DEFAULT};
        snprintf(row.key, sizeof row.key, "test:action-%u", (unsigned)id);
        row.metadata[2] = id == 576 ? 1 : 2; /* A delivery names its row's own damage class. */
        if (id == 576 || id == 577) row.metadata[0x11] = 1; /* HP damage, independent of leaf. */
        if (id == 577 || id == 579 || id == 580) row.metadata[0x1e] = 8; /* Mixed rider or pure KO. */
        assert(p4_game_skills_register(skills, "test", &row) == P4_TABLE_OK);
        text(names, P4_TEXT_SKILL, id, P4_TEXT_NAME); text(descriptions, P4_TEXT_SKILL, id, P4_TEXT_DESCRIPTION);
    }
    assert(!p4_game_tables_text_seal(names, 1) && !p4_game_tables_text_seal(descriptions, 1));
    inputs.skills = skills; inputs.tables = tables; inputs.names = names; inputs.descriptions = descriptions;
    p4_test_element_inputs_begin(&inputs);
    assert(!p4_game_tables_items_seal(items, retail_items, P4_ITEM_RETAIL_FILE_SIZE, names, descriptions, 1));
    P4GameActionElement prepared;
    assert(!p4_game_elements_resolve_primary_action(tables, skills, items, NULL, 577, &prepared));
    assert(prepared.element_id == 65535u && !p4_game_skills_revision(skills) && !p4_game_tables_items_revision(items));
    assert(!p4_game_elements_resolve_weapon(tables, items, 10, P4_ACTION_COMPONENT_DAMAGE, &prepared));
    assert(prepared.element_id == 1 && !sequence_count);
    assert(!p4_game_tables_text_materialize(names) && !p4_game_tables_text_materialize(descriptions));
    p4_test_element_inputs_materialize(&inputs);
    assert(!p4_game_tables_items_materialize(items));
    assert(!p4_game_skills_native_admit(skills)); /* Wide leaf 65535 admitted; activation is cohort-gated. */
    assert(P4_G8(p4_game_skills_elements(skills) + 577u * 2u) == P4_SKILL_LEGACY_ELEMENT_INVALID);
    assert(P4_G32(0x007644a4u) == P4_H2G(elements)); /* No new bank publication. */

    P4GameActionElement result;
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 120, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK);
    assert(result.valid && result.element_id == 19 && p4_game_action_element_is_physical(&result));
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_NOT_READY);
    put16(unit + 2, 1);
    assert(p4_game_elements_resolve_action(tables, skills, items, unit, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK);
    assert(result.element_id == 20 && retail_equipment_reads == 1);
    equipped = 10;
    assert(p4_game_elements_resolve_action(tables, skills, items, unit, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK);
    assert(result.element_id == 1 && !p4_game_action_element_is_physical(&result) && retail_equipment_reads == 2);
    equipped = 11;
    assert(p4_game_elements_resolve_action(tables, skills, items, unit, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK && !result.valid);
    assert(!p4_game_tables_items_equipment_set(items, 1, 0, 1536));
    assert(p4_game_elements_resolve_action(tables, skills, items, unit, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK);
    assert(result.valid && result.element_id == 65535u && retail_equipment_reads == 3);
    put16(unit, 4); put16(unit + 2, 5); put16(unit + 4, 6);
    assert(p4_game_elements_resolve_action(tables, skills, items, unit, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK && result.element_id == 20);
    put16(unit + 2, 6); put16(unit + 4, 5);
    assert(p4_game_elements_resolve_action(tables, skills, items, unit, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK && result.element_id == 1);
    put16(unit + 2, 336);
    assert(p4_game_elements_resolve_action(tables, skills, items, unit, 0, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_NOT_FOUND);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 577, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK);
    assert(result.valid && result.element_id == 65535u && result.channel_id == P4_CHANNEL_DAMAGE);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 577, P4_ACTION_COMPONENT_INSTANT_KO, 0x80000u, &result) == P4_TABLE_OK);
    assert(result.element_id == 6 && result.channel_id == P4_CHANNEL_INSTANT_KO);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 577, P4_ACTION_COMPONENT_STATUS, 0x20u, &result) == P4_TABLE_OK);
    assert(result.element_id == 9 && result.channel_id == P4_CHANNEL_STATUS);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 11, P4_ACTION_COMPONENT_STATUS, 0x20u, &result) == P4_TABLE_OK && result.element_id == 6);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 11, P4_ACTION_COMPONENT_STATUS, 0x80020u, &result) == P4_TABLE_OK && result.element_id == 6);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 11, P4_ACTION_COMPONENT_INSTANT_KO, 0x80000u, &result) == P4_TABLE_OK && result.element_id == 6);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 577, P4_ACTION_COMPONENT_INSTANT_KO, 0, &result) == P4_TABLE_INVALID);
    /* No skill (skill -1): 001F6930's recovery chance maps the payload alone,
     * as datCalcStatusElement does, without a skill definition. */
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 0xffff, P4_ACTION_COMPONENT_STATUS, 0x4u, &result) == P4_TABLE_OK);
    assert(result.valid && result.element_id == 10 && result.channel_id == P4_CHANNEL_STATUS);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 0xffff, P4_ACTION_COMPONENT_INSTANT_KO, 0x80000u, &result) == P4_TABLE_OK);
    assert(result.element_id == 7 && result.channel_id == P4_CHANNEL_INSTANT_KO);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 0xffff, P4_ACTION_COMPONENT_STATUS, 0x6u, &result) == P4_TABLE_INVALID);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 0xffff, P4_ACTION_COMPONENT_INSTANT_KO, 0x80004u, &result) == P4_TABLE_INVALID);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 0xffff, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_INVALID);
    {
        P4GameActionFrame recovery;
        assert(p4_game_elements_resolve_action_frame(tables, skills, items, NULL, 0xffff, P4_ACTION_COMPONENT_STATUS,
            0x8u, 0, &recovery) == P4_TABLE_OK);
        assert(recovery.skill_id == 0xffff && recovery.source_kind == P4_ACTION_ELEMENT_SKILL && !recovery.owned_binding);
        assert(recovery.element.element_id == 15 && recovery.element.channel_id == P4_CHANNEL_STATUS);
    }
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 578, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_INVALID);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 12, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_OK && !result.valid);
    assert(p4_game_elements_resolve_action(tables, skills, items, NULL, 13, P4_ACTION_COMPONENT_DAMAGE, 0, &result) == P4_TABLE_INVALID);
    assert(p4_game_elements_resolve_primary_action(tables, skills, items, NULL, 576, &result) == P4_TABLE_OK);
    assert(result.element_id == 6 && result.component == P4_ACTION_COMPONENT_DAMAGE && result.channel_id == P4_CHANNEL_DAMAGE);
    {
        /* An authored physical delivery marks the damage frame only; the
         * element (and its affinity) stays leaf 6. */
        P4GameActionFrame delivered = {.element = result};
        assert(result.flags & P4_ELEMENT_DELIVERY_PHYSICAL && !p4_game_action_element_is_physical(&result) &&
            p4_game_action_frame_physical(&delivered));
        delivered.element.flags &= ~P4_ELEMENT_DELIVERY_PHYSICAL;
        assert(!p4_game_action_frame_physical(&delivered));
        delivered.element.flags |= P4_ELEMENT_PHYSICAL | P4_ELEMENT_DELIVERY_MAGIC;
        assert(!p4_game_action_frame_physical(&delivered));
        delivered.element.flags &= ~P4_ELEMENT_DELIVERY_MAGIC;
        assert(p4_game_action_frame_physical(&delivered));
    }
    assert(p4_game_elements_resolve_primary_action(tables, skills, items, NULL, 577, &result) == P4_TABLE_OK);
    assert(result.element_id == 65535u && result.component == P4_ACTION_COMPONENT_DAMAGE && result.channel_id == P4_CHANNEL_DAMAGE);
    assert(p4_game_elements_resolve_primary_action(tables, skills, items, NULL, 579, &result) == P4_TABLE_OK);
    assert(result.element_id == 6 && result.component == P4_ACTION_COMPONENT_INSTANT_KO && result.channel_id == P4_CHANNEL_INSTANT_KO);
    /* A delivery marks the damage component only, never a KO or status payload. */
    assert(!(result.flags & (P4_ELEMENT_DELIVERY_PHYSICAL | P4_ELEMENT_DELIVERY_MAGIC)));
    assert(p4_game_elements_resolve_primary_action(tables, skills, items, NULL, 11, &result) == P4_TABLE_OK);
    assert(result.element_id == 6 && result.channel_id == P4_CHANNEL_INSTANT_KO);
    assert(p4_game_elements_resolve_primary_action(tables, skills, items, NULL, 578, &result) == P4_TABLE_INVALID);
    P4GameActionFrame damage, rider, weapon, enemy;
    assert(p4_game_elements_resolve_primary_frame(tables, skills, items, NULL, 120, UINT32_MAX, &damage) == P4_TABLE_OK);
    assert(damage.skill_id == 120 && damage.source_kind == P4_ACTION_ELEMENT_SKILL && damage.source_id == 120);
    assert(damage.owned_binding && damage.native_options == UINT32_MAX && damage.element.element_id == 19);
    assert(p4_game_elements_resolve_primary_frame(tables, skills, items, NULL, 577, 2, &damage) == P4_TABLE_OK);
    assert(!damage.owned_binding && damage.source_id == 577 && damage.native_options == 2);
    assert(damage.element.element_id == 65535u && damage.element.component == P4_ACTION_COMPONENT_DAMAGE);
    const P4GameActionFrame saved_damage = damage;
    assert(p4_game_elements_resolve_action_frame(tables, skills, items, NULL, 577, P4_ACTION_COMPONENT_INSTANT_KO, 0x80000u, 0x80000000u, &rider) == P4_TABLE_OK);
    assert(rider.owned_binding && rider.element.element_id == 6 && rider.element.component == P4_ACTION_COMPONENT_INSTANT_KO);
    assert(rider.native_options == 0x80000000u && !memcmp(&damage, &saved_damage, sizeof damage));
    put16(unit, 0); put16(unit + 2, 1);
    const unsigned calls_before = retail_equipment_reads;
    assert(p4_game_elements_resolve_primary_frame(tables, skills, items, unit, 0, 0x55aa55aau, &weapon) == P4_TABLE_OK);
    assert(weapon.source_kind == P4_ACTION_ELEMENT_WEAPON && weapon.source_id == 1536 && weapon.skill_id == 0);
    assert(weapon.owned_binding && weapon.element.element_id == 65535u && weapon.native_options == 0x55aa55aau);
    assert(retail_equipment_reads == calls_before);
    put16(unit, 4); put16(unit + 2, 5); put16(unit + 4, 6);
    assert(p4_game_elements_resolve_primary_frame(tables, skills, items, unit, 0, 4, &enemy) == P4_TABLE_OK);
    assert(enemy.source_kind == P4_ACTION_ELEMENT_ENEMY_ATTACK && enemy.source_id == 5 && enemy.owned_binding);
    assert(enemy.element.element_id == 20 && !memcmp(&weapon.element, &damage.element, sizeof weapon.element));
    const P4GameActionFrame saved_enemy = enemy;
    assert(p4_game_elements_resolve_primary_frame(tables, skills, items, NULL, 0, 0, &enemy) == P4_TABLE_NOT_READY);
    assert(!memcmp(&enemy, &saved_enemy, sizeof enemy));
    assert(p4_game_elements_resolve_action_frame(tables, skills, items, NULL, 120, UINT32_MAX, 0, 0, &enemy) == P4_TABLE_NOT_READY);
    assert(!memcmp(&enemy, &saved_enemy, sizeof enemy));
    assert(p4_game_elements_resolve_primary_frame(tables, skills, items, unit, 0, 0, NULL) == P4_TABLE_INVALID);
    assert(p4_game_elements_resolve_weapon(tables, items, 1536, P4_ACTION_COMPONENT_DAMAGE, &result) == P4_TABLE_OK);
    assert(!memcmp(&result, &weapon.element, sizeof result));
    assert(p4_game_elements_resolve_weapon(tables, items, 10, P4_ACTION_COMPONENT_DAMAGE, &result) == P4_TABLE_OK);
    assert(result.valid && result.element_id == 1);
    assert(p4_game_elements_resolve_weapon(tables, items, 11, P4_ACTION_COMPONENT_DAMAGE, &result) == P4_TABLE_OK && !result.valid);
    const P4GameActionElement saved_weapon = result;
    assert(p4_game_elements_resolve_weapon(tables, items, 1536, UINT32_MAX, &result) == P4_TABLE_NOT_READY);
    assert(!memcmp(&result, &saved_weapon, sizeof result) && !sequence_count);
    assert(!registry_uses); /* All definition/preparation/preview calls were pure. */
    put16(unit, 0); put16(unit + 2, 1);
    assert(!p4_game_elements_resolve_runtime_primary_frame(tables, skills, items, unit, 0, 7, &weapon));
    assert(sequence_count == 2 && sequence[0] == 1 && sequence[1] == 3);
    assert(source_calls[0] == 1 && source_calls[1] == 0 && source_calls[2] == 1);
    assert(!p4_game_elements_resolve_runtime_primary_frame(tables, skills, items, NULL, 577, 9, &damage));
    assert(sequence_count == 3 && sequence[2] == 2 && source_calls[1] == 1);
    assert(!p4_game_elements_resolve_runtime_weapon(tables, items, 1536, P4_ACTION_COMPONENT_DAMAGE, &result));
    assert(sequence_count == 4 && sequence[3] == 3 && source_calls[2] == 2);
    assert(registry_uses == 5);
    const P4GameActionFrame preserved = weapon;
    skill_source_result = 1;
    assert(p4_game_elements_resolve_runtime_primary_frame(tables, skills, items, unit, 0, 0, &weapon) == P4_TABLE_INVALID);
    assert(sequence_count == 5 && sequence[4] == 1 && source_calls[2] == 2);
    assert(!memcmp(&weapon, &preserved, sizeof weapon));
    skill_source_result = 0; item_source_result = 1;
    assert(p4_game_elements_resolve_runtime_primary_frame(tables, skills, items, unit, 0, 0, &weapon) == P4_TABLE_INVALID);
    assert(sequence_count == 7 && sequence[5] == 1 && sequence[6] == 3);
    assert(!memcmp(&weapon, &preserved, sizeof weapon));
    assert(registry_uses == 5);
    item_source_result = 0; registry_source_result = 1;
    const P4GameActionElement before_registry_failure = result;
    assert(p4_game_elements_resolve_runtime_weapon(tables, items, 1536, P4_ACTION_COMPONENT_DAMAGE, &result) == P4_TABLE_INVALID);
    assert(registry_uses == 6 && !memcmp(&result, &before_registry_failure, sizeof result));
    registry_source_result = 0; sequence_count = 0;
    const unsigned equipment_before_actorless = retail_equipment_reads;
    assert(!p4_game_elements_resolve_runtime_applicability_frame(tables, skills, items, NULL, 0, &damage));
    assert(damage.skill_id == 0 && damage.source_kind == P4_ACTION_ELEMENT_SKILL &&
        damage.element.element_id == P4_G8(p4_game_skills_elements(skills)));
    assert(sequence_count == 1 && sequence[0] == 1 && retail_equipment_reads == equipment_before_actorless);
    sequence_count = 0;
    assert(!p4_game_elements_resolve_runtime_applicability_frame(tables, skills, items, NULL, 580, &damage));
    assert(damage.element.element_id == 16 && damage.element.component == P4_ACTION_COMPONENT_DAMAGE &&
        !p4_game_action_element_has_affinity(&damage.element) && sequence_count == 1 && sequence[0] == 2);
    p4_game_skills_destroy(skills); p4_game_tables_items_destroy(items);
    p4_game_tables_text_destroy(names); p4_game_tables_text_destroy(descriptions); p4_game_tables_destroy(tables);
    p4_package_registry_retire(); p4_test_element_inputs_release(&inputs);
    puts("game_elements_action: genuine sealed definitions, wide frames, pure zero-touch and ordered runtime source dispatch passed (probes; not native activation)");
    return 0;
}
