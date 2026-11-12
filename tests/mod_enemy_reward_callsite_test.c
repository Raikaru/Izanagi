/* Synthetic records around the real generated defeat and skill-roll bodies.
 * The hook dispatcher and generated calculator bridges are production code.
 * A native scaling callback exercises the same scope predicate as the Luau
 * binding; calculator values, game records and other services are synthetic. */
#include "type.h"
#include "btl_action_internal.h"
#include "btl_effect_internal.h"
#include "datPersona_internal.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mods.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern u16 mod_test_enemy_exp(unsigned char *);
extern u16 mod_test_enemy_yen(unsigned char *);
extern void btlTargetRecordDefeatReward(u8 *);
extern s32 datCalcSkillRoll(s32, DatUnit *, DatUnit *, s32);

static BtlBattleState battle;
static DatUnit hero;
static u8 skill[0x28];
u8 *D_0076449C = (u8 *)&battle;
u8 *iGpffffb3b8 = skill;
u8 D_0060AB08[1], D_00635938[1];
P4ModEntry p4_mod_entry_001f2cc0, p4_mod_entry_002384b0;
void p4_mod_guard_001f2cc0(void *arg) { (void)arg; abort(); }
s32 p4_mod_guard_002384b0(s32 a, void *b, void *c, s32 d)
{ (void)a; (void)b; (void)c; (void)d; abort(); }
const P4PackageInfo *p4_packages_info(size_t owner) { (void)owner; return NULL; }
_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{ fprintf(stderr, "unexpected hook failure %zu: %s\n", owner, reason); abort(); }
u32 p4_game_enemies_native_limit(void) { return 0x150; }
void kAssertFail(void *file, s32 line) { (void)file; fprintf(stderr, "game assertion %d\n", line); abort(); }
u16 datCalcEnemyExp(s32 unit) { return mod_test_enemy_exp((u8 *)unit); }
u16 datCalcEnemyMoney(s32 unit) { return mod_test_enemy_yen((u8 *)unit); }
u16 datCalcPickWeightedSkill(u8 *unit, s32 *mode) { (void)unit; *mode = 1; return 0; }
u32 datCalcRandomBelow(u32 bound) { (void)bound; return 0; }
u32 RpRandom(void) { return 10; }
u8 *datUnitGetRecord(s32 id) { if (id != 1) abort(); return (u8 *)&hero; }
u8 datCalcGetBattleStat(DatUnit *unit, s32 stat) { (void)unit; (void)stat; return 5; }
u32 datGetMoney(void) { return 0; }
DatPersonaRecord *datPersonaGetRecord(u16 id) { (void)id; abort(); }
u8 datPersonaGetLevel(s32 record) { (void)record; abort(); }

static unsigned exp_calls, yen_calls, nested_calls;
static DatUnit *nested_target;
static int scale(P4ModCall *call, void *user)
{
    if (!p4_mod_reward_active()) return 0;
    if (user) ++yen_calls; else ++exp_calls;
    call->result->u *= 2;
    return 0;
}
static int nested(P4ModCall *call, void *user)
{
    (void)call; (void)user;
    ++nested_calls;
    /* Called inside the real defeat EXP dispatch. Its nested yen call belongs
     * to the skill, not to the outer defeat's scope. */
    if (datCalcSkillRoll(0, nested_target, nested_target, 0) != 7) abort();
    return 0;
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "reward callsite: line %d: %s\n", __LINE__, #x); return 1; } } while (0)
int main(void)
{
    DatUnit enemies[2] = {{0}};
    BtlUnit units[2] = {{0}};
    BtlAction actions[2] = {{0}};
    hero.id = 1; hero.level = 5; skill[0x24] = 6;
    for (unsigned i = 0; i < 2; ++i) {
        enemies[i].flags = 4; enemies[i].id = (u16)(42 + i);
        units[i].genus = 1; units[i].charId = enemies[i].id; units[i].datUnit = &enemies[i];
        actions[i].unk_1a = 1; actions[i].unit = &units[i];
    }
    nested_target = &enemies[0];
    P4ModFunction *exp = p4_mod_function_find(0x243950, "Hp");
    P4ModFunction *yen = p4_mod_function_find(0x2439c0, "Hp");
    CHECK(exp && yen);
    P4ModHook *exp_hook = p4_mod_hook_add(0, exp, P4_MOD_AFTER, scale, NULL);
    P4ModHook *yen_hook = p4_mod_hook_add(0, yen, P4_MOD_AFTER, scale, (void *)1);
    P4ModHook *nested_hook = p4_mod_hook_add(0, exp, P4_MOD_BEFORE, nested, NULL);
    CHECK(exp_hook && yen_hook && nested_hook);
    CHECK(datCalcSkillRoll(0, &enemies[0], &enemies[0], 0) == 7);
    CHECK(exp_calls == 0 && yen_calls == 0);
    btlTargetRecordDefeatReward((u8 *)&actions[0]);
    btlTargetRecordDefeatReward((u8 *)&actions[1]);
    CHECK(battle.rewardExp == 20 && battle.rewardMoney == 28);
    CHECK(exp_calls == 2 && yen_calls == 2 && nested_calls == 2);
    btlTargetRecordDefeatReward((u8 *)&actions[0]);
    CHECK(battle.rewardExp == 20 && battle.rewardMoney == 28 && nested_calls == 2);
    CHECK(datCalcSkillRoll(0, &enemies[1], &enemies[1], 0) == 7);
    CHECK(exp_calls == 2 && yen_calls == 2);
    CHECK(p4_mod_hook_enable(exp_hook, 0) == 0 && p4_mod_hook_enable(yen_hook, 0) == 0);
    actions[0].unk_1a = 1;
    btlTargetRecordDefeatReward((u8 *)&actions[0]);
    CHECK(battle.rewardExp == 25 && battle.rewardMoney == 35 && nested_calls == 3);
    CHECK(exp_calls == 2 && yen_calls == 2);
    CHECK(p4_mod_hook_remove(exp_hook) == 0 && p4_mod_hook_remove(yen_hook) == 0 && p4_mod_hook_remove(nested_hook) == 0);
    p4_mod_hooks_clear();
    puts("real defeat caller and kind-six skill roll: scope, nested, repeated and disabled passed");
    return 0;
}
