/* The real shared native precedence body, with scripted native-query facts.
 * This is an in-memory branch/order witness, not a battle or save qualification. */
#include "p4pc/game_elements.h"
#include "p4pc/game_tables_native.h"
#include "p4pc/gmem.h"
#include "p4pc/package_boot.h"
#include "p4pc/game_table_sources.h"
#include "p4pc/game_damage_cover.h"
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_passive_grant.h"
#include "element_skill_inputs.h"
#include "guest_test.h"
#include "no_resource_work.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static P4TestElementSkillInputs inputs;
static unsigned original_skill_uses;
static unsigned registry_uses, registry_domains[7];
static int registry_source_result;
static int registry_use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(context == view && ids && domain >= P4_TABLE_SOURCE_ELEMENT && domain <= P4_TABLE_SOURCE_EFFECT);
    assert(domain != P4_TABLE_SOURCE_CHANNEL); /* Only explicit unowned builtin channels here. */
    ++registry_uses; ++registry_domains[domain];
    return registry_source_result; /* Explicit dispatch probe, not provenance. */
}
static int original_skill_use(const void *sealed_span_identity, void *actual_input)
{
    const P4GameTableInputSpan *span = sealed_span_identity;
    assert(actual_input == inputs.skill && p4_game_table_input_span_owner(span) == actual_input);
    assert(span == p4_game_table_input_span(inputs.skill, 1));
    ++original_skill_uses;
    return P4_TABLE_OK; /* Explicit dispatch probe, not an origin attestation. */
}
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{
    fprintf(stderr, "unexpected intercepted native refusal: %s\n", reason);
    abort();
}
/* Enemy- and persona-family facts: retail banks published; the staged
 * snapshots may declare added enemies and personas that the element coverage
 * must answer. */
static uint32_t declared_enemies = 336, declared_personas = 256;
static uint32_t enemy_limit = 336;
uint32_t p4_game_enemies_native_limit(void) { return enemy_limit; }
uint32_t p4_game_enemies_native_declared_limit(void) { return declared_enemies; }
uint32_t p4_game_personas_native_declared_limit(void) { return declared_personas; }
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef struct DatUnit {
    uint8_t native[0x30], learned[16];
    uint8_t skills[0x240], properties[256];
    int8_t buffs[24];
    int guarding;
} DatUnit;
typedef struct Query { uint32_t kind, id; } Query;
static Query queries[32];
static unsigned query_count;
static void query(uint32_t kind, uint32_t id)
{
    assert(query_count < sizeof queries / sizeof queries[0]);
    queries[query_count++] = (Query){kind, id};
}
static int32_t datCalcIsGuarding(u16 *unit)
{
    query(0, 0); return ((DatUnit *)(void *)unit)->guarding;
}
static int32_t datCalcHasRequestedSkill(DatUnit *unit, int32_t skill)
{
    assert(skill >= 0 && (uint32_t)skill < sizeof unit->skills);
    query(1, (uint32_t)skill); return unit->skills[skill];
}
static int32_t datCalcHasEquipmentProperty(u8 *unit, int32_t property)
{
    assert(property >= 0 && property < 256);
    query(2, (uint32_t)property); return ((DatUnit *)(void *)unit)->properties[property];
}
static int32_t datCalcGetBuffLevel(DatUnit *unit, int32_t buff)
{
    assert(buff >= 0 && buff < 24);
    query(3, (uint32_t)buff); return unit->buffs[buff];
}
#include "p4pc/game_elements_affinity_body.inc"
static uint16_t selected_persona;
static int16_t equipment[11][3];
static uint8_t item_metadata[16][68];
uint16_t datPersonaGetRecordId(uint16_t party)
{
    assert(party > 0 && party < 11); return selected_persona;
}
uint32_t datCalcGetTableLength(void *unit) { assert(unit); return 8; }
/* The skill classes the passive grant table admits from (game_skills_native.c):
 * the retail split, and the added passives 700 and 701 the cover rows name. */
int p4_game_skill_registered(uint32_t id) { return id < 576u || id == 700u || id == 701u; }
int p4_game_skill_active(uint32_t id) { return id < 440u; }
/* Damage cover reads a party member's list by id, as datCalcSkillList does. */
static uint16_t pc_skills[11][8];
uint16_t *datPersonaGetSkillsByPcId(uint16_t pc_id) { assert(pc_id > 0 && pc_id < 11); return pc_skills[pc_id]; }
uint8_t *datCalcSkillList(void *unit) { return ((DatUnit *)unit)->learned; }
int16_t datGetEquippedItemByUnit(int16_t party, int16_t slot)
{
    assert(party > 0 && party < 11 && slot >= 0 && slot < 3); return equipment[party][slot];
}
int32_t p4_game_tables_items_retail_equipment(int32_t party, int32_t slot)
{ return datGetEquippedItemByUnit((int16_t)party, (int16_t)slot); }
const uint8_t *p4_game_tables_items_native_metadata(uint32_t id)
{
    assert(id < 16); return item_metadata[id];
}
static void put16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value; bytes[1] = (uint8_t)(value >> 8);
}
/* Scripted original entry results witness which native ABI is called; they
 * are not a claim that the real game's wrappers or UI have been qualified. */
static unsigned legacy_calls, legacy_wrapper_calls;
static const uint32_t legacy_word = P4_AFF_WEAK | 125u;
/* Side-ailment defaults read several retail leaves: one word per leaf. */
static int legacy_by_leaf;
static uint32_t legacy_leaf_words[19];
int32_t datCalcGetAffinity(uint8_t *unit, int32_t leaf)
{
    assert(unit && leaf >= 0 && leaf < 19); ++legacy_calls;
    return (int32_t)(legacy_by_leaf ? legacy_leaf_words[leaf] : legacy_word);
}
int32_t datCalcGetElementAffinity(uint8_t *unit, int32_t leaf)
{
    assert(unit && leaf >= 0 && leaf < 19); ++legacy_wrapper_calls; return (int32_t)legacy_word;
}

