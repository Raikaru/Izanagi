/* ACTUAL production amount/term/formula overrides, not reimplemented math.
 * Engine queries, source transport and RNG are explicit synthetic probes.
 * This is an equation/state/order witness, NOT native battle/UI admission. */
#include <assert.h>
#include <math.h>
#include <setjmp.h>
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
 typedef uint32_t u32; typedef int32_t s32; typedef uint64_t u64; typedef int64_t s64; typedef float f32;
#include "p4pc/game_elements_calc_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_tables.h"
#include "p4pc/game_damage_stage.h"
#include "datScript_internal.h"
#include "dat_skill_entry_internal.h"
#include "dat_enemy_internal.h"
#include "operand_order_internal.h"
/* Exact scalar views consumed by these equations, pinned in datUnit_internal.h,
 * datCalc.c and btl_effect_internal.h; no host-pointer-bearing guest layout. */
typedef struct DatUnit {
    u16 flags, id, id2; u8 level, pad07; u16 hp, sp; u32 bad; s8 tactic;
    u8 unknown11[0x1b]; u16 message; u8 pad2e[2];
} DatUnit;
typedef struct DatCalcUnitState {
    u8 pad00[0x14]; u32 buffFlags, consumedFlags; u8 rest[0x14];
} DatCalcUnitState;
typedef struct BtlBattleState {
    u8 pad00[0xc58]; u16 partyActionCount, padc5a, padc5c, escapeCount;
} BtlBattleState;
_Static_assert(sizeof(DatUnit) == 0x30 && offsetof(DatUnit, hp) == 8 && offsetof(DatUnit, bad) == 12, "actual unit fields");
_Static_assert(offsetof(DatCalcUnitState, consumedFlags) == 0x18 && sizeof(DatCalcUnitState) == 0x30, "actual consumed flags");
_Static_assert(sizeof(DatSkillEntry) == 40 && sizeof(DatEnemyUnitInit) == 60, "actual source row strides");
#define MEMBER_OFFSET(T, M) ((s32)offsetof(T, M))
#define PERSONA_STAT_MAG 1
static u8 *byteAddressAdd(u32 offset, u8 *base) { return base + offset; }
static u16 *D_00881460[1], *D_00881464[1], D_0088146C[1], D_00881474[1];
static s32 D_00881468[1], D_00881470[1];
static u8 *iGpffffb3b8, *iGpffffb3c4, *DAT_007644A8, *DAT_0076449c;
static f32 level_factors[190], level_steps[32];
static s16 tier_factors[10], extra_factors[21];
static u8 *DAT_007644E4 = (u8 *)level_factors, *DAT_007644E8 = (u8 *)level_steps;
static u8 *DAT_007644F0 = (u8 *)tier_factors, *DAT_007644F4 = (u8 *)extra_factors;
static f32 D_00761260 = 1.25f, fGpffff811c = 0.75f, fGpffff824c = 2.0f, fGpffff82c0 = 0.5f;
f32 iGpffff8170 = 0.7f, fGpffff82c4 = 1.2f, fGpffff82c8 = 1.4f, fGpffff80bc = 1.6f;
f32 fGpffff82cc = 0.8f, fGpffff809c = 0.6f;
static u8 D_00635938[1], D_00635980[1];
static unsigned enabled, rng_calls, primary_calls, rider_calls, skill_uses, actual_affinities, provided_affinities;
static u32 rng_state, legacy_leaf, actual_leaf, actual_family, actual_flags, source_word, target_word;
static unsigned traits, source_reads, target_reads;
static DatUnit *source_unit, *target_unit;
static jmp_buf failure;
static int expecting_failure;
static u32 failed_line;
static void kAssertFail(const void *file, s32 line)
{
    (void)file; assert(expecting_failure); failed_line = (u32)line; longjmp(failure, 1);
}
#define K_ASSERT(C, L) do { if (!(C)) kAssertFail(D_00635980, L); } while (0)
_Noreturn void p4_game_elements_native_calculation_refuse(int32_t result)
{ (void)result; kAssertFail(D_00635980, 0xf000); abort(); }
int p4_game_elements_native_enabled(void) { return (int)enabled; }
int p4_game_skill_registered(uint32_t id) { return id < 576 || id == 1024; }
int p4_game_skill_active(uint32_t id) { return id < 440 || id == 1024; }
/* Unexpanded enemy banks: the retail 0x150 domain. */
uint32_t p4_game_enemies_native_limit(void) { return 0x150u; }
void p4_game_elements_native_skill_use(uint32_t id) { assert(p4_game_skill_active(id)); ++skill_uses; }
static P4GameActionFrame action_frame(u32 skill, u32 component, u32 options)
{
    return (P4GameActionFrame){.element = {1, actual_leaf, component == P4_ACTION_COMPONENT_INSTANT_KO ? 1u :
        component == P4_ACTION_COMPONENT_STATUS ? 2u : 0u, actual_flags, actual_family, component},
        .skill_id = skill, .source_kind = P4_ACTION_ELEMENT_SKILL, .source_id = skill, .native_options = options};
}
void p4_game_elements_native_primary_frame(void *unit, uint32_t skill, uint32_t options, P4GameActionFrame *out)
{
    assert(unit == source_unit); ++primary_calls; p4_game_elements_native_skill_use(skill);
    *out = action_frame(skill, P4_ACTION_COMPONENT_DAMAGE, options);
}
void p4_game_elements_native_action_frame(void *unit, uint32_t skill, uint32_t component, uint32_t payload,
    uint32_t options, P4GameActionFrame *out)
{
    assert(unit == source_unit && payload); ++rider_calls; p4_game_elements_native_skill_use(skill);
    *out = action_frame(skill, component, options);
}
uint32_t p4_game_elements_native_affinity(void *unit, const P4GameActionFrame *frame)
{
    assert(frame && frame->element.element_id == actual_leaf); ++actual_affinities;
    if (unit == source_unit) { ++source_reads; return source_word; }
    assert(unit == target_unit); ++target_reads; return target_word;
}
uint32_t p4_game_elements_native_affinity_query(void *unit, const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{
    assert(unit == target_unit);
    if (query && query->mode == P4_AFFINITY_QUERY_PROVIDED) { ++provided_affinities; return query->value; }
    return p4_game_elements_native_affinity(unit, frame);
}
static u32 RpRandom(void) { ++rng_calls; rng_state = rng_state * 1664525u + 1013904223u; return rng_state; }
static DatUnit *as_unit(const void *unit) { assert(unit == source_unit || unit == target_unit); return (DatUnit *)unit; }
static s32 datCalcGetLevel(const void *unit) { return as_unit(unit)->level; }
static s32 datCalcGetHp(const void *unit) { return as_unit(unit)->hp; }
static u16 datCalcMaxHp(const void *unit) { return (u16)(500 + as_unit(unit)->level); }
static u16 datCalcMaxSp(const void *unit) { return (u16)(200 + as_unit(unit)->level); }
static s32 datCalcGetBattleStat(const void *unit, u32 index) { return (s32)(30 + as_unit(unit)->level + index * 3); }
static s32 datCalcGetWeaponProperty(const void *unit, s32 kind) { return (s32)(40 + as_unit(unit)->level + kind * 5); }
static s32 datCalcGetArmorProperty(const void *unit, s32 kind) { return (s32)(20 + as_unit(unit)->level + kind * 4); }
static s32 datCalcEnemyHasTableFlag(const void *unit) { return (as_unit(unit)->flags & 4u) && (traits & 1u); }
static s32 datCalcSkillElement(void *unit, u16 skill) { (void)unit; assert(p4_game_skill_registered(skill)); return (s32)legacy_leaf; }
static s32 datCalcStatusElement(s16 skill, s32 mask) { (void)skill; return mask == 0x80000 ? (legacy_leaf == 6 ? 6 : 7) : 9; }
static s32 datCalcGetAffinity(u8 *unit, s32 leaf)
{
    assert(leaf >= 0 && leaf < 19);
    if ((void *)unit == source_unit) { ++source_reads; return (s32)source_word; }
    assert((void *)unit == target_unit); ++target_reads; return (s32)target_word;
}
static s32 datCalcElementBit(s16 leaf) { assert(leaf >= -1 && leaf < 19); return leaf == 0 ? 2 : 0; }
static f32 datCalcBuffMultiplier(u8 *unit, s32 kind, u16 skill) { (void)skill; as_unit(unit); return traits & 2u ? 1.0f + kind * 0.1f : 1.0f; }
static s32 datCalcAccessoryProperty(DatUnit *unit) { return traits & 4u ? (unit == source_unit ? 0xbd : 0xc1) : 0; }
static f32 datCalcAccessoryFraction(DatUnit *unit) { as_unit(unit); return 0.1f; }
static s32 datCalcHasRequestedSkill(DatUnit *unit, s32 skill) { as_unit(unit); return (traits & 8u) && !(skill & 1); }
static s32 datCalcCountEquipmentProperty(u8 *unit, s32 property) { as_unit(unit); return (traits & 16u) && !(property & 1); }
static s32 datCalcHasEquipmentProperty(u8 *unit, s32 property) { as_unit(unit); return (traits & 32u) && (property & 1); }
static s32 datGetFlag(s32 flag) { return (traits & 64u) && (flag & 1); }
static s32 datCalcIsGuarding(u16 *unit) { return unit == (u16 *)target_unit && (traits & 128u); }
void datScriptSetResult(s32 value) { D_00881474[0] |= 1; D_00881468[0] = value; }
static s32 datScriptTargetIsNotEnemy(void) { K_ASSERT(D_00881460[0], 0x1fd); return !(*D_00881460[0] & 4u); }
static s32 datScriptCmdFlag2Set(void) { assert(p4_game_skill_active(D_0088146C[0])); return DAT_007644A8[D_0088146C[0] * 40] & 2u; }
#include "../overrides/00245030.c"
#include "../overrides/00244110.c"
#include "../overrides/00244540.c"
#include "../overrides/002447c0.c"
#include "../overrides/00244870.c"
#include "../overrides/002449c0.c"
#include "../overrides/00244ad0.c"
#include "../overrides/00244b10.c"
#include "../overrides/00244c10.c"
#include "../overrides/00244cf0.c"
#include "../overrides/00244e20.c"
static void datCalcFormulaNoop(void) { }
void (*D_00635950[11])(void) = {datCalcFormulaNoop, datScriptCalcPhysicalDamage, datScriptCalcMagicDamage,
    datCalcFormulaTerms13And1A, datScriptCalcHitRate, datCalcFormulaScaledRatio, datCalcFormulaPassCalc15,
    datCalcFormulaWeightedRatio, datCalcFormulaInverseRatio, datScriptCalcPhysicalDamageScaled, datCalcFormulaRatioOfProducts};
#include "../overrides/00244f60.c"
#include "../overrides/00235520.c"

typedef struct Receipt {
    s32 amount, result, parameter; u16 command, flags;
    u32 rng, calls, source_reads, target_reads;
    DatUnit source, target;
} Receipt;
static DatUnit initial_source, initial_target;
static void reset(u32 skill, u32 is_enabled)
{
    *source_unit = initial_source; *target_unit = initial_target;
    enabled = is_enabled; rng_state = 0x12345678u; rng_calls = primary_calls = rider_calls = skill_uses = 0;
    actual_affinities = provided_affinities = source_reads = target_reads = 0;
    D_0088146C[0] = 33; D_00881474[0] = 0xfaceu; D_00881468[0] = 1234; D_00881470[0] = -99;
    D_00881460[0] = (u16 *)target_unit; D_00881464[0] = (u16 *)source_unit;
    assert(p4_game_skill_active(skill));
}
static Receipt receipt(s32 amount)
{
    return (Receipt){amount, D_00881468[0], D_00881470[0], D_0088146C[0], D_00881474[0],
        rng_state, rng_calls, source_reads, target_reads, *source_unit, *target_unit};
}
static void same(const Receipt *left, const Receipt *right)
{
    assert(left->amount == right->amount && left->result == right->result && left->parameter == right->parameter);
    assert(left->command == right->command && left->flags == right->flags && left->rng == right->rng && left->calls == right->calls);
    assert(left->source_reads == right->source_reads && left->target_reads == right->target_reads);
    assert(!memcmp(&left->source, &right->source, sizeof left->source) && !memcmp(&left->target, &right->target, sizeof left->target));
}
static void amount_case(u32 skill, u32 channel, u32 outcome, u32 options)
{
    reset(skill, 0);
    const Receipt original = receipt(datCalcSkillAmount((u16)skill, (u8 *)source_unit, (u8 *)target_unit, 3, (u16)outcome, 2, options, (u8)channel));
    assert(!primary_calls && !rider_calls && !skill_uses && !actual_affinities);
    reset(skill, 1);
    P4GameActionFrame frame = action_frame(skill, P4_ACTION_COMPONENT_DAMAGE, options);
    const Receipt full = receipt(p4_game_elements_native_amount(skill, source_unit, target_unit, 3, outcome, 2, options, channel, &frame, NULL));
    same(&original, &full);
    assert(!primary_calls && !rider_calls); /* Caller frame is never recomputed. */
    reset(skill, 1);
    const Receipt direct = receipt(datCalcSkillAmount((u16)skill, (u8 *)source_unit, (u8 *)target_unit, 3, (u16)outcome, 2, options, (u8)channel));
    same(&original, &direct);
    assert(primary_calls <= 1 && !rider_calls);
}
/* Final-damage stage over the real equation (game_damage_stage.c linked as
 * built): no registered modifier is retail; a consulted modifier that leaves
 * the hit alone changes nothing; challenge + hard-mode modifiers compose. */
static unsigned stage_consulted;
static P4GameDamageHit stage_seen;
static int stage_decline(void *user, const P4GameDamageHit *hit, double *value)
{
    (void)user; (void)value; ++stage_consulted; stage_seen = *hit; return 0;
}
/* SEVERE DAMAGE: opposite sides by the unit the hit was aimed at. */
static int stage_severe(void *user, const P4GameDamageHit *hit, double *value)
{
    (void)user;
    if (hit->source_side == hit->origin_side) return 0;
    double factor = hit->source_side == P4_DAMAGE_SIDE_ENEMY ? 1.5 : 0.75;
    if (hit->hit_kind == P4_DAMAGE_HIT_CRITICAL || hit->hit_kind == P4_DAMAGE_HIT_WEAK) factor *= 1.25;
    *value = factor;
    return 1;
}
static int stage_hard_scale(void *user, const P4GameDamageHit *hit, double *value)
{
    (void)user;
    if (hit->target_side != P4_DAMAGE_SIDE_PARTY) return 0;
    *value = 1.2;
    return 1;
}
static int stage_hard_floor(void *user, const P4GameDamageHit *hit, double *value)
{
    (void)user; (void)hit; *value = 3.0; return 1;
}
static Receipt stage_run(u8 *target, u32 channel, u32 outcome, u32 hit_kind, u32 options)
{
    reset(10, 0);
    return receipt(datCalcSkillAmount(10, (u8 *)source_unit, target, 3, (u16)outcome, (u16)hit_kind, (s32)options, (u8)channel));
}
static void stage_parity(DatSkillEntry *row)
{
    static const u8 effects[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    static const u32 outcomes[] = {1, 0x400, 2}, kinds[] = {1, 2, 4, 16};
    const P4GameDamageModifierDesc decline = {"test.decline", 0, 0, P4_DAMAGE_MULTIPLY, stage_decline, NULL};
    unsigned eligible_total = 0;
    assert(!p4_game_damage_native_active());
    for (unsigned i = 0; i < sizeof effects; ++i) {
        row->effectKind = row->spEffectKind = effects[i];
        row->damageClass = (i & 1) ? 1 : 2;
        for (u32 channel = 1; channel <= 2; ++channel)
            for (unsigned o = 0; o < 3; ++o)
                for (unsigned k = 0; k < 4; ++k)
                    for (u32 options = 0; options < 2; ++options) {
                        const Receipt plain = stage_run((u8 *)target_unit, channel, outcomes[o], kinds[k], options);
                        uint32_t handle = 0;
                        assert(p4_game_damage_register(&decline, &handle) == P4_DAMAGE_OK);
                        stage_consulted = 0;
                        const Receipt observed = stage_run((u8 *)target_unit, channel, outcomes[o], kinds[k], options);
                        assert(p4_game_damage_unregister(handle) == P4_DAMAGE_OK);
                        same(&plain, &observed);
                        const unsigned eligible = channel == 1 && outcomes[o] == 1 && !options && kinds[k] != 16 &&
                            (effects[i] == 1 || effects[i] == 6 || effects[i] == 12 || effects[i] == 14 || effects[i] == 16);
                        assert(stage_consulted == eligible);
                        if (eligible)
                            assert(stage_seen.stock == -plain.amount && stage_seen.hit_kind == kinds[k] &&
                                stage_seen.skill_id == 10 && !stage_seen.resolved && !stage_seen.reflected &&
                                stage_seen.element_valid && stage_seen.element_id == legacy_leaf &&
                                stage_seen.source_status == initial_source.bad && stage_seen.target_status == initial_target.bad);
                        eligible_total += eligible;
                    }
    }
    assert(eligible_total == 15 && !p4_game_damage_native_active());
}
static void stage_expect(const Receipt *plain, const Receipt *observed, double hard, double severe)
{
    double value = (double)-plain->amount;
    if (hard != 0.0) value *= hard;
    if (severe != 0.0) value *= severe;
    value += 3.0;
    const s32 expected = value < 1.0 ? 1 : value > 32550.0 ? 32550 : (s32)value;
    assert(plain->amount < 0 && observed->amount == -expected);
    Receipt rest = *observed; rest.amount = plain->amount;
    same(plain, &rest); /* the modifiers draw no RNG and touch no unit */
}
static void stage_composition(DatSkillEntry *row)
{
    row->effectKind = row->spEffectKind = 1; row->damageClass = 1;
    initial_source.flags = 0; initial_target.flags = 4;
    const Receipt critical = stage_run((u8 *)target_unit, 1, 1, P4_DAMAGE_HIT_CRITICAL, 0);
    const Receipt normal = stage_run((u8 *)target_unit, 1, 1, P4_DAMAGE_HIT_NORMAL, 0);
    p4_game_damage_native_hit_begin(10, 1, 2, 1); /* repelled by enemy 2 back to its party source */
    const Receipt reflected = stage_run((u8 *)source_unit, 1, 1, P4_DAMAGE_HIT_NORMAL, 0);
    p4_game_damage_native_hit_end();
    /* The ailment words are the ones this equation read for the hit. */
    initial_source.flags = 4; initial_target.flags = 0;
    initial_source.bad = 0x2u; initial_target.bad = 0x100000u;
    const Receipt weak = stage_run((u8 *)target_unit, 1, 1, P4_DAMAGE_HIT_WEAK, 0);
    /* Registered out of order: hard mode's factor (load 1) runs before the
     * challenge factor (load 2), and its additive floor after both. */
    const P4GameDamageModifierDesc severe = {"challenge.severe", 0, 2, P4_DAMAGE_MULTIPLY, stage_severe, NULL};
    const P4GameDamageModifierDesc scale = {"hardmode.scale", 0, 1, P4_DAMAGE_MULTIPLY, stage_hard_scale, NULL};
    const P4GameDamageModifierDesc floor_add = {"hardmode.floor", 0, 1, P4_DAMAGE_ADD, stage_hard_floor, NULL};
    const P4GameDamageModifierDesc observe = {"test.observe", 1, 0, P4_DAMAGE_MULTIPLY, stage_decline, NULL};
    uint32_t handles[4];
    assert(p4_game_damage_register(&floor_add, &handles[0]) == P4_DAMAGE_OK);
    assert(p4_game_damage_register(&severe, &handles[1]) == P4_DAMAGE_OK);
    assert(p4_game_damage_register(&scale, &handles[2]) == P4_DAMAGE_OK);
    assert(p4_game_damage_register(&observe, &handles[3]) == P4_DAMAGE_OK);
    Receipt observed = stage_run((u8 *)target_unit, 1, 1, P4_DAMAGE_HIT_WEAK, 0);
    stage_expect(&weak, &observed, 1.2, 1.875);
    assert(stage_seen.source_status == 0x2u && stage_seen.target_status == 0x100000u);
    initial_source.bad = initial_target.bad = 0;
    initial_source.flags = 0; initial_target.flags = 4;
    observed = stage_run((u8 *)target_unit, 1, 1, P4_DAMAGE_HIT_CRITICAL, 0);
    stage_expect(&critical, &observed, 0.0, 0.9375);
    observed = stage_run((u8 *)target_unit, 1, 1, P4_DAMAGE_HIT_NORMAL, 0);
    stage_expect(&normal, &observed, 0.0, 0.75);
    p4_game_damage_native_hit_begin(10, 1, 2, 1);
    observed = stage_run((u8 *)source_unit, 1, 1, P4_DAMAGE_HIT_NORMAL, 0);
    p4_game_damage_native_hit_end();
    stage_expect(&reflected, &observed, 1.2, 0.75); /* scaled once, by the side it was aimed at */
    /* An estimate of the same self-delivered hit is same-side: hard mode only. */
    observed = stage_run((u8 *)source_unit, 1, 1, P4_DAMAGE_HIT_NORMAL, 0);
    stage_expect(&reflected, &observed, 1.2, 0.0);
    for (unsigned i = 0; i < 4; ++i) assert(p4_game_damage_unregister(handles[i]) == P4_DAMAGE_OK);
    initial_source.flags = initial_target.flags = 0;
}
static void formulas(void)
{
    for (u32 enemy = 0; enemy < 2; ++enemy) {
        initial_source.flags = enemy ? 4 : 0;
        for (u32 kind = 1; kind < 11; ++kind) {
            const s32 parameter = kind == 8 || kind == 10 ? 0x80000 : 1;
            reset(10, 0);
            const Receipt baseline = receipt(datScriptRunCalc(kind, (u16 *)source_unit, (u16 *)target_unit, 10, parameter));
            reset(10, 1);
            const Receipt actual = receipt(datScriptRunCalc(kind, (u16 *)source_unit, (u16 *)target_unit, 10, parameter));
            same(&baseline, &actual);
            assert(primary_calls == (kind == 1 || kind == 2 || kind == 9));
            assert(rider_calls == (kind == 8 || kind == 10));
        }
    }
    initial_source.flags = 0;
}
static int initialize(void)
{
    const size_t size = 0x20000;
#ifdef _WIN32
    u8 *arena = VirtualAlloc((void *)(uintptr_t)0x20000000u, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    u8 *arena = mmap((void *)(uintptr_t)0x20000000u, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) arena = NULL;
#endif
    if (!arena || (uintptr_t)arena > UINT32_MAX - size) return 77;
    iGpffffb3b8 = DAT_007644A8 = arena; iGpffffb3c4 = arena + 0x10000; DAT_0076449c = arena + 0x18000;
    assert(((uintptr_t)iGpffffb3b8 & 15u) == 0);
    for (unsigned i = 0; i < 190; ++i) level_factors[i] = 1.0f + i * 0.001f;
    for (unsigned i = 0; i < 32; ++i) level_steps[i] = 1.0f;
    for (unsigned i = 0; i < 10; ++i) tier_factors[i] = (s16)(i + 2);
    for (unsigned i = 0; i < 21; ++i) extra_factors[i] = (s16)(i + 3);
    ((BtlBattleState *)DAT_0076449c)->partyActionCount = 2;
    ((BtlBattleState *)DAT_0076449c)->escapeCount = 3;
    initial_source = (DatUnit){.id = 1, .level = 35, .hp = 250, .sp = 80};
    initial_target = (DatUnit){.id = 2, .level = 30, .hp = 240, .sp = 70};
    static DatUnit source, target; source_unit = &source; target_unit = &target;
    source_word = 100; target_word = 125; actual_flags = P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY;
    return 0;
}
int main(void)
{
    int result = initialize(); if (result) return result;
    DatSkillEntry *row = (DatSkillEntry *)(iGpffffb3b8 + 10 * 40);
    row->power = 43; row->spPower = 17; row->accuracy = 90; row->field19 = 60; row->critRate = 25; row->damageClass = 1;
    static const u8 effects[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    for (u32 leaf = 0; leaf < 5; ++leaf) {
        legacy_leaf = actual_leaf = actual_family = leaf;
        actual_flags = P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY | (leaf ? 0 : P4_ELEMENT_PHYSICAL);
        for (unsigned modifier = 0; modifier < 2; ++modifier) {
            traits = modifier ? 255u : 0;
            initial_source.bad = modifier ? 0x22 : 0; initial_target.bad = modifier ? 0x100040 : 0;
            for (unsigned i = 0; i < sizeof effects; ++i) {
                row->effectKind = row->spEffectKind = effects[i];
                row->damageClass = (i & 1) ? 1 : 2;
                for (u32 channel = 1; channel <= 2; ++channel) {
                    amount_case(10, channel, 1, 0); amount_case(10, channel, 0x400, 0); amount_case(10, channel, 2, 1);
                }
            }
        }
    }
    traits = 0; initial_source.bad = initial_target.bad = 0;
    legacy_leaf = 0; actual_leaf = UINT32_MAX; actual_family = 0;
    actual_flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY;
    row->damageClass = 1; row->effectKind = row->spEffectKind = 1;
    amount_case(10, 1, 1, 0); /* SemanticMAX never becomes a packed legacy element. */
    legacy_leaf = 1; actual_leaf = 65536; actual_family = 1; actual_flags = P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY;
    row->damageClass = 2; amount_case(10, 1, 1, 0);
    legacy_leaf = actual_leaf = actual_family = 6;
    row->effectKind = 1; row->damageClass = 2; formulas();
    reset(10, 1);
    P4GameActionFrame frame = action_frame(10, P4_ACTION_COMPONENT_DAMAGE, 0);
    P4GameAffinityQuery neutral = {P4_AFFINITY_QUERY_PROVIDED, 100};
    const s32 neutral_amount = p4_game_elements_native_amount(10, source_unit, target_unit, 1, 1, 1, 1, 1, &frame, &neutral);
    assert(provided_affinities == 1 && !actual_affinities && !target_reads && !primary_calls);
    reset(10, 1); target_word = 100;
    assert(p4_game_elements_native_amount(10, source_unit, target_unit, 1, 1, 1, 1, 1, &frame, NULL) == neutral_amount);
    D_00881460[0] = (u16 *)source_unit; D_00881464[0] = (u16 *)target_unit;
    provided_affinities = actual_affinities = 0;
    assert(p4_game_elements_calculation_float(11, 0, &frame, &neutral) == 1.0f);
    assert(actual_affinities == 1 && !provided_affinities); /* Policy never leaks to source term11. */
    row->effectKind = 5; reset(10, 1);
    assert(datCalcSkillAmount(10, (u8 *)source_unit, (u8 *)target_unit, 1, 1, 1, 0, 1) == row->power);
    assert(!primary_calls && !rider_calls && !actual_affinities && skill_uses == 1); /* Fixed amount never consumes a leaf. */
    memcpy(iGpffffb3b8 + 1024 * 40, row, 40); ((DatSkillEntry *)(iGpffffb3b8 + 1024 * 40))->effectKind = 1;
    amount_case(1024, 1, 1, 0);
    {
        const DatSkillEntry saved = *row;
        stage_parity(row);
        stage_composition(row);
        *row = saved;
    }
    expecting_failure = 1; failed_line = 0;
    if (!setjmp(failure)) { frame.skill_id = 11; p4_game_elements_native_amount(10, source_unit, target_unit, 1, 1, 1, 0, 1, &frame, NULL); assert(0); }
    assert(failed_line == 0xf000);
    reset(10, 0); failed_line = 0;
    if (!setjmp(failure)) { datScriptRunCalc(0, (u16 *)source_unit, (u16 *)target_unit, 10, 1); assert(0); }
    assert(failed_line == 0x82 && !(D_00881474[0] & 1) && D_0088146C[0] == 10);
    puts("game_elements_amount: actual canonical HP/SP equations, ten formulas, state/RNG parity, full-width frames, exact target policy and the final-damage stage passed (engine/source probes; not native admission)");
    return 0;
}
