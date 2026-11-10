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
static u16 listed_skills[8];
static u32 held_count;
static void *expected_action_source;
static uint64_t trace;
static void mark(u32 x) { trace = trace * UINT64_C(1099511628211) ^ x; }
static void kAssertFail(const void *file, s32 line) { (void)file; fprintf(stderr, "Unexpected retail assertion %x\n", (unsigned)line); abort(); }
_Noreturn void p4_game_elements_native_calculation_refuse(int32_t result) { fprintf(stderr, "Unexpected native refusal %d\n", result); abort(); }
int p4_game_elements_native_enabled(void) { return (int)enabled; }
int p4_game_skill_active(uint32_t id) { return id < P4_SKILL_RETAIL_ACTIVE_COUNT || id == 1024; }
int p4_game_skill_registered(uint32_t id) { return id < P4_SKILL_RETAIL_COUNT || id == 1024; }
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
{ assert(unit == &source_unit); mark(7000 + (u32)id); return 0; }
static s32 datItemElement(s16 item) { (void)item; return (s32)attack_element; }
static s32 datGetEquippedItemByUnit(s16 id, s16 slot) { (void)id; (void)slot; return 1; }
static u8 *datPersonaGetSkillsByPcId(s32 id) { assert(id == 1); mark(8000); return (u8 *)listed_skills; }
static s32 datItemGrantedSkill(s16 id) { assert(id == 1); mark(8001); return 0; }
static s32 datItemCount(s32 id) { assert(id == 0x340); mark(8002); return (s32)held_count; }
static s32 cmmCommunityForArcana(s16 id) { (void)id; abort(); }
static s32 cmmCommunityRecordValue(u16 id) { (void)id; abort(); }
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
#include "../overrides/0023e6f0.c"
#include "../overrides/002411a0.c"
#include "../overrides/00241bc0.c"
#include "../overrides/00238940.c"
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
    source_unit.id = 1; source_unit.flags = unit_flags; enabled = route; held_count = items; attack_element = family;
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
    puts("game_elements_hit: actual hit, critical, applicability and status retail/tagged probes passed; not native gameplay qualification");
    return 0;
}