static uint32_t expected(uint32_t affinity, unsigned passives, int guarding, int clear, int grant)
{
    /* Literal ordered final-stage masks in the retail body. */
    if (guarding && (affinity & 0x08000000u)) affinity = (affinity & 0xf7ff0000u) | 100u;
    if ((passives & 1u) && !(affinity & 0x07000000u))
        affinity = (((affinity & 0xf7ffffffu) | 0x10000000u) & 0xffff0000u) | 50u;
    if ((passives & 2u) && !(affinity & 0x06000000u))
        affinity = (((affinity & 0xe7ffffffu) | 0x01000000u) & 0xffff0000u) | 100u;
    if ((passives & 4u) && !(affinity & 0x04000000u))
        affinity = (((affinity & 0xe6ffffffu) | 0x02000000u) & 0xffff0000u) | 100u;
    if (passives & 8u)
        affinity = (((affinity & 0xe4ffffffu) | 0x04000000u) & 0xffff0000u) | 100u;
    if (clear && (affinity & 0x17000000u)) affinity = (affinity & 0xe8ff0000u) | 100u;
    if (grant && ((affinity & 0x08000000u) || !(affinity & 0xff000000u)))
        affinity = (((affinity & 0x00ffffffu) | 0x10000000u) & 0xffff0000u) | 50u;
    return affinity;
}
static void precedence(void)
{
    static const uint32_t bases[] = {0u, 100u, 65535u, P4_AFF_WEAK | 125u,
        P4_AFF_RESIST | 50u, P4_AFF_NULL | 100u, P4_AFF_REPEL | 100u,
        P4_AFF_DRAIN | 100u, 0x20000000u | 100u, P4_AFF_WEAK | P4_AFF_DRAIN | 125u};
    for (unsigned selector = 0; selector < 5; ++selector)
        for (unsigned mask = 0; mask < 16; ++mask)
            for (unsigned guard = 0; guard < 2; ++guard)
                for (unsigned buffs = 0; buffs < 4; ++buffs) {
                    DatUnit unit = {0}; unit.guarding = (int)guard;
                    for (unsigned i = 0; i < 4; ++i) unit.skills[0x1b8 + selector * 4 + i] = (mask >> i) & 1u;
                    /* Break swaps Elec/Wind; Wall independently stays12..15. */
                    unsigned break_index = selector == 4 ? 10 : selector == 3 ? 11 : 7 + selector;
                    unsigned wall_index = 11 + selector;
                    if (selector) {
                        unit.buffs[break_index] = (buffs & 1u) ? 1 : -1;
                        unit.buffs[wall_index] = (buffs & 2u) ? 1 : -1;
                    }
                    for (size_t b = 0; b < sizeof bases / sizeof bases[0]; ++b) {
                        query_count = 0;
                        uint32_t result = p4_game_elements_native_affinity_apply(&unit, (int32_t)selector, bases[b], 0);
                        uint32_t wanted = expected(bases[b], mask, (int)guard, selector && (buffs & 1u), selector && (buffs & 2u));
                        if (result != wanted)
                            fprintf(stderr, "selector=%u passives=%u guard=%u buffs=%u base=%08x actual=%08x expected=%08x\n",
                                selector, mask, guard, buffs, bases[b], result, wanted);
                        assert(result == wanted);
                    }
                }
}
static void query_order_and_specials(void)
{
    DatUnit unit = {0};
    static const Query physical[] = {{0,0}, {1,0x1b8}, {1,0x1b9}, {1,0x1ba}, {1,0x1bb}, {1,0x1db}};
    query_count = 0;
    assert(p4_game_elements_native_affinity_apply(&unit, 0, P4_AFF_WEAK | 125u, 0) == (P4_AFF_WEAK | 125u));
    assert(query_count == sizeof physical / sizeof physical[0] && !memcmp(queries, physical, sizeof physical));
    unit.skills[0x1db] = 1;
    query_count = 0;
    assert(p4_game_elements_native_affinity_apply(&unit, 5, 100, 0) == 100); /* Almighty excludes Masakados. */
    assert(query_count == 1 && queries[0].kind == 0);
    query_count = 0;
    assert(p4_game_elements_native_affinity_apply(&unit, 1, P4_AFF_WEAK | 125u, 0) == (P4_AFF_NULL | 100u));
    memset(&unit, 0, sizeof unit); unit.properties[0x4c] = 1;
    query_count = 0;
    assert(p4_game_elements_native_affinity_apply(&unit, 9, 100, 0) == (P4_AFF_RESIST | 50u));
    unit.skills[0x1d7] = 1;
    query_count = 0;
    assert(p4_game_elements_native_affinity_apply(&unit, 9, 100, 0) == (P4_AFF_NULL | 100u));
    memset(&unit, 0, sizeof unit); unit.skills[0x1cc] = unit.skills[0x1cd] = unit.skills[0x1ce] = 1;
    query_count = 0;
    assert(p4_game_elements_native_affinity_apply(&unit, 6, P4_AFF_WEAK | 125u, 0) == (P4_AFF_REPEL | 100u));
}
static void additional_effects(void)
{
    static const uint32_t bases[] = {0, 65535u, P4_AFF_WEAK | 125u, P4_AFF_RESIST | 50u,
        P4_AFF_NULL | 100u, P4_AFF_REPEL | 100u, P4_AFF_DRAIN | 100u};
    for (unsigned legacy = 0; legacy < 16; ++legacy)
        for (unsigned added = 0; added < 16; ++added)
            for (unsigned guard = 0; guard < 2; ++guard) {
                DatUnit unit = {0}; unit.guarding = (int)guard;
                for (unsigned i = 0; i < 4; ++i) unit.skills[0x1b8 + i] = (legacy >> i) & 1u;
                uint32_t flags = ((added & 1u) ? P4_AFF_RESIST : 0) | ((added & 2u) ? P4_AFF_NULL : 0) |
                    ((added & 4u) ? P4_AFF_REPEL : 0) | ((added & 8u) ? P4_AFF_DRAIN : 0);
                for (size_t b = 0; b < sizeof bases / sizeof bases[0]; ++b) {
                    query_count = 0;
                    assert(p4_game_elements_native_affinity_apply(&unit, 0, bases[b], flags) ==
                        expected(bases[b], legacy | added, (int)guard, 0, 0));
                }
            }
}
static void base_row(P4GameTables *tables, uint32_t kind, uint32_t id, uint32_t profile,
    uint32_t leaf, uint32_t channel, uint32_t value, uint32_t scalar)
{
    P4AffinityRow row = {kind, id, profile, leaf, channel, value, scalar};
    assert(p4_game_tables_affinity(tables, "test", &row) == P4_TABLE_OK);
}
static void profile_alias(P4GameTables *tables, uint32_t kind, uint32_t id,
    uint32_t native_profile, uint32_t profile, const char *key)
{
    P4AffinityProfileAlias alias = {kind, id, native_profile, profile, id, 1, 1, ""};
    strcpy(alias.key, key);
    assert(p4_game_tables_affinity_alias(tables, "test", &alias) == P4_TABLE_OK);
}
static P4GameActionElement resolved(P4GameTables *tables, uint32_t leaf, uint32_t channel, uint32_t component)
{
    P4GameActionElement declared = {1, leaf, channel, 0, 0, component}, result;
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, UINT32_MAX,
        component, &declared, &result) == P4_TABLE_OK);
    return result;
}
static uint32_t effective(P4GameTables *tables, DatUnit *unit, const P4GameActionElement *element)
{
    uint32_t result = 0xfeedbeefu; query_count = 0;
    assert(p4_game_elements_affinity_effective(tables, unit, element, &result) == P4_TABLE_OK);
    return result;
}
static void side_rows_and_native_effects(uint32_t leaf_id)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4ElementDesc leaf = {.id = leaf_id, .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
        .family = 0, .order = 19, .revision = 1, .key = "test:leaf", .display_name = "Leaf", .short_name = "Leaf"};
    assert(p4_game_tables_element(tables, "test", &leaf) == P4_TABLE_OK);
    base_row(tables, P4_ENTITY_ENEMY, 1, 99, leaf_id, P4_CHANNEL_DAMAGE, P4_AFF_WEAK | 125u, 125);
    base_row(tables, P4_ENTITY_ENEMY, 2, 88, leaf_id, P4_CHANNEL_DAMAGE, P4_AFF_RESIST | 50u, 50);
    base_row(tables, P4_ENTITY_PERSONA, 20, 77, leaf_id, P4_CHANNEL_DAMAGE, 100, 100);
    base_row(tables, P4_ENTITY_PERSONA, 21, 78, leaf_id, P4_CHANNEL_DAMAGE, P4_AFF_DRAIN | 100u, 100);
    base_row(tables, P4_ENTITY_ENEMY, 1, 99, 6, P4_CHANNEL_DAMAGE, P4_AFF_WEAK | 125u, 125);
    base_row(tables, P4_ENTITY_ENEMY, 1, 99, 6, P4_CHANNEL_INSTANT_KO, P4_KO_NORMAL, 100);
    base_row(tables, P4_ENTITY_ENEMY, 2, 88, 6, P4_CHANNEL_DAMAGE, P4_AFF_RESIST | 50u, 50);
    base_row(tables, P4_ENTITY_ENEMY, 2, 88, 6, P4_CHANNEL_INSTANT_KO, P4_KO_NULL, 0);
    profile_alias(tables, P4_ENTITY_ENEMY, 1, 5, 99, "test:enemy-one");
    profile_alias(tables, P4_ENTITY_ENEMY, 2, 5, 88, "test:enemy-two");
    profile_alias(tables, P4_ENTITY_PERSONA, 20, 0, 77, "test:persona-one");
    profile_alias(tables, P4_ENTITY_PERSONA, 21, 0, 78, "test:persona-two");
    P4AffinityEffectBinding effects[] = {
        {P4_AFFINITY_EFFECT_SKILL, 600, leaf_id, P4_CHANNEL_DAMAGE, P4_AFF_RESIST, 1, 1, "test:skill-resist"},
        {P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 9, leaf_id, P4_CHANNEL_DAMAGE, P4_AFF_NULL, 1, 1, "test:gear-null"},
        {P4_AFFINITY_EFFECT_SKILL, 601, 6, P4_CHANNEL_DAMAGE, P4_AFF_DRAIN, 1, 1, "test:light-drain"},
        {P4_AFFINITY_EFFECT_SKILL, 602, leaf_id, P4_CHANNEL_DAMAGE, P4_AFF_REPEL, 1, 1, "test:accessory-repel"}
    };
    for (size_t i = 0; i < sizeof effects / sizeof effects[0]; ++i)
        assert(p4_game_tables_affinity_effect(tables, "test", &effects[i]) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    P4GameActionElement physical = resolved(tables, leaf_id, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE);
    P4GameActionElement light_damage = resolved(tables, 6, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE);
    P4GameActionElement light_ko = resolved(tables, 6, P4_CHANNEL_INSTANT_KO, P4_ACTION_COMPONENT_INSTANT_KO);
    const P4AffinityRow *static_base = NULL;
    assert(p4_game_elements_affinity_base(tables, P4_ENTITY_ENEMY, 1, 5, &physical, &static_base) == P4_TABLE_OK);
    assert(static_base->value == (P4_AFF_WEAK | 125u) && static_base->scalar == 125);
    DatUnit unit = {0}; memset(equipment, 0xff, sizeof equipment);
    put16(unit.native, 4); put16(unit.native + 2, 1); put16(unit.native + 4, 5);
    assert(effective(tables, &unit, &physical) == (P4_AFF_WEAK | 125u));
    unit.guarding = 1;
    assert(effective(tables, &unit, &physical) == 100);
    unit.guarding = 0; unit.skills[0x1b8] = 1;
    assert(effective(tables, &unit, &physical) == (P4_AFF_RESIST | 50u));
    unit.skills[0x1bb] = 1;
    assert(effective(tables, &unit, &physical) == (P4_AFF_DRAIN | 100u));
    memset(unit.skills, 0, sizeof unit.skills);
    put16(unit.native + 2, 2); /* Same native id2, different logical enemy. */
    assert(effective(tables, &unit, &physical) == (P4_AFF_RESIST | 50u));
    uint32_t unchanged = 0xfeedbeefu;
    put16(unit.native + 4, 6);
    assert(p4_game_elements_affinity_effective(tables, &unit, &physical, &unchanged) == P4_TABLE_NOT_FOUND);
    assert(unchanged == 0xfeedbeefu); /* Never invent another profile or alias. */
    put16(unit.native, 0); put16(unit.native + 2, 1); selected_persona = 20;
    assert(effective(tables, &unit, &physical) == 100);
    selected_persona = 21;
    assert(effective(tables, &unit, &physical) == (P4_AFF_DRAIN | 100u));
    selected_persona = 20; put16(unit.learned, 600);
    assert(effective(tables, &unit, &physical) == (P4_AFF_RESIST | 50u));
    equipment[1][0] = 9;
    assert(effective(tables, &unit, &physical) == (P4_AFF_NULL | 100u));
    equipment[1][2] = 10; put16(item_metadata[10] + 0x24, 602);
    assert(effective(tables, &unit, &physical) == (P4_AFF_REPEL | 100u));
    unit.skills[0x1bb] = 1;
    assert(effective(tables, &unit, &physical) == (P4_AFF_DRAIN | 100u));
    memset(&unit, 0, sizeof unit);
    put16(unit.native, 4); put16(unit.native + 2, 1); put16(unit.native + 4, 5);
    unit.skills[0x1cc] = 1; /* Old Resist Light is KO-only, not HP damage. */
    assert(effective(tables, &unit, &light_damage) == (P4_AFF_WEAK | 125u));
    assert(effective(tables, &unit, &light_ko) == (P4_AFF_RESIST | 50u));
    unit.skills[0x1ce] = 1;
    assert(effective(tables, &unit, &light_ko) == (P4_AFF_NULL | 100u)); /* KO never reflects/heals. */
    put16(unit.learned, 601);
    assert(effective(tables, &unit, &light_damage) == (P4_AFF_DRAIN | 100u));
    memset(unit.skills, 0, sizeof unit.skills); memset(unit.learned, 0, sizeof unit.learned);
    put16(unit.native + 2, 2);
    assert(effective(tables, &unit, &light_ko) == P4_AFF_NULL); /* Keep the explicit zero scalar. */
    assert(effective(tables, &unit, &light_damage) == (P4_AFF_RESIST | 50u));
    assert(static_base->value == (P4_AFF_WEAK | 125u) && static_base->scalar == 125);
    p4_game_tables_destroy(tables);
}
/* Wards: a one-leaf resist on a physical leaf is an equipped-item
 * affinity-effect row. It resists only its leaf and lifts that leaf's
 * weakness through the same precedence; the other leaves keep their base. */
