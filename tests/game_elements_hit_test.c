/* Exact 0023e6f0 body with scripted engine facts, not a gameplay qualification. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#define TYPE_H
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef float f32;
#include "p4pc/game_elements_calc_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_knowledge_native.h"
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_chance_stage.h"
#include "p4pc/game_chance_stage_native.h"
#include "p4pc/game_hit_stage.h"
#include "p4pc/mod_entry.h"
typedef struct DatUnit {
    u16 flags, id, id2; u8 level, pad07; u16 hp, sp; u32 bad; u8 rest[0x20];
} DatUnit;
typedef struct DatCalcUnitState {
    u8 pad00[0x14]; u32 buffFlags, consumedFlags; u8 rest[0x10]; u16 message, pad2e;
} DatCalcUnitState;
typedef struct DatEnemyUnitInit { u8 pad[0x38], weaponElement, rest[3]; } DatEnemyUnitInit;
typedef struct DatSkillEntry { u8 flags00, pad01[0x17], mode18, pad19[3]; u32 statusMask; u8 rest[8]; } DatSkillEntry;
_Static_assert(sizeof(DatUnit) == 0x30 && offsetof(DatUnit, bad) == 0xc, "unit ABI");
_Static_assert(offsetof(DatCalcUnitState, buffFlags) == 0x14 && offsetof(DatCalcUnitState, message) == 0x2c, "calculation ABI");
_Static_assert(sizeof(DatEnemyUnitInit) == 0x3c && sizeof(DatSkillEntry) == 40 &&
    offsetof(DatSkillEntry, statusMask) == 0x1c, "source row ABI");
#define DAT_SCRIPT_CALC_HIT_RATE 4
#define DAT_SCRIPT_CALC_SCALED_RATIO 5
#define DAT_SCRIPT_CALC_RATIO_OF_PRODUCTS 6
#define MEMBER_OFFSET(type, member) offsetof(type, member)
enum { METADATA_BYTES = 1025 * 40, ELEMENT_BYTES = 1025 * 2 };
static u8 *metadata, *elements, *enemy_rows, D_00635938[1];
static u8 *iGpffffb3b8, *iGpffffb3b4, *iGpffffb3c4;
f32 iGpffff8170 = 0.7f, fGpffff80cc = 0.8f;
static DatUnit source_unit, target_unit;
static P4GameActionFrame expected_frame;
static P4GameActionFrame expected_status_frame;
static u32 expected_status_mask;
static unsigned enabled, random_calls, source_calls, native_affinity_calls, legacy_affinity_calls;
static u32 random_state, affinity, ko_light, ko_dark, attack_element, guard, fixed_skill, equip_property, counter_property, current_skill = 10;
static u32 requested_skill;
static u16 listed_skills[8];
static u32 held_count;
static u16 rescue_character = 1, social_rank;
uint32_t p4_mod_authorize_mask;
static int protection_allowed = 1, protection_calls;
int p4_mod_authorize_battle_protection_item(int character, int item, int element)
{
    assert(character == rescue_character && item == 0x340 && (element == 6 || element == 7));
    ++protection_calls;
    return protection_allowed;
}
static void *expected_action_source;
static uint64_t trace;
static void mark(u32 x) { trace = trace * UINT64_C(1099511628211) ^ x; }
static void kAssertFail(const void *file, s32 line) { (void)file; fprintf(stderr, "Unexpected retail assertion %x\n", (unsigned)line); abort(); }
_Noreturn void p4_game_elements_native_calculation_refuse(int32_t result) { fprintf(stderr, "Unexpected native refusal %d\n", result); abort(); }
int p4_game_elements_native_enabled(void) { return (int)enabled; }
int p4_game_skill_active(uint32_t id) { return id < P4_SKILL_RETAIL_ACTIVE_COUNT || id == 1024; }
int p4_game_skill_registered(uint32_t id) { return id < P4_SKILL_RETAIL_COUNT || id == 1024; }
/* No status-ailment rows: the side-ailment readers are inert (game_side_ailment_battle.c). */
int p4_side_ailment_native_lose_turn(void *unit) { (void)unit; return 0; }
int32_t p4_side_ailment_native_crit_bonus(void *target, uint32_t affinity) { (void)target; (void)affinity; return 0; }
/* Unexpanded enemy banks: the retail 0x150 domain. */
uint32_t p4_game_enemies_native_limit(void) { return 0x150u; }
int32_t p4_game_skill_element(uint32_t id, int32_t retail) { assert(p4_game_skill_active(id)); return retail; }
void p4_game_elements_native_primary_frame(void *unit, uint32_t skill, uint32_t options, P4GameActionFrame *out)
{ assert(unit == &source_unit && skill == current_skill && options == expected_frame.native_options); ++source_calls; *out = expected_frame; }
void p4_game_elements_native_applicability_frame(void *unit, uint32_t skill, P4GameActionFrame *out)
{ assert((!unit || unit == &source_unit) && skill == current_skill); ++source_calls; *out = expected_frame; }
void p4_game_elements_native_action_frame(void *unit, uint32_t skill, uint32_t component, uint32_t status, uint32_t options, P4GameActionFrame *out)
{
    assert(unit == (expected_action_source ? expected_action_source : &source_unit) && skill == current_skill && options == expected_frame.native_options);
    ++source_calls;
    if (component == P4_ACTION_COMPONENT_DAMAGE) { assert(!status); *out = expected_frame; }
    else { assert(status == expected_status_mask && component == expected_status_frame.element.component); *out = expected_status_frame; }
}
uint32_t p4_game_elements_native_affinity(void *unit, const P4GameActionFrame *frame)
{ assert(unit == &target_unit && frame->element.element_id == expected_status_frame.element.element_id &&
    frame->element.channel_id == expected_status_frame.element.channel_id); ++native_affinity_calls; mark(100); return affinity; }