static void one_leaf_ward(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    static const char *const keys[] = {"test:ward-slash-leaf", "test:ward-strike-leaf", "test:ward-pierce-leaf"};
    for (uint32_t leaf = 19; leaf <= 21; ++leaf) {
        P4ElementDesc d = {.id = leaf, .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
            .order = leaf, .revision = 1, .display_name = "Leaf", .short_name = "Leaf"};
        strcpy(d.key, keys[leaf - 19]);
        assert(p4_game_tables_element(tables, "test", &d) == P4_TABLE_OK);
        base_row(tables, P4_ENTITY_PERSONA, 20, 77, leaf, P4_CHANNEL_DAMAGE,
            leaf == 19 ? (P4_AFF_WEAK | 125u) : 100u, leaf == 19 ? 125 : 100);
    }
    profile_alias(tables, P4_ENTITY_PERSONA, 20, 0, 77, "test:ward-persona");
    const P4AffinityEffectBinding ward = {P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 11, 19, P4_CHANNEL_DAMAGE,
        P4_AFF_RESIST, 1, 1, "test:ward-slash"};
    assert(p4_game_tables_affinity_effect(tables, "test", &ward) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    P4GameActionElement slash = resolved(tables, 19, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE);
    P4GameActionElement strike = resolved(tables, 20, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE);
    P4GameActionElement pierce = resolved(tables, 21, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE);
    DatUnit unit = {0}; memset(equipment, 0xff, sizeof equipment); memset(item_metadata, 0, sizeof item_metadata);
    put16(unit.native + 2, 1); selected_persona = 20;
    assert(effective(tables, &unit, &slash) == (P4_AFF_WEAK | 125u));
    assert(effective(tables, &unit, &strike) == 100u && effective(tables, &unit, &pierce) == 100u);
    equipment[1][2] = 11; /* The ward accessory. */
    assert(effective(tables, &unit, &slash) == (P4_AFF_RESIST | 50u)); /* Resists, no longer weak. */
    assert(effective(tables, &unit, &strike) == 100u && effective(tables, &unit, &pierce) == 100u);
    equipment[1][2] = -1; equipment[1][1] = 11; /* Any equipped slot carries it. */
    assert(effective(tables, &unit, &slash) == (P4_AFF_RESIST | 50u));
    memset(equipment, 0xff, sizeof equipment);
    p4_game_tables_destroy(tables);
}
/* Side ailments: without a status-channel row a unit's susceptibility comes
 * from its default leaf's effective damage affinity, or is immune when every
 * retail ailment leaf is; an authored row and added effects still apply. */
static unsigned side_uses;
static int side_use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(view == context && ids);
    if (domain == P4_TABLE_SOURCE_SIDE_AILMENT) { assert(ids[0] == 22); ++side_uses; }
    return P4_TABLE_OK;
}
static void side_ailment_defaults(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    for (uint32_t leaf = 22; leaf <= 23; ++leaf) {
        P4ElementDesc d = {.id = leaf, .flags = P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, .family = leaf,
            .order = leaf, .revision = 1, .display_name = "Ailment", .short_name = "Ail"};
        strcpy(d.key, leaf == 22 ? "test:freeze" : "test:shock");
        assert(p4_game_tables_element(tables, "test", &d) == P4_TABLE_OK);
    }
    P4SideAilmentDef side = {22, P4_SIDE_FLAGS, 0x16, 2, 100, 1, 1, "test:freeze-ailment"};
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_OK);
    side.leaf = 23; side.default_from = P4_SIDE_DEFAULT_NONE; strcpy(side.key, "test:shock-ailment");
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_OK);
    profile_alias(tables, P4_ENTITY_PERSONA, 20, 0, 77, "test:side-persona");
    base_row(tables, P4_ENTITY_PERSONA, 20, 77, 0, P4_CHANNEL_DAMAGE, 100, 100); /* A profile has a row. */
    profile_alias(tables, P4_ENTITY_PERSONA, 21, 0, 78, "test:side-authored");
    base_row(tables, P4_ENTITY_PERSONA, 21, 78, 22, P4_CHANNEL_STATUS, P4_AFF_WEAK | 125u, 125);
    const P4AffinityEffectBinding ward = {P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 11, 22, P4_CHANNEL_STATUS,
        P4_AFF_NULL, 1, 1, "test:freeze-ward"};
    assert(p4_game_tables_affinity_effect(tables, "test", &ward) == P4_TABLE_OK);
    assert(!p4_game_tables_bind_runtime_use(tables, side_use, tables));
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    const P4GameActionElement freeze = resolved(tables, 22, P4_CHANNEL_STATUS, P4_ACTION_COMPONENT_STATUS);
    const P4GameActionElement shock = resolved(tables, 23, P4_CHANNEL_STATUS, P4_ACTION_COMPONENT_STATUS);
    DatUnit unit = {0}; memset(equipment, 0xff, sizeof equipment); memset(item_metadata, 0, sizeof item_metadata);
    put16(unit.native + 2, 1); selected_persona = 20;
    legacy_by_leaf = 1;
    for (uint32_t leaf = 0; leaf < 19; ++leaf) legacy_leaf_words[leaf] = 100u;
    static const struct { uint32_t ice, expected; } ice[] = {
        {P4_AFF_WEAK | 125u, P4_AFF_WEAK | 125u}, {100u, 100u}, {P4_AFF_RESIST | 50u, P4_AFF_RESIST | 50u},
        {P4_AFF_NULL | 100u, P4_AFF_NULL | 100u}, {P4_AFF_REPEL | 100u, P4_AFF_NULL | 100u},
        {P4_AFF_DRAIN | 100u, P4_AFF_NULL | 100u},
    };
    for (size_t i = 0; i < sizeof ice / sizeof ice[0]; ++i) {
        legacy_leaf_words[2] = ice[i].ice;
        assert(effective(tables, &unit, &freeze) == ice[i].expected);
    }
    /* Immune to every retail ailment (the no-effect flag): immune whatever Ice is. */
    legacy_leaf_words[2] = P4_AFF_WEAK | 125u;
    for (uint32_t leaf = 8; leaf <= 15; ++leaf) legacy_leaf_words[leaf] = 0x20000000u | 100u;
    assert(effective(tables, &unit, &freeze) == (P4_AFF_NULL | 100u));
    legacy_leaf_words[15] = 100u; /* One ailment lands: Ice decides again. */
    assert(effective(tables, &unit, &freeze) == (P4_AFF_WEAK | 125u));
    side_uses = 0;
    uint32_t value = 0;
    assert(p4_game_elements_runtime_affinity_effective(tables, &unit, &freeze, &value) == P4_TABLE_OK);
    assert(value == (P4_AFF_WEAK | 125u) && side_uses == 1);
    equipment[1][2] = 11; /* The ward nulls the defaulted ailment. */
    assert(effective(tables, &unit, &freeze) == (P4_AFF_NULL | 100u));
    equipment[1][2] = -1;
    selected_persona = 21; legacy_leaf_words[2] = P4_AFF_NULL | 100u; /* An authored row wins. */
    assert(effective(tables, &unit, &freeze) == (P4_AFF_WEAK | 125u));
    selected_persona = 20; value = 0xfeedbeefu;
    assert(p4_game_elements_affinity_effective(tables, &unit, &shock, &value) == P4_TABLE_NOT_FOUND && value == 0xfeedbeefu);
    legacy_by_leaf = 0; legacy_calls = 0; /* Later cases count their own original calls. */
    memset(equipment, 0xff, sizeof equipment);
    p4_game_tables_destroy(tables);
}
/* Damage cover: one owner's families over a unit's skill list, equipped items
 * and accessory skill, then the stage modifiers at catalog publication. */