uint32_t p4_game_elements_native_element_affinity(void *unit, const P4GameActionFrame *frame)
{ assert(unit == &target_unit && frame->element.element_id == expected_frame.element.element_id && frame->element.channel_id == expected_frame.element.channel_id); ++native_affinity_calls; mark(100); return affinity; }
int p4_game_elements_native_profile_affinity(void *unit, uint32_t leaf, uint32_t channel, uint32_t component, uint32_t *out)
{ assert(unit == &target_unit && channel == P4_CHANNEL_INSTANT_KO && component == P4_ACTION_COMPONENT_INSTANT_KO); ++native_affinity_calls; mark(100 + leaf); *out = leaf == 6 ? ko_light : ko_dark; return P4_TABLE_OK; }
/* Landed-ailment observation for Analyze; outside the traced retail order. */
static unsigned observe_calls;
static P4GameActionFrame observed_frame;
static u16 target_flags;
int p4_game_knowledge_native_observe(void *defender, const P4GameActionFrame *frame)
{ assert(defender == &target_unit && frame); ++observe_calls; observed_frame = *frame; return P4_TABLE_OK; }
static u8 *byteAddressAdd(u32 offset, u8 *base) { return base + offset; }
static s32 datCalcIsGuarding(u16 *unit) { (void)unit; mark(10); return (s32)guard; }
static s32 datCalcGetElementAffinity(u8 *unit, s32 element)
{ (void)unit; ++legacy_affinity_calls; if (metadata[current_skill * 40 + 0x24] == 12 && (element == 6 || element == 7)) { mark(100 + (u32)element); return (s32)(element == 6 ? ko_light : ko_dark); } mark(100); return (s32)affinity; }
static s32 datCalcGetAffinity(u8 *unit, s32 element)
{ assert(unit == (u8 *)&target_unit); (void)element; ++legacy_affinity_calls; mark(100); return (s32)affinity; }
s32 datCalcStatusElement(s32 skill, s32 status)
{ assert((u16)skill == current_skill && (u32)status == expected_status_mask); return (s32)attack_element; }
static s32 datCalcSkillElement(DatUnit *unit, u16 skill)
{ assert(unit == &source_unit && skill == current_skill); return (s32)attack_element; }
static s32 datCalcHasRequestedSkill(DatUnit *unit, s32 id)
{ assert(unit == &source_unit); mark(7000 + (u32)id); return (u32)id == requested_skill; }
static s32 datItemElement(s16 item) { (void)item; return (s32)attack_element; }
static s32 datGetEquippedItemByUnit(s16 id, s16 slot) { (void)id; (void)slot; return 1; }
static u8 *datPersonaGetSkillsByPcId(s32 id) { assert(id == rescue_character); mark(8000); return (u8 *)listed_skills; }
static s32 datItemGrantedSkill(s16 id) { assert(id == 1); mark(8001); return 0; }
static s32 datItemCount(s32 id) { assert(id == 0x340); mark(8002); return (s32)held_count; }
static s32 cmmCommunityForArcana(s16 id) { assert(id == rescue_character); return id; }
static s32 cmmCommunityRecordValue(u16 id) { assert(id == rescue_character); return social_rank; }
static s32 datCalcHasFixedSkill(u8 *unit, s32 id) { (void)unit; mark(1000 + (u32)id); return fixed_skill == (u32)id; }
static s32 datCalcCountEquipmentProperty(u8 *unit, s32 id) { (void)unit; mark(2000 + (u32)id); return counter_property == (u32)id; }
static s32 datCalcHasEquipmentProperty(u8 *unit, s32 id) { (void)unit; mark(3000 + (u32)id); return equip_property == (u32)id; }
static u32 RpRandom(void) { ++random_calls; random_state = random_state * 1664525u + 1013904223u; mark(4000); return random_state; }
static f32 datCalcBuffMultiplier(u8 *unit, s32 stat, u16 skill) { (void)unit; (void)skill; mark(5000 + (u32)stat); return 1.0f; }
static s8 datCalcGetBuffLevel(DatUnit *unit, s32 index) { (void)unit; mark(5500 + (u32)index); return 0; }
static s32 datScriptRunCalc(u32 kind, u16 *source, u16 *target, u16 skill, s32 parameter)
{
    assert(source == (u16 *)&source_unit && target == (u16 *)&target_unit && skill == current_skill);
    if (kind == DAT_SCRIPT_CALC_RATIO_OF_PRODUCTS) { assert((u32)parameter == expected_status_mask); mark(6002); return 45; }
    assert((kind == DAT_SCRIPT_CALC_HIT_RATE || kind == DAT_SCRIPT_CALC_SCALED_RATIO) && !parameter);
    mark(6000 + (kind == DAT_SCRIPT_CALC_SCALED_RATIO)); return kind == DAT_SCRIPT_CALC_HIT_RATE ? 75 : 35;
}
int32_t p4_game_elements_calculation_run(uint32_t kind, void *source, void *target, uint32_t skill, int32_t parameter, const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ assert(kind == DAT_SCRIPT_CALC_HIT_RATE && source == &source_unit && target == &target_unit && skill == current_skill && !parameter && frame && !query); mark(6000); return 75; }
/* The chance stage asks for the party source's active persona record only
 * while it has a modifier. */
static unsigned persona_reads;
static u16 datPersonaGetRecordId(u16 party) { assert(party == source_unit.id); ++persona_reads; return 0x2a; }
static u8 datPersonaGetLevelByPcId(u16 party) { assert(party == source_unit.id); ++persona_reads; return 31; }
/* It reports the target's afflictions this battle, read only while it has a
 * modifier. */
static unsigned affliction_reads;
uint32_t p4_game_afflictions_native_count(const void *unit) { assert(unit == &target_unit); ++affliction_reads; return 3; }
#include "../overrides/0023e6f0.c"
#include "../overrides/002411a0.c"
#include "../overrides/00241bc0.c"
#include "../overrides/00238940.c"
static P4GameChanceHit chance_seen;
static unsigned chance_calls;
static double chance_factor;
static int chance_modifier(void *user, const P4GameChanceHit *hit, double *value)
{ (void)user; chance_seen = *hit; ++chance_calls; *value = chance_factor; return 1; }
static P4GameHitQuery hit_seen;
static unsigned hit_calls;
static double hit_factor;
static int hit_modifier(void *user, const P4GameHitQuery *query, double *value)
{ (void)user; hit_seen = *query; ++hit_calls; *value = hit_factor; return 1; }
/* Passive grants (game_passive_grant_native.h): the fixture's one granted
 * skill on the source; 00242990 asks only while something is granted. */