static unsigned cover_uses;
static int cover_use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(view == context && ids && domain == P4_TABLE_SOURCE_COVER);
    ++cover_uses;
    return P4_TABLE_OK;
}
static uint32_t cover_load_order(const char *owner) { return !strcmp(owner, "test") ? 3u : 9u; }
static void add_cover(P4GameTables *tables, const char *owner, uint32_t kind, uint32_t source, uint32_t leaf,
    uint32_t percent, const char *family, const char *key)
{
    P4DamageCoverBinding cover = {kind, source, leaf, P4_CHANNEL_DAMAGE, percent, 1, 1, "", ""};
    strcpy(cover.family, family); strcpy(cover.key, key);
    assert(p4_game_tables_damage_cover(tables, owner, &cover) == P4_TABLE_OK);
}
static int doubled(void *user, const P4GameDamageHit *hit, double *value)
{
    (void)user; (void)hit; *value = 2.0; return 1;
}
static void damage_cover(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    for (uint32_t leaf = 19; leaf <= 20; ++leaf) {
        P4ElementDesc d = {.id = leaf, .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
            .order = leaf, .revision = 1, .display_name = "Leaf", .short_name = "Leaf"};
        strcpy(d.key, leaf == 19 ? "test:cover-slash" : "test:cover-strike");
        assert(p4_game_tables_element(tables, "test", &d) == P4_TABLE_OK);
    }
    add_cover(tables, "test", P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 12, 19, 25, "test:lining", "test:lining-low");
    add_cover(tables, "test", P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 13, 19, 50, "test:lining", "test:lining-high");
    add_cover(tables, "test", P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 14, 20, 50, "test:lining", "test:lining-strike");
    add_cover(tables, "test", P4_AFFINITY_EFFECT_SKILL, 700, 19, 75, "test:guard", "test:guard-slash");
    add_cover(tables, "test", P4_AFFINITY_EFFECT_SKILL, 701, 19, 50, "test:guard", "test:guard-granted");
    add_cover(tables, "other", P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 15, 19, 50, "other:lining", "other:lining-slash");
    assert(!p4_game_tables_bind_runtime_use(tables, cover_use, tables));
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    memset(equipment, 0xff, sizeof equipment); memset(item_metadata, 0, sizeof item_metadata);
    memset(pc_skills, 0, sizeof pc_skills);
    double factor = -1.0;
    /* Nothing carried: no cover, and the result is untouched. */
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 0 && factor == -1.0);
    /* Same family: the highest of armor 25% and accessory 50%; a second family multiplies. */
    equipment[1][1] = 12; equipment[1][2] = 13; pc_skills[1][3] = 700;
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.25);
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 20, 0, &factor) == 0); /* Only its leaf. */
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 21, 0, &factor) == 0);
    assert(p4_game_damage_cover_factor(tables, "other", 0, 1, 19, 0, &factor) == 0); /* Only the owner's rows. */
    /* The accessory's granted skill is a source after its item; the family keeps its maximum. */
    equipment[1][1] = -1; pc_skills[1][3] = 0; put16(item_metadata[13] + 0x24, 701);
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.5);
    equipment[1][0] = 15;
    assert(p4_game_damage_cover_factor(tables, "other", 0, 1, 19, 0, &factor) == 1 && factor == 0.5);
    cover_uses = 0;
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 1, &factor) == 1 && cover_uses == 2);
    /* An enemy's metadata skill list is a source; enemies wear nothing. */
    static uint8_t enemy_rows[401 * 0x3C];
    const uint32_t saved = P4_G32(0x007644b4u);
    P4_G32(0x007644b4u) = P4_H2G(enemy_rows);
    put16(enemy_rows + 2u * 0x3Cu + 0xEu + 4u, 700);
    put16(enemy_rows + 400u * 0x3Cu + 0xEu, 700);
    assert(p4_game_damage_cover_factor(tables, "test", 1, 2, 19, 0, &factor) == 1 && factor == 0.25);
    assert(p4_game_damage_cover_factor(tables, "test", 1, 3, 19, 0, &factor) == 0);
    assert(p4_game_damage_cover_factor(tables, "test", 1, 336, 19, 0, &factor) == P4_TABLE_NOT_FOUND);
    enemy_limit = 401; /* An added enemy past the retail bank reads the relocated rows. */
    assert(p4_game_damage_cover_factor(tables, "test", 1, 400, 19, 0, &factor) == 1 && factor == 0.25);
    assert(p4_game_damage_cover_factor(tables, "test", 1, 401, 19, 0, &factor) == P4_TABLE_NOT_FOUND);
    enemy_limit = 336;
    /* Granted passives are sources after the slots, items and accessory: a
     * party member's, its current Persona's, an equipped item's and an enemy
     * kind's grants each reach the unit, and the family keeps its maximum. A
     * grant of a skill the unit already has, or a second owner's, is not
     * visited again; revoking one owner's grant leaves the other's. */
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.5);
    assert(p4_game_passive_grant("test", 3, P4_PASSIVE_PARTY, 1, 700) == 1);
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.25);
    assert(p4_game_damage_cover_factor(tables, "test", 0, 2, 19, 0, &factor) == 0); /* Only member 1. */
    assert(p4_game_passive_revoke(3, P4_PASSIVE_PARTY, 1, 700) == 1);
    selected_persona = 0x2a;
    assert(p4_game_passive_grant("test", 3, P4_PASSIVE_PERSONA, 0x2a, 700) == 1);
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.25);
    selected_persona = 0x2b;
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.5);
    assert(p4_game_passive_grant("test", 3, P4_PASSIVE_ITEM, 15, 700) == 1);
    assert(p4_game_passive_grant("other", 9, P4_PASSIVE_ITEM, 15, 700) == 1);
    cover_uses = 0;
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 1, &factor) == 1 && factor == 0.5 * 0.25 &&
        cover_uses == 2);
    assert(p4_game_passive_revoke_owner(3) == 2);
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.25);
    assert(p4_game_passive_revoke_owner(9) == 1);
    assert(p4_game_damage_cover_factor(tables, "test", 0, 1, 19, 0, &factor) == 1 && factor == 0.5 * 0.5);
    assert(p4_game_passive_grant("test", 3, P4_PASSIVE_ENEMY, 3, 700) == 1);
    assert(p4_game_damage_cover_factor(tables, "test", 1, 3, 19, 0, &factor) == 1 && factor == 0.25);
    assert(p4_game_passive_grant("test", 3, P4_PASSIVE_ENEMY, 2, 700) == 1); /* Already in its row. */
    assert(p4_game_damage_cover_factor(tables, "test", 1, 2, 19, 0, &factor) == 1 && factor == 0.25);
    assert(p4_game_passive_revoke_owner(3) == 2 && !p4_game_passive_count() && !p4_game_passive_live);
    /* Publication: one multiply per owner, nothing before it, none after session end. */
    assert(p4_game_damage_count() == 0);
    assert(p4_game_damage_cover_prepare(tables, cover_load_order) == P4_TABLE_OK && p4_game_damage_count() == 0);
    p4_game_damage_cover_publish();
    assert(p4_game_damage_count() == 2);
    p4_game_damage_cover_publish(); /* Idempotent. */
    assert(p4_game_damage_count() == 2);
    uint32_t handle;
    const P4GameDamageModifierDesc boost = {"test-boost", 100, 1, P4_DAMAGE_MULTIPLY, doubled, NULL};
    assert(p4_game_damage_register(&boost, &handle) == P4_DAMAGE_OK);
    P4GameDamageHit hit = {.skill_id = 1, .target_side = P4_DAMAGE_SIDE_PARTY, .target_id = 1,
        .hit_kind = P4_DAMAGE_HIT_WEAK, .element_valid = 1, .element_id = 19, .stock = 1000};
    cover_uses = 0;
    assert(p4_game_damage_apply(&hit) == 250 && cover_uses == 3); /* 1000 x2 x(0.5 x 0.5) x0.5 */
    hit.resolved = 1;
    assert(p4_game_damage_apply(&hit) == 250); /* Estimates and resolved hits agree. */
    hit.element_id = 20;
    assert(p4_game_damage_apply(&hit) == 2000);
    hit.element_id = 19; hit.element_valid = 0;
    assert(p4_game_damage_apply(&hit) == 2000); /* A hit without a leaf has no cover. */
    hit.element_valid = 1; hit.target_side = P4_DAMAGE_SIDE_ENEMY; hit.target_id = 2;
    assert(p4_game_damage_apply(&hit) == 500);
    assert(p4_game_damage_unregister(handle) == P4_DAMAGE_OK);
    p4_game_damage_cover_session_end();
    assert(p4_game_damage_count() == 0);
    assert(p4_game_damage_cover_prepare(tables, cover_load_order) == P4_TABLE_OK); /* A fresh session plans again. */
    p4_game_damage_cover_session_end();
    P4_G32(0x007644b4u) = saved;
    memset(equipment, 0xff, sizeof equipment); memset(item_metadata, 0, sizeof item_metadata);
    p4_game_tables_destroy(tables);
}
/* Scripted source/AI/UI/knowledge probes exercise atomic ordering only, NOT
 * native action-storage qualification. element-route is the real participant
 * that element staging registers for its compiled native call-site cutover. */