uint32_t p4_game_passive_live;
static u32 granted_skill;
static unsigned grant_queries;
int p4_game_passive_native_unit(const void *unit, uint32_t skill)
{ assert(unit == &source_unit); ++grant_queries; return skill == granted_skill; }
#include "../overrides/00242990.c"
typedef struct Receipt { s32 result; DatUnit source, target; u32 rng, calls; uint64_t trace; } Receipt;
static Receipt run(unsigned route, u32 leaf, u32 family, u32 flags, u32 channel, u32 buffs, u32 aff, u32 fixed, u32 equip, u32 counter, u32 options, int weapon)
{
    memset(&source_unit, 0, sizeof source_unit); memset(&target_unit, 0, sizeof target_unit);
    memset(metadata, 0, METADATA_BYTES); memset(elements, 0, ELEMENT_BYTES);
    metadata[current_skill * 40] = weapon ? 2 : 0; metadata[current_skill * 40 + 0xe] = 75; elements[current_skill * 2] = (u8)family;
    source_unit.id = 1; target_unit.id = 2; ((DatCalcUnitState *)&target_unit)->buffFlags = buffs;
    enabled = route; affinity = aff; fixed_skill = fixed; equip_property = equip; counter_property = counter;
    attack_element = family; random_state = 0x1234; random_calls = source_calls = native_affinity_calls = legacy_affinity_calls = 0; trace = 0;
    expected_frame = (P4GameActionFrame){.element = {1, leaf, channel, flags, family, P4_ACTION_COMPONENT_DAMAGE}, .skill_id = current_skill, .native_options = options};
    s32 result = datCalcResolveSkillHit((u8 *)&source_unit, (u8 *)&target_unit, (u16)current_skill, (s32)options);
    if (route) assert(source_calls == 1 && !legacy_affinity_calls); else assert(!source_calls && !native_affinity_calls);
    return (Receipt){result, source_unit, target_unit, random_state, random_calls, trace};
}
static Receipt ko_case(unsigned route, u32 light, u32 dark)
{
    memset(&source_unit, 0, sizeof source_unit); memset(&target_unit, 0, sizeof target_unit);
    memset(metadata, 0, METADATA_BYTES); metadata[10 * 40 + 0x24] = 12;
    enabled = route; ko_light = light; ko_dark = dark; random_state = 0x1234;
    random_calls = source_calls = native_affinity_calls = legacy_affinity_calls = 0; trace = 0;
    s32 result = datCalcResolveSkillHit((u8 *)&source_unit, (u8 *)&target_unit, 10, 0);
    assert(!source_calls && (route ? !legacy_affinity_calls : !native_affinity_calls));
    assert((route ? native_affinity_calls : legacy_affinity_calls) == (light & P4_AFF_WEAK ? 1u : 2u));
    return (Receipt){result, source_unit, target_unit, random_state, random_calls, trace};
}
static Receipt critical_case(unsigned route, u32 leaf, u32 family, u32 flags, u32 aff, u8 damage_class, u8 critical_rate, u32 outcome)
{
    memset(&source_unit, 0, sizeof source_unit); memset(&target_unit, 0, sizeof target_unit);
    memset(metadata, 0, METADATA_BYTES); memset(elements, 0, ELEMENT_BYTES);
    source_unit.id = 1; target_unit.id = 2; target_unit.flags = 4;
    metadata[current_skill * 40 + 2] = damage_class;
    metadata[current_skill * 40 + 0x25] = critical_rate;
    elements[current_skill * 2] = (u8)family;
    enabled = route; affinity = aff; fixed_skill = equip_property = counter_property = 0;
    attack_element = family; random_state = 0x1234;
    random_calls = source_calls = native_affinity_calls = legacy_affinity_calls = 0; trace = 0;
    expected_frame = (P4GameActionFrame){.element = {1, leaf, 29, flags, family, P4_ACTION_COMPONENT_DAMAGE}, .skill_id = current_skill};
    s32 result = datCalcHitResult((u8 *)&source_unit, (u8 *)&target_unit, (u16)current_skill, (s32)outcome, 0);
    if (outcome == 0) assert(route ? source_calls == 1 && !legacy_affinity_calls : !source_calls && !native_affinity_calls);
    else assert(!source_calls && !native_affinity_calls && !legacy_affinity_calls);
    return (Receipt){result, source_unit, target_unit, random_state, random_calls, trace};
}
static Receipt applicability_case(unsigned route, u32 leaf, u32 family, u32 flags, u32 channel, int effect, u16 target_flags, u32 target_bad, int actor)
{
    memset(&source_unit, 0, sizeof source_unit); memset(&target_unit, 0, sizeof target_unit);
    memset(metadata, 0, METADATA_BYTES); memset(elements, 0, ELEMENT_BYTES);
    metadata[current_skill * 40 + 0x11] = effect ? 1 : 0;
    elements[current_skill * 2] = (u8)family;
    source_unit.id = 1; target_unit.id = 2; target_unit.flags = target_flags; target_unit.bad = target_bad; enabled = route;
    random_state = 0x1234; random_calls = source_calls = native_affinity_calls = legacy_affinity_calls = 0; trace = 0;
    expected_frame = (P4GameActionFrame){.element = {1, leaf, 29, flags, family, P4_ACTION_COMPONENT_DAMAGE}, .skill_id = current_skill};
    s32 result = datCalcSkillAppliesToTarget(actor ? (u8 *)&source_unit : NULL, (u8 *)&target_unit, (s32)current_skill, (s32)channel, 0);
    assert(!native_affinity_calls && !legacy_affinity_calls && !random_calls);
    assert(source_calls == (unsigned)(route && channel == 1 && effect && !(target_flags & 8) && !(target_bad & 0x100000)));
    return (Receipt){result, source_unit, target_unit, random_state, random_calls, trace};
}
static Receipt status_case(unsigned route, u32 status, u32 leaf, u32 family, u32 flags, u32 word, u32 options, u8 rate, u8 selection)
{
    memset(&source_unit, 0, sizeof source_unit); memset(&target_unit, 0, sizeof target_unit);
    memset(metadata, 0, METADATA_BYTES); memset(elements, 0, ELEMENT_BYTES);
    DatSkillEntry *entry = (DatSkillEntry *)(metadata + current_skill * 40);
    entry->mode18 = selection; entry->statusMask = status;
    metadata[current_skill * 40 + 0x19] = rate;
    source_unit.id = 1; target_unit.id = 2; target_unit.flags = target_flags;
    enabled = route; affinity = word; attack_element = family; guard = 0;
    expected_status_mask = selection == 3 ? (((0x1234u * 1664525u + 1013904223u) % 2u) ? 0x20u : 0x2u) : status;
    expected_frame = (P4GameActionFrame){.element = {1, leaf, 29, flags, family, P4_ACTION_COMPONENT_DAMAGE}, .skill_id = current_skill, .native_options = options};
    expected_status_frame = (P4GameActionFrame){.element = {1, leaf, 31, flags, family,
        expected_status_mask == 0x80000 ? P4_ACTION_COMPONENT_INSTANT_KO : P4_ACTION_COMPONENT_STATUS}, .skill_id = current_skill};
    random_state = 0x1234; random_calls = source_calls = native_affinity_calls = legacy_affinity_calls = 0; trace = 0;
    s32 result = datCalcInflictStatus((u16)current_skill, &source_unit, &target_unit, (s32)options);
    assert(route ? !legacy_affinity_calls : !native_affinity_calls);
    return (Receipt){result, source_unit, target_unit, random_state, random_calls, trace};
}
static Receipt substitution_case(unsigned route, u32 leaf, u32 family, u16 listed, u32 items, u16 unit_flags, u8 category, u32 statuses, int bound)
{
    memset(&source_unit, 0, sizeof source_unit); memset(&target_unit, 0, sizeof target_unit);
    memset(metadata, 0, METADATA_BYTES); memset(elements, 0, ELEMENT_BYTES);
    memset(listed_skills, 0, sizeof listed_skills); listed_skills[0] = listed;
    metadata[current_skill * 40 + 0x24] = category; elements[current_skill * 2] = (u8)family;
    source_unit.id = rescue_character; source_unit.flags = unit_flags; enabled = route; held_count = items; attack_element = family;
    expected_frame = (P4GameActionFrame){.element = {1, leaf, 29, P4_ELEMENT_AFFINITY, family, P4_ACTION_COMPONENT_DAMAGE}, .skill_id = current_skill};
    expected_status_mask = 0x80000u; expected_action_source = &target_unit;
    expected_status_frame = (P4GameActionFrame){.element = {1, leaf, P4_CHANNEL_INSTANT_KO, P4_ELEMENT_AFFINITY, family, P4_ACTION_COMPONENT_INSTANT_KO}, .skill_id = current_skill, .owned_binding = bound};
    random_state = 0x1234; random_calls = source_calls = native_affinity_calls = legacy_affinity_calls = 0; trace = 0;
    s32 result = datCalcResolveSkillId(&source_unit, (s32)current_skill, &target_unit, statuses);
    expected_action_source = NULL;
    assert(!native_affinity_calls && !legacy_affinity_calls);
    return (Receipt){result, source_unit, target_unit, random_state, random_calls, trace};
}
int main(void)
{
    const size_t arena_size = 0x20000;
#ifdef _WIN32
    u8 *arena = VirtualAlloc((void *)(uintptr_t)0x20000000u, arena_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    u8 *arena = mmap((void *)(uintptr_t)0x20000000u, arena_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) arena = NULL;
#endif
    if (!arena || (uintptr_t)arena > UINT32_MAX - arena_size) return 77;
    iGpffffb3b8 = metadata = arena;
    iGpffffb3b4 = elements = arena + 0xb000;
    iGpffffb3c4 = enemy_rows = arena + 0xc000;
    for (u32 family = 0; family < 19; ++family) {
        const u32 flags = (family < 16 ? P4_ELEMENT_AFFINITY : 0) | (family == 0 ? P4_ELEMENT_PHYSICAL : 0) | (family < 8 ? P4_ELEMENT_OFFENSIVE : 0);
        for (u32 mode = 0; mode < 7; ++mode) {
            u32 buffs = mode == 1 ? 0x10000u : mode == 2 ? 0x20000u : 0;
            u32 word = mode == 3 ? P4_AFF_REPEL : mode == 4 ? P4_AFF_DRAIN : mode == 5 ? P4_AFF_NULL : 100u;
            u32 fixed = mode == 6 ? 0x1f8u : 0;
            for (int weapon = 0; weapon < 2; ++weapon) {
                Receipt retail = run(0, family, family, flags, 0, buffs, word, fixed, 0, 0, 0, weapon);
                Receipt tagged = run(1, family, family, flags, 0, buffs, word, fixed, 0, 0, 0, weapon);
                assert(retail.result == tagged.result && retail.rng == tagged.rng &&
                    retail.calls == tagged.calls && retail.trace == tagged.trace &&
                    !memcmp(&retail.source, &tagged.source, sizeof retail.source) &&
                    !memcmp(&retail.target, &tagged.target, sizeof retail.target));
            }
        }
    }
    for (u32 mode = 0; mode < 3; ++mode) {
        u32 light = mode == 0 ? P4_AFF_WEAK : 0, dark = mode == 1 ? P4_AFF_WEAK : 0;
        Receipt retail = ko_case(0, light, dark), tagged = ko_case(1, light, dark);
        assert(retail.result == tagged.result && retail.trace == tagged.trace &&
            retail.rng == tagged.rng && retail.calls == tagged.calls);
    }
    Receipt wide = run(1, UINT32_MAX, 0, P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 19, 0, P4_AFF_REPEL, 0, 0, 0, 0, 0);
    assert(wide.result == 0x200 && !wide.calls && native_affinity_calls == 1);
    wide = run(1, 0x80000000u, 6, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, P4_CHANNEL_DAMAGE, 0, P4_AFF_DRAIN, 0, 0, 0, 0, 0);
    assert(wide.result == 0x400 && !wide.calls && native_affinity_calls == 1);
    wide = run(1, 0xfffffffeu, 1, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 29, 0x20000u, 100, 0, 0, 0, 0, 0);
    assert(wide.result == 0x200 && !wide.calls && !native_affinity_calls &&
        (((DatCalcUnitState *)&wide.target)->consumedFlags & 0x20000u));
    wide = run(1, 0xfffffffdu, 0, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 29, 0x10000u, P4_AFF_DRAIN, 0, 0, 0, 0, 0);
    assert(wide.result == 0x400 && native_affinity_calls == 1 &&
        !(((DatCalcUnitState *)&wide.target)->consumedFlags & 0x10000u));
    current_skill = 1024;
    Receipt extended_skill = run(0, 1, 1, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 0, 0, 100, 0, 0, 0, 0, 0);
    Receipt tagged_skill = run(1, 1, 1, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 0, 0, 100, 0, 0, 0, 0, 0);
    assert(extended_skill.result == tagged_skill.result && extended_skill.trace == tagged_skill.trace &&
        extended_skill.rng == tagged_skill.rng && extended_skill.calls == tagged_skill.calls);
    for (u32 family = 0; family < 19; ++family) {
        u32 flags = (family < 16 ? P4_ELEMENT_AFFINITY : 0) |
            (family == 0 ? P4_ELEMENT_PHYSICAL : 0) | (family < 8 ? P4_ELEMENT_OFFENSIVE : 0);
        for (u32 mode = 0; mode < 4; ++mode) {
            Receipt retail = critical_case(0, family, family, flags, mode == 1 ? P4_AFF_WEAK : 100, mode == 2 ? 1 : 2, mode == 3 ? 100 : 0, 0);
            Receipt tagged = critical_case(1, family, family, flags, mode == 1 ? P4_AFF_WEAK : 100, mode == 2 ? 1 : 2, mode == 3 ? 100 : 0, 0);
            assert(retail.result == tagged.result && retail.trace == tagged.trace &&
                retail.rng == tagged.rng && retail.calls == tagged.calls);
        }
    }
    Receipt critical = critical_case(1, UINT32_MAX, 0, P4_ELEMENT_PHYSICAL | P4_ELEMENT_AFFINITY, P4_AFF_WEAK, 1, 100, 0);
    assert(critical.result == 4 && !critical.calls);
    critical = critical_case(1, 0xfffffffdu, 0, P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 100, 1, 100, 0);
    assert(critical.result == 1 && !critical.calls);
    critical = critical_case(1, UINT32_MAX, 0, P4_ELEMENT_PHYSICAL | P4_ELEMENT_AFFINITY, 100, 1, 100, 0x200);
    assert(critical.result == 1 && !critical.calls && !source_calls);
    for (u32 family = 0; family < 19; ++family) {
        u32 flags = (family < 16 ? P4_ELEMENT_AFFINITY : 0) |
            (family == 0 ? P4_ELEMENT_PHYSICAL : 0) | (family < 8 ? P4_ELEMENT_OFFENSIVE : 0);
        for (u32 channel = 0; channel <= 1; ++channel) {
            Receipt retail = applicability_case(0, family, family, flags, channel, 1, 0, 0, 1);
            Receipt tagged = applicability_case(1, family, family, flags, channel, 1, 0, 0, 1);
            assert(retail.result == tagged.result && retail.trace == tagged.trace);
        }
    }
    Receipt applicable = applicability_case(1, UINT32_MAX, 0, P4_ELEMENT_PHYSICAL | P4_ELEMENT_AFFINITY, 1, 1, 0, 0, 1);
    assert(applicable.result == 1 && source_calls == 1);
    applicable = applicability_case(1, UINT32_MAX, 0, P4_ELEMENT_OFFENSIVE, 1, 1, 0, 0, 1);
    assert(applicable.result == 0 && source_calls == 1);
    applicable = applicability_case(1, UINT32_MAX, 0, P4_ELEMENT_PHYSICAL | P4_ELEMENT_AFFINITY, 1, 0, 0, 0, 1);
    assert(applicable.result == 0 && !source_calls);
    applicable = applicability_case(1, UINT32_MAX, 0, P4_ELEMENT_AFFINITY, 1, 1, 8, 0, 1);
    assert(applicable.result == 0 && !source_calls);
    applicable = applicability_case(1, UINT32_MAX, 0, P4_ELEMENT_AFFINITY, 1, 1, 0, 0x100000, 1);
    assert(applicable.result == 0 && !source_calls);
    applicable = applicability_case(1, UINT32_MAX, 0, P4_ELEMENT_AFFINITY, 1, 1, 0, 0, 0);
    assert(applicable.result == 1 && source_calls == 1);
    for (u32 family = 0; family < 19; ++family) {
        u32 flags = (family < 16 ? P4_ELEMENT_AFFINITY : 0) |
            (family == 0 ? P4_ELEMENT_PHYSICAL : 0);
        for (u32 kind = 0; kind < 2; ++kind)
            for (u32 mode = 0; mode < 3; ++mode)
                for (u32 options = 0; options < 2; ++options) {
                    u32 status = kind ? 0x80000u : 0x20u;
                    u32 word = mode == 0 ? 100u : mode == 1 ? 0x01000064u : 0x04000064u;
                    Receipt retail = status_case(0, status, family, family, flags, word, options, 75, 1);
                    Receipt tagged = status_case(1, status, family, family, flags, word, options, 75, 1);
                    assert(retail.result == tagged.result && retail.trace == tagged.trace &&
                        retail.rng == tagged.rng && retail.calls == tagged.calls);
                }
    }
    Receipt status_wide = status_case(1, 0x20u, UINT32_MAX, 9, P4_ELEMENT_AFFINITY, 100, 0, 100, 1);
    assert(status_wide.result == 0x20 && source_calls == 2 && native_affinity_calls == 1 && !status_wide.calls);
    Receipt retail_random = status_case(0, 0x22u, 9, 9, P4_ELEMENT_AFFINITY, 100, 0, 75, 3);
    Receipt tagged_random = status_case(1, 0x22u, UINT32_MAX, 9, P4_ELEMENT_AFFINITY, 100, 0, 75, 3);
    assert(retail_random.result == tagged_random.result && retail_random.trace == tagged_random.trace &&
        retail_random.rng == tagged_random.rng && retail_random.calls == tagged_random.calls &&
        tagged_random.calls == 2 && native_affinity_calls == 1);
    /* A routed ailment that lands on an enemy is observed once; immunity, a
     * missed roll, a KO payload, a party target and the retail route are not. */
    target_flags = 4; observe_calls = 0;
    Receipt landed = status_case(1, 0x20u, 9, 9, P4_ELEMENT_AFFINITY, 100, 0, 100, 1);
    assert(landed.result == 0x20 && observe_calls == 1 && observed_frame.element.element_id == 9 &&
        observed_frame.element.component == P4_ACTION_COMPONENT_STATUS);
    Receipt immune = status_case(1, 0x20u, 9, 9, P4_ELEMENT_AFFINITY, 0x01000064u, 0, 100, 1);
    Receipt missed = status_case(1, 0x20u, 9, 9, P4_ELEMENT_AFFINITY, 0x10000032u, 0, 0, 1);
    Receipt ko = status_case(1, 0x80000u, 7, 7, P4_ELEMENT_AFFINITY, 100, 0, 100, 1);
    Receipt retail_landed = status_case(0, 0x20u, 9, 9, P4_ELEMENT_AFFINITY, 100, 0, 100, 1);
    assert(!immune.result && !missed.result && missed.calls == 1 && ko.result == 0x80000 &&
        retail_landed.result == 0x20 && observe_calls == 1);
    target_flags = 0;
    Receipt party = status_case(1, 0x20u, 9, 9, P4_ELEMENT_AFFINITY, 100, 0, 100, 1);
    assert(party.result == 0x20 && observe_calls == 1);
    const u16 replacements[] = {0, 0x20f, 0x210, 0x211, 0x212, 0x221, 0x222, 0x231, 0x232};
    for (u32 family = 0; family < 19; ++family)
        for (size_t i = 0; i < sizeof replacements / sizeof replacements[0]; ++i)
            for (u32 items = 0; items < 2; ++items) {
                Receipt retail = substitution_case(0, family, family, replacements[i], items, 0, 0, 0x80000, 1);
                Receipt tagged = substitution_case(1, family, family, replacements[i], items, 0, 0, 0x80000, 1);
                assert(retail.result == tagged.result && retail.trace == tagged.trace &&
                    retail.rng == tagged.rng && retail.calls == tagged.calls);
            }
    Receipt light = substitution_case(1, UINT32_MAX, 6, 0x221, 0, 0, 0, 0x80000, 1);
    assert(light.result == 0x221 && source_calls == 1);
    Receipt unrelated = substitution_case(1, 6, 0x10006, 0, 1, 0, 0, 0x80000, 1);
    assert(unrelated.result == -1 && source_calls == 1);
    Receipt early = substitution_case(1, UINT32_MAX, 6, 0x231, 0, 0, 0, 0x80000, 1);
    assert(early.result == 0x231 && !source_calls);
    early = substitution_case(1, UINT32_MAX, 6, 0, 0, 0x20, 0, 0x80000, 1);
    assert(early.result == 0 && !source_calls);
    early = substitution_case(1, UINT32_MAX, 6, 0, 0, 0, 13, 0x80000, 1);
    assert(early.result == -1 && !source_calls);
    Receipt light_damage = substitution_case(1, 6, 6, 0x221, 1, 0, 0, 0, 1);
    assert(light_damage.result == -1 && !source_calls);
    Receipt default_ko = substitution_case(1, 7, 6, 0x221, 0, 0, 0, 0x80000, 0);
    assert(default_ko.result == 0x221 && source_calls == 2);
    /* Each of the eight searches sees a granted passive exactly as one in a
     * slot: the same substitution, RNG draws and engine calls. Nothing granted
     * asks nothing. */
    for (u32 family = 0; family < 19; ++family)
        for (size_t i = 1; i < sizeof replacements / sizeof replacements[0]; ++i)
            for (u32 items = 0; items < 2; ++items) {
                p4_game_passive_live = 0; granted_skill = 0; grant_queries = 0;
                Receipt slot = substitution_case(1, family, family, replacements[i], items, 0, 0, 0x80000, 1);
                assert(!grant_queries);
                p4_game_passive_live = 1; granted_skill = replacements[i];
                Receipt granted = substitution_case(1, family, family, 0, items, 0, 0, 0x80000, 1);
                assert(slot.result == granted.result && slot.rng == granted.rng && slot.calls == granted.calls);
            }
    p4_game_passive_live = 0; granted_skill = 0;
    /* Item authorization occurs before the native fallback searches. Denial
     * leaves the stock untouched and does not suppress passive or social rescue. */
    for (u32 family = 6; family <= 7; ++family) {
        p4_mod_authorize_mask = 0;
        protection_calls = 0;
        protection_allowed = 0;
        Receipt available = substitution_case(1, family, family, 0, 1, 0, 0, 0x80000, 1);
        assert(available.result == 0x154 && held_count == 1 && !protection_calls);
        p4_mod_authorize_mask = P4_MOD_AUTH_BATTLE_PROTECTION_ITEM_BIT;
        Receipt denied = substitution_case(1, family, family, 0, 1, 0, 0, 0x80000, 1);
        assert(denied.result == -1 && held_count == 1 && protection_calls == 1);
        Receipt passive = substitution_case(1, family, family, 0x210, 1, 0, 0, 0x80000, 1);
        assert(passive.result == 0x210 && held_count == 1);
        Receipt immunity = substitution_case(1, family, family, family == 6 ? 0x221 : 0x222, 1, 0, 0, 0x80000, 1);
        assert(immunity.result == (family == 6 ? 0x221 : 0x222) && held_count == 1);
        rescue_character = 2; social_rank = 9;
        Receipt social = substitution_case(1, family, family, 0, 1, 0, 0, 0x80000, 1);
        assert(social.result == 0x20f && held_count == 1);
        rescue_character = 1; social_rank = 0;
        protection_allowed = 1;
        Receipt allowed = substitution_case(1, family, family, 0, 1, 0, 0, 0x80000, 1);
        assert(allowed.result == 0x154 && held_count == 1);
        protection_calls = 0;
        Receipt empty = substitution_case(1, family, family, 0, 0, 0, 0, 0x80000, 1);
        assert(empty.result == -1 && !protection_calls);
        protection_calls = 0;
        Receipt enemy = substitution_case(1, family, family, 0, 1, 4, 0, 0x80000, 1);
        assert(enemy.result == -1 && !protection_calls && held_count == 1);
    }
    p4_mod_authorize_mask = 0;
    /* Status/KO chance stage: the native truncated chance (45 from the ratio
     * of products at 100% affinity) is composed before the 0..99 clamp and the
     * single RpRandom() draw. An identity modifier changes nothing. */
    {
        const u32 roll = (0x1234u * 1664525u + 1013904223u) % 100u;
        const P4GameChanceModifierDesc desc = {"test.chance", 0, 0, P4_CHANCE_MULTIPLY, chance_modifier, NULL};
        uint32_t handle = 0;
        for (u32 kind = 0; kind < 2; ++kind)
            for (u32 options = 0; options < 2; ++options) {
                const u32 status = kind ? 0x80000u : 0x20u;
                Receipt plain = status_case(1, status, 9, 9, P4_ELEMENT_AFFINITY, 100, options, 75, 1);
                assert(p4_game_chance_register(&desc, &handle) == P4_DAMAGE_OK);
                const double factors[] = {1.0, 2.0, 0.0, 3.0};
                for (size_t f = 0; f < sizeof factors / sizeof factors[0]; ++f) {
                    chance_factor = factors[f]; chance_calls = persona_reads = affliction_reads = 0;
                    Receipt staged = status_case(1, status, 9, 9, P4_ELEMENT_AFFINITY, 100, options, 75, 1);
                    s32 rate = (s32)(45.0 * factors[f]);
                    rate = rate > 99 ? 99 : rate < 0 ? 0 : rate;
                    assert(chance_calls == 1 && staged.calls == plain.calls && staged.rng == plain.rng);
                    assert(staged.result == ((s32)roll < rate ? (s32)status : 0));
                    if (factors[f] == 1.0) assert(staged.result == plain.result && staged.trace == plain.trace);
                    assert(chance_seen.stock == 45 && chance_seen.statuses == status && chance_seen.item == options &&
                        chance_seen.component == (kind ? P4_CHANCE_COMPONENT_INSTANT_KO : P4_CHANCE_COMPONENT_AILMENT) &&
                        chance_seen.source_side == P4_DAMAGE_SIDE_PARTY && chance_seen.target_id == 2 &&
                        chance_seen.source_persona == (options ? 0u : 0x2au) && chance_seen.afflictions == 3 &&
                        chance_seen.source_persona_level == (options ? 0u : 31u) && persona_reads == (options ? 0u : 2u) &&
                        affliction_reads == 1);
                }
                /* A guaranteed rate (100 or more) never reaches the stage. */
                chance_calls = affliction_reads = 0;
                (void)status_case(1, status, 9, 9, P4_ELEMENT_AFFINITY, 100, options, 100, 1);
                assert(!chance_calls && !affliction_reads);
                assert(p4_game_chance_unregister(handle) == P4_DAMAGE_OK && !p4_game_chance_native_active());
            }
    }
    /* Preserve the original Light/Dark status rule, including non-KO
     * payloads: neither legacy passive is repurposed into HP damage. */
    {
        static int policy_owner;
        const P4GameOffensePolicy policy = {1.2f, 1.3f, 1.55f};
        assert(p4_game_offense_policy_enable(&policy_owner, &policy) == P4_DAMAGE_OK);
        const P4GameChanceModifierDesc desc = {"test.legacy-ko", 0, 0, P4_CHANCE_MULTIPLY, chance_modifier, NULL};
        uint32_t handle = 0; chance_factor = 1.0;
        assert(p4_game_chance_register(&desc, &handle) == P4_DAMAGE_OK);
        for (u32 family = 6; family <= 7; ++family)
            for (u32 route = 0; route <= 1; ++route)
                for (u32 kind = 0; kind <= 1; ++kind) {
                    requested_skill = family == 6 ? 0x21fu : 0x220u;
                    chance_calls = 0;
                    (void)status_case(route, kind ? 0x80000u : 0x20u, family, family,
                        P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 100, 0, 75, 1);
                    assert(chance_calls == 1 && chance_seen.stock == 56);
                    (void)status_case(route, kind ? 0x80000u : 0x20u, family, family,
                        P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY, 100, 1, 75, 1);
                    assert(chance_seen.stock == 45);
                }
        requested_skill = 0;
        p4_game_offense_policy_disable(&policy_owner);
        assert(p4_game_chance_unregister(handle) == P4_DAMAGE_OK);
    }
    /* Hit stage: a rolled hit's chance (75 from the formula, inside the 50..99
     * clamp) and a critical chance (35 from the scaled ratio) pass it before
     * their compare. An identity modifier changes nothing, 0 never succeeds,
     * 100 always does, and the RNG draw count never changes. */
    {
        const P4GameHitModifierDesc desc = {"test.hit", 0, 0, P4_HIT_MULTIPLY, hit_modifier, NULL};
        const u32 flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY;
        uint32_t handle = 0;
        current_skill = 1;
        Receipt plain = run(1, 0, 0, flags, 0, 0, 100, 0, 0, 0, 0, 1);
        Receipt plain_critical = critical_case(1, 0, 0, flags, 100, 1, 1, 0);
        assert(p4_game_hit_register(&desc, &handle) == P4_DAMAGE_OK);
        const double factors[] = {1.0, 0.0, 3.0};
        for (size_t f = 0; f < sizeof factors / sizeof factors[0]; ++f) {
            hit_factor = factors[f]; hit_calls = 0;
            Receipt staged = run(1, 0, 0, flags, 0, 0, 100, 0, 0, 0, 0, 1);
            assert(hit_calls == 1 && hit_seen.roll == P4_HIT_ACCURACY && hit_seen.stock == 75 &&
                hit_seen.skill_id == 1 && hit_seen.source_side == P4_DAMAGE_SIDE_PARTY && hit_seen.source_id == 1 &&
                hit_seen.target_side == P4_DAMAGE_SIDE_PARTY && hit_seen.target_id == 2);
            assert(staged.rng == plain.rng && staged.calls == plain.calls);
            assert(staged.result == (f == 0 ? plain.result : f == 1 ? 4 : 1));
            if (f == 0) assert(staged.trace == plain.trace);
            hit_calls = 0;
            Receipt critical = critical_case(1, 0, 0, flags, 100, 1, 1, 0);
            assert(hit_calls == 1 && hit_seen.roll == P4_HIT_CRITICAL && hit_seen.stock == 35 &&
                hit_seen.target_side == P4_DAMAGE_SIDE_ENEMY);
            assert(critical.rng == plain_critical.rng && critical.calls == plain_critical.calls);
            assert(critical.result == (f == 0 ? plain_critical.result : f == 1 ? 1 : 2));
        }
        assert(p4_game_hit_unregister(handle) == P4_DAMAGE_OK && !p4_game_hit_native_active());
    }
    puts("game_elements_hit: actual hit, critical, applicability, status, chance-stage and hit-stage retail/tagged probes passed; not native gameplay qualification");
    return 0;
}