static unsigned consumer_prepares, consumer_commits, failed_consumer;
static const P4GameTables *expected_prepared_tables;
static int consumer_prepare(void *user)
{
    ++consumer_prepares;
    if ((uintptr_t)user != 3u) {
        assert(p4_game_elements_prepared_snapshot() == expected_prepared_tables);
        assert(p4_game_elements_prepared_skills() == inputs.skills);
    }
    return failed_consumer == (unsigned)(uintptr_t)user ? P4_TABLE_INVALID : P4_TABLE_OK;
}
static void consumer_commit(void *user)
{
    (void)user; assert(consumer_prepares == 4); ++consumer_commits;
}
static void consumers(const P4GameTables *tables)
{
    expected_prepared_tables = tables;
    assert(p4_game_tables_battle_complete_register("element-ui", consumer_prepare, consumer_commit, (void *)(uintptr_t)2) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("element-knowledge", consumer_prepare, consumer_commit, (void *)(uintptr_t)1) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("table-source", consumer_prepare, consumer_commit, (void *)(uintptr_t)3) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("element-ai", consumer_prepare, consumer_commit, (void *)(uintptr_t)4) == P4_TABLE_OK);
}
static void detach(void)
{
    assert(p4_game_elements_session_end() == P4_TABLE_OK);
    assert(!p4_game_elements_prepared_snapshot());
    assert(!p4_game_elements_prepared_skills());
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    consumer_prepares = consumer_commits = failed_consumer = 0;
}
static P4GameTables *native_graph(int complete, int roster)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    assert(!p4_game_tables_bind_runtime_use(tables, registry_use, tables));
    P4ElementDesc leaf = {.id = 19, .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
        .revision = 1, .key = "test:native-physical", .display_name = "Native physical", .short_name = "Native"};
    assert(p4_game_tables_element(tables, "test", &leaf) == P4_TABLE_OK);
    P4ActionElementBinding action = {.source_kind = P4_ACTION_ELEMENT_SKILL, .source_id = 120,
        .component = P4_ACTION_COMPONENT_DAMAGE, .element_id = 19, .schema = 1, .revision = 1, .key = "test:native-action"};
    assert(p4_game_tables_action_element(tables, "test", &action) == P4_TABLE_OK);
    P4AffinityEffectBinding effect = {.source_kind = P4_AFFINITY_EFFECT_SKILL, .source_id = 450,
        .element_id = 19, .flags = P4_AFF_RESIST, .schema = 1, .revision = 1, .key = "test:native-passive"};
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_OK);
    profile_alias(tables, P4_ENTITY_ENEMY, 1, 5, 7, "test:native-enemy");
    profile_alias(tables, P4_ENTITY_PERSONA, 20, 0, 99, "test:native-persona");
    base_row(tables, P4_ENTITY_ENEMY, 1, 7, 19, P4_CHANNEL_DAMAGE, 100, 100);
    base_row(tables, P4_ENTITY_PERSONA, 20, 99, complete ? 19 : 1, P4_CHANNEL_DAMAGE, 100, 100);
    /* Each enemy row's initial selector (id2 = id) and every persona record
     * 1..255 answers the requirement. */
    for (uint32_t enemy = 0; (roster & 1) && enemy < P4_ENEMY_RETAIL_COUNT + ((roster >> 2) & 1); ++enemy) {
        char key[P4_TABLE_KEY_SIZE];
        snprintf(key, sizeof key, "test:native-enemy-row-%u", (unsigned)enemy);
        profile_alias(tables, P4_ENTITY_ENEMY, enemy, enemy, 7, key);
        if (enemy != 1) base_row(tables, P4_ENTITY_ENEMY, enemy, 7, 19, P4_CHANNEL_DAMAGE, 100, 100);
    }
    for (uint32_t persona = 1; (roster & 2) && persona < P4_PERSONA_RETAIL_COUNT + ((roster >> 3) & 1); ++persona) {
        if (persona == 20) continue;
        char key[P4_TABLE_KEY_SIZE];
        snprintf(key, sizeof key, "test:native-persona-row-%u", (unsigned)persona);
        profile_alias(tables, P4_ENTITY_PERSONA, persona, 0, 99, key);
        base_row(tables, P4_ENTITY_PERSONA, persona, 99, 19, P4_CHANNEL_DAMAGE, 100, 100);
    }
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    return tables;
}
static void native_publication(void)
{
    uint8_t *metadata = p4t_galloc(440u * 40u, 16), *elements = p4t_galloc(576u * 2u, 16);
    uint8_t *enemies = p4t_galloc(336u * 60u, 16); assert(metadata && elements && enemies);
    elements[10u * 2u] = 1; put16(enemies + 60u, 0x4004);
    P4_G32(0x007644a4u) = P4_H2G(elements); P4_G32(0x007644a8u) = P4_H2G(metadata);
    P4_G32(0x007644b4u) = P4_H2G(enemies);
    P4GameTables *empty = p4_game_tables_create(); assert(empty);
    assert(p4_game_tables_freeze(empty, 1) == P4_TABLE_OK);
    assert(p4_game_elements_boot_stage(empty, NULL, NULL) == P4_TABLE_OK);
    assert(!p4_game_elements_active() && !p4_game_elements_native_enabled());
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK && !p4_game_elements_native_enabled());
    assert(P4_G32(0x007644a4u) == P4_H2G(elements) && P4_G32(0x007644a8u) == P4_H2G(metadata));
    detach(); p4_game_tables_destroy(empty);
    P4GameTables *claimed = p4_game_tables_create(); assert(claimed);
    P4ElementDesc builtin = *p4_game_tables_find_element(claimed, 0);
    builtin.flags &= ~P4_ELEMENT_VISIBLE; builtin.revision = 1;
    assert(p4_game_tables_element(claimed, "test", &builtin) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(claimed, 1) == P4_TABLE_OK);
    assert(p4_game_tables_element_count(claimed) == P4_ELEMENT_RETAIL_COUNT);
    assert(p4_game_elements_boot_stage(claimed, inputs.skills, NULL) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY && !consumer_prepares && !consumer_commits);
    assert(!p4_game_elements_active()); detach(); p4_game_tables_destroy(claimed);
    P4GameTables *incomplete = native_graph(0, 3);
    assert(p4_game_elements_boot_stage(incomplete, inputs.skills, NULL) == P4_TABLE_OK); consumers(incomplete);
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY);
    assert(consumer_prepares == 1 && !consumer_commits && !p4_game_elements_active());
    assert(!p4_game_elements_prepared_snapshot());
    detach(); p4_game_tables_destroy(incomplete);
    /* Declared aliases alone are not enough: an enemy row whose initial
     * selector, or a persona record, has no alias would refuse mid-battle or
     * in the compendium, so prepare refuses first. An added enemy or persona
     * the staged snapshots declare needs its own alias too (roster 3 covers
     * only the retail rows; roster bit 4 adds enemy 336, bit 8 persona 256). */
    for (int roster = 0; roster < 6; ++roster) {
        declared_enemies = roster == 3 || roster == 5 ? 337 : 336;
        declared_personas = roster == 4 || roster == 5 ? 257 : 256;
        P4GameTables *uncovered = native_graph(1, roster < 3 ? roster : roster == 3 ? 3 : roster == 4 ? 7 : 11);
        assert(p4_game_elements_boot_stage(uncovered, inputs.skills, NULL) == P4_TABLE_OK); consumers(uncovered);
        assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY);
        assert(consumer_prepares == 1 && !consumer_commits && !p4_game_elements_active());
        assert(!p4_game_elements_prepared_snapshot());
        detach(); p4_game_tables_destroy(uncovered);
    }
    P4GameTables *added = native_graph(1, 15);
    assert(p4_game_elements_boot_stage(added, inputs.skills, NULL) == P4_TABLE_OK); consumers(added);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK && p4_game_elements_active() == added);
    detach(); p4_game_tables_destroy(added);
    declared_enemies = 336; declared_personas = 256;
    P4GameTables *tables = native_graph(1, 3);
    assert(p4_game_elements_boot_stage(tables, inputs.skills, NULL) == P4_TABLE_OK);
    assert(!p4_game_elements_prepared_snapshot());
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY && !p4_game_elements_active());
    detach();
    /* The real route owns its slot: no second participant can stand in for
     * the compiled cutover. */
    assert(p4_game_elements_boot_stage(tables, inputs.skills, NULL) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("element-route", consumer_prepare, consumer_commit,
        (void *)(uintptr_t)5) == P4_TABLE_CONFLICT);
    detach();
    assert(p4_game_elements_boot_stage(tables, inputs.skills, NULL) == P4_TABLE_OK); consumers(tables);
    failed_consumer = 2;
    assert(p4_game_tables_battle_complete() == P4_TABLE_INVALID);
    assert(consumer_prepares == 4 && !consumer_commits && !p4_game_elements_active());
    assert(!p4_game_elements_native_enabled());
    assert(p4_game_elements_prepared_snapshot() == tables); /* Prepared is not published capability. */
    detach();
    assert(p4_game_elements_boot_stage(tables, inputs.skills, NULL) == P4_TABLE_OK); consumers(tables);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(consumer_prepares == 4 && consumer_commits == 4 && p4_game_elements_active() == tables);
    assert(p4_game_elements_native_enabled());
    DatUnit unit = {0}; put16(unit.native, 4); put16(unit.native + 2, 1); put16(unit.native + 4, 5);
    P4GameUnitIdentity identity;
    assert(p4_game_elements_unit_identity(&unit, &identity) == P4_TABLE_OK);
    assert(identity.entity_kind == P4_ENTITY_ENEMY && identity.entity_id == 1 && identity.native_profile == 5);
    P4GameEnemyContext context;
    assert(p4_game_elements_native_enemy_context(&unit, &context) == P4_TABLE_OK);
    assert(context.enemy_id == 1 && context.native_profile == 5 && context.profile_id == 7 &&
        context.knowledge_id == 1 && context.metadata_flags == 0x4004);
    const P4GameEnemyContext saved = context;
    put16(unit.native + 4, 6);
    assert(p4_game_elements_native_enemy_context(&unit, &context) == P4_TABLE_NOT_FOUND && !memcmp(&context, &saved, sizeof context));
    put16(unit.native + 2, 336);
    assert(p4_game_elements_native_enemy_context(&unit, &context) == P4_TABLE_NOT_READY && !memcmp(&context, &saved, sizeof context));
    put16(unit.native + 2, 1); put16(unit.native + 4, 5); put16(unit.learned, 450);
    P4GameActionFrame frame, preview;
    const unsigned registry_before_preview = registry_uses;
    assert(!p4_game_elements_native_preview_primary_frame(&unit, 120, 0x80000002u, &preview));
    assert(!original_skill_uses);
    assert(registry_uses == registry_before_preview);
    p4_game_elements_native_primary_frame(&unit, 120, 0x80000002u, &frame);
    assert(original_skill_uses == 1 && !memcmp(&preview, &frame, sizeof frame));
    assert(frame.element.element_id == 19 && frame.owned_binding && frame.native_options == 0x80000002u);
    query_count = 0;
    assert(p4_game_elements_native_affinity(&unit, &frame) == (P4_AFF_RESIST | 50u));
    unit.skills[0x1ba] = 1; query_count = 0;
    assert(p4_game_elements_native_affinity_leaf(&unit, 19, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE) == (P4_AFF_REPEL | 100u));
    uint32_t profile_word = 0xdeadbeefu;
    assert(p4_game_elements_native_profile_affinity(&unit, 19, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == (P4_AFF_REPEL | 100u) && !legacy_calls && !legacy_wrapper_calls);
    assert(p4_game_elements_native_profile_affinity(&unit, 19, P4_CHANNEL_STATUS, P4_ACTION_COMPONENT_STATUS, &profile_word) == P4_TABLE_NOT_FOUND);
    assert(profile_word == (P4_AFF_REPEL | 100u) && !legacy_calls && !legacy_wrapper_calls);
    /* Phys0 is a natural retail pair like leaves 1..18: battle's original
     * route covers it, so an unchanged cohort that keeps it visible needs no row. */
    assert(p4_game_elements_native_profile_affinity(&unit, 0, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == legacy_word && !legacy_calls && legacy_wrapper_calls == 1);
    assert(p4_game_elements_native_profile_affinity(&unit, 1, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == legacy_word && !legacy_calls && legacy_wrapper_calls == 2);
    assert(p4_game_elements_native_profile_affinity(&unit, 6, P4_CHANNEL_INSTANT_KO, P4_ACTION_COMPONENT_INSTANT_KO, &profile_word) == P4_TABLE_OK);
    assert(profile_word == legacy_word && legacy_wrapper_calls == 3); /* Do not clamp untouched retail KO words. */
    assert(p4_game_elements_native_profile_affinity(&unit, 6, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_NOT_FOUND);
    assert(profile_word == legacy_word && legacy_wrapper_calls == 3);
    assert(p4_game_elements_native_profile_affinity(NULL, 16, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == 100 && legacy_wrapper_calls == 3);
    assert(p4_game_elements_native_profile_base_affinity(&unit, 1, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == legacy_word && legacy_calls == 1 && legacy_wrapper_calls == 3);
    assert(p4_game_elements_native_profile_base_affinity(&unit, 6, P4_CHANNEL_INSTANT_KO, P4_ACTION_COMPONENT_INSTANT_KO, &profile_word) == P4_TABLE_OK);
    assert(profile_word == legacy_word && legacy_calls == 2 && legacy_wrapper_calls == 3);
    assert(p4_game_elements_native_profile_base_affinity(&unit, 19, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == (P4_AFF_REPEL | 100u) && legacy_calls == 2 && legacy_wrapper_calls == 3);
    assert(p4_game_elements_native_profile_base_affinity(&unit, 0, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == legacy_word && legacy_calls == 3 && legacy_wrapper_calls == 3);
    assert(p4_game_elements_native_profile_base_affinity(NULL, 16, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_OK);
    assert(profile_word == 100 && legacy_calls == 3 && legacy_wrapper_calls == 3);
    /* The pure display predicates: natural retail pairs and the prepared battle requirement. */
    assert(p4_game_elements_profile_natural_pair(tables, 0, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE));
    assert(p4_game_elements_profile_natural_pair(tables, 7, P4_CHANNEL_INSTANT_KO, P4_ACTION_COMPONENT_INSTANT_KO));
    assert(!p4_game_elements_profile_natural_pair(tables, 6, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE));
    assert(!p4_game_elements_profile_natural_pair(tables, 19, P4_CHANNEL_DAMAGE, P4_ACTION_COMPONENT_DAMAGE));
    assert(legacy_calls == 3 && legacy_wrapper_calls == 3);
    assert(p4_game_elements_prepared_required_pair(19, P4_CHANNEL_DAMAGE) == 1);
    assert(p4_game_elements_prepared_required_pair(19, P4_CHANNEL_STATUS) == 0);
    assert(p4_game_elements_prepared_required_pair(0, P4_CHANNEL_DAMAGE) == 0);
    legacy_calls = 0;
    legacy_wrapper_calls = 0;
    P4GameAffinityQuery neutral = {P4_AFFINITY_QUERY_PROVIDED, 100};
    query_count = 0;
    assert(p4_game_elements_native_affinity_query(NULL, &frame, &neutral) == 100 && !query_count);
    assert(!p4_game_elements_native_preview_primary_frame(&unit, 10, 0, &preview));
    assert(original_skill_uses == 1);
    p4_game_elements_native_primary_frame(&unit, 10, 0, &frame);
    assert(original_skill_uses == 2 && !memcmp(&preview, &frame, sizeof frame));
    assert(frame.element.element_id == 1 && !frame.owned_binding);
    assert(p4_game_elements_native_affinity(&unit, &frame) == legacy_word && legacy_calls == 1 && !legacy_wrapper_calls);
    assert(p4_game_elements_native_element_affinity(&unit, &frame) == legacy_word && legacy_wrapper_calls == 1);
    /* Presentation routes: a retail leaf keeps its exact value and legacy bit;
     * the bound physical leaf 19 presents as its declared family 0, with no
     * ailment-class bit, for actor-derived and actorless queries alike. */
    assert(p4_game_elements_native_presentation_leaf(&unit, 10) == 1);
    assert(p4_game_elements_native_hit_class_bits(&unit, 10) == 1u << 2);
    assert(p4_game_elements_native_presentation_leaf(&unit, 120) == 0);
    assert(p4_game_elements_native_hit_class_bits(&unit, 120) == 0u);
    assert(p4_game_elements_native_presentation_leaf(NULL, 120) == 0);
    assert(original_skill_uses == 7); /* Each presentation query is one actual skill use. */
    put16(unit.native, 0); put16(unit.native + 2, 1); selected_persona = 20;
    assert(p4_game_elements_unit_identity(&unit, &identity) == P4_TABLE_OK);
    assert(identity.entity_kind == P4_ENTITY_PERSONA && identity.entity_id == 20 && !identity.native_profile);
    assert(P4_G32(0x007644a4u) == P4_H2G(elements) && P4_G32(0x007644a8u) == P4_H2G(metadata));
    assert(registry_domains[P4_TABLE_SOURCE_ELEMENT] && registry_domains[P4_TABLE_SOURCE_AFFINITY] &&
        registry_domains[P4_TABLE_SOURCE_ACTION] && registry_domains[P4_TABLE_SOURCE_ALIAS] &&
        registry_domains[P4_TABLE_SOURCE_EFFECT] && !registry_domains[P4_TABLE_SOURCE_CHANNEL]);
    registry_source_result = 1; query_count = 0;
    const uint32_t preserved_profile = profile_word;
    assert(p4_game_elements_native_profile_affinity(&unit, 19, P4_CHANNEL_DAMAGE,
        P4_ACTION_COMPONENT_DAMAGE, &profile_word) == P4_TABLE_INVALID);
    assert(profile_word == preserved_profile && !query_count);
    registry_source_result = 0;
    consumer_prepares = consumer_commits = 0;
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK && consumer_prepares == 4 && consumer_commits == 4);
    detach();
    assert(p4_game_elements_prepared_required_pair(19, P4_CHANNEL_DAMAGE) == P4_TABLE_NOT_READY);
    p4_game_tables_destroy(tables);
}
int main(void)
{
    if (p4t_guest_map(0x00764000u, 0x2000u)) return 77;
    inputs.skills = p4_game_skills_create(NULL); assert(inputs.skills);
    assert(!p4_game_skills_bind_runtime_use(inputs.skills, original_skill_use, NULL, NULL));
    p4_test_element_inputs_begin(&inputs);
    p4_test_element_inputs_materialize(&inputs);
    precedence(); query_order_and_specials(); additional_effects();
    side_rows_and_native_effects(19); side_rows_and_native_effects(65535u);
    one_leaf_ward(); side_ailment_defaults(); damage_cover();
    native_publication();
    assert(original_skill_uses == 7); /* Skill120/skill10 action frames plus five presentation queries. */
    p4_game_skills_destroy(inputs.skills);
    p4_package_registry_retire(); p4_test_element_inputs_release(&inputs);
    puts("game_elements_affinity: shared native guard/passive/break/wall precedence and query order passed (scripted facts; not battle qualification)");
    return 0;
}
