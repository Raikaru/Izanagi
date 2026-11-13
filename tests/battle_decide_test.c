/* The battle decision stage (src/platform/mods/battle_decide.c) over synthetic
 * guest battle records: snapshot, composition, legality from the collector,
 * the command record it writes and every refusal. The collector, death and
 * maximum-HP natives are controlled stubs; the in-game witness qualifies them. */
#include "guest_test.h"
#include "p4pc/battle_decide.h"
#include "p4pc/packages.h"
#include "p4pc/table_result.h"
#include <assert.h>
#include <setjmp.h>
#include <string.h>

enum { SKILL_SINGLE = 10, SKILL_ALL = 20, SKILL_SELF = 227, SKILL_INACTIVE = 900 };
static uint32_t actions[5], units[5], records[5];
static unsigned carrier_calls, carrier_kind, carrier_skill;
static int carrier_result;

static jmp_buf recovery;
static size_t failed_owner;
_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{
    (void)reason;
    failed_owner = owner;
    longjmp(recovery, 1);
}
int p4_game_skill_active(uint32_t id) { return id < 800; }
const P4PackageInfo *p4_packages_info(size_t ordinal) { (void)ordinal; return NULL; }
uint32_t datCalcIsDead(void *record, int32_t delta) { (void)delta; return P4_G16(P4_H2G(record) + 8) == 0; }
uint16_t datCalcMaxHp(void *record) { (void)record; return 200; }
uint16_t datCalcMaxSp(void *record) { (void)record; return 50; }
int p4_game_elements_ai_native_decide(void *action, uint32_t kind, uint32_t skill)
{
    (void)action;
    ++carrier_calls; carrier_kind = kind; carrier_skill = skill;
    return carrier_result;
}
/* The enemy at index 3 deciding: single-target skills reach the living party
 * (0..2), SKILL_ALL every living party member, SKILL_SELF only the actor. */
uint16_t btlSkillClassifyTargets(void *action, void *selection, uint16_t skill, uint32_t reverse)
{
    (void)reverse;
    const uint32_t out = P4_H2G(selection);
    uint16_t count = 0;
    if (skill == SKILL_SELF) P4_G32(out + 4u * count++) = P4_H2G(action);
    else
        for (unsigned i = 0; i < 3; ++i)
            if (P4_G16(records[i] + 8)) P4_G32(out + 4u * count++) = actions[i];
    P4_G16(out + 0x38) = count;
    return skill == SKILL_ALL;
}

static void build(void)
{
    const uint32_t battle = P4_H2G(p4t_galloc(0x300, 16));
    P4_GPDATA(uint32_t, 0xb3ac) = battle;
    P4_G16(battle + 0x20) = 4;
    for (unsigned i = 0; i < 5; ++i) {
        actions[i] = P4_H2G(p4t_galloc(0x458, 16));
        units[i] = P4_H2G(p4t_galloc(0xa70, 16));
        records[i] = P4_H2G(p4t_galloc(0x20, 16));
        P4_G8(actions[i] + 0x1a) = i == 4 ? 0 : 1; /* the fifth is not present */
        P4_G32(actions[i] + 0x30) = units[i];
        P4_G8(units[i] + 0xa2) = i < 3 ? 0 : 1;
        P4_G16(units[i] + 0xa4) = i < 3 ? (uint16_t)(i + 1) : 0x2b0;
        P4_G32(units[i] + 0xa8) = 100 + i;
        P4_G32(units[i] + 0xa64) = records[i];
        P4_G16(records[i] + 8) = (uint16_t)(50 + 10 * i);
        P4_G16(records[i] + 10) = 20;
        if (i) P4_G32(actions[i - 1] + 0x450) = actions[i];
    }
    P4_G32(battle + 0x174) = actions[0];
    P4_G32(records[3] + 0x14) = 0x40000u; /* charged */
}
/* The enemy's own choice: SKILL_SINGLE at party slot 1. */
static void native_pick(void)
{
    const uint32_t a = actions[3];
    P4_G16(a + 0x6c) = 2;
    P4_G16(a + 0x6e) = SKILL_SINGLE;
    P4_G32(a + 0x38) = actions[1];
    P4_G16(a + 0x6a) = 1;
    P4_G8(a + 0x28) = 1;
    carrier_calls = 0;
    carrier_result = P4_TABLE_OK;
}

typedef struct Script { P4BattleDecision decision; unsigned calls, applied; P4BattleDecideState seen; } Script;
static int scripted(const P4BattleDecideState *state, P4BattleDecision *decision, void *user)
{
    Script *script = user;
    ++script->calls;
    script->seen = *state;
    *decision = script->decision;
    return 0;
}
static void applied(void *user) { ++((Script *)user)->applied; }
static int failing(const P4BattleDecideState *state, P4BattleDecision *decision, void *user)
{
    (void)state; (void)decision; ++*(unsigned *)user; return -1;
}

static void snapshot_and_keep(void)
{
    native_pick();
    Script keep = {0};
    P4BattleDecider *d = p4_battle_decider_add(7, 0, scripted, applied, &keep);
    assert(d && p4_battle_decide_interest);
    p4_battle_decide_ready(actions[3]);
    assert(keep.calls == 1 && !keep.applied && !carrier_calls);
    const P4BattleDecideState *s = &keep.seen;
    assert(s->count == 4 && s->actor == 3 && s->round == 4 && s->one_more == 1 && s->extra == 0);
    assert(s->pick == P4_BATTLE_PICK_SKILL && s->skill == SKILL_SINGLE && s->target_count == 1 && s->targets[0] == 1);
    assert(s->units[3].side == 1 && s->units[3].id == 0x2b0 && s->units[3].charged && s->units[3].hp_max == 200);
    assert(s->units[0].side == 0 && s->units[0].id == 1 && s->units[0].alive && s->units[0].hp == 50 && s->units[0].uid == 100);
    uint8_t indices[12]; uint32_t count; int all;
    assert(!p4_battle_decide_candidates(s, SKILL_ALL, indices, &count, &all) && count == 3 && all);
    assert(p4_battle_decide_candidates(s, SKILL_INACTIVE, indices, &count, &all) && !count);
    /* Party actors are not decided by battle.decide@1. */
    p4_battle_decide_ready(actions[0]);
    assert(keep.calls == 1);
    assert(!p4_battle_decider_remove(d));
    p4_battle_deciders_remove_owner(7);
    assert(!p4_battle_decide_interest);
}
static void retarget_substitute_and_pass(void)
{
    native_pick();
    Script first = {.decision = {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_SINGLE, .has_target = 1, .target = 2}};
    Script second = {0};
    p4_battle_decider_add(1, 5, scripted, applied, &second);
    p4_battle_decider_add(2, -5, scripted, applied, &first); /* lower priority first */
    p4_battle_decide_ready(actions[3]);
    assert(first.applied == 1 && carrier_calls == 1 && carrier_kind == 2 && carrier_skill == SKILL_SINGLE);
    assert(P4_G16(actions[3] + 0x6a) == 1 && P4_G32(actions[3] + 0x38) == actions[2]);
    /* The later decider sees the earlier one's choice. */
    assert(second.calls == 1 && second.seen.targets[0] == 2);
    p4_battle_deciders_remove_owner(1);
    p4_battle_deciders_remove_owner(2);

    native_pick();
    Script all = {.decision = {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_ALL}};
    p4_battle_decider_add(3, 0, scripted, applied, &all);
    p4_battle_decide_ready(actions[3]);
    assert(all.applied == 1 && P4_G16(actions[3] + 0x6c) == 2 && P4_G16(actions[3] + 0x6e) == SKILL_ALL);
    assert(P4_G16(actions[3] + 0x6a) == 3 && P4_G32(actions[3] + 0x38 + 8) == actions[2]);
    p4_battle_deciders_remove_owner(3);

    /* No target given: the native target stays when it is still legal. */
    native_pick();
    Script keep_target = {.decision = {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_SINGLE + 1}};
    p4_battle_decider_add(3, 0, scripted, applied, &keep_target);
    p4_battle_decide_ready(actions[3]);
    assert(keep_target.applied == 1 && P4_G16(actions[3] + 0x6e) == SKILL_SINGLE + 1 && P4_G32(actions[3] + 0x38) == actions[1]);
    p4_battle_deciders_remove_owner(3);

    native_pick();
    Script pass = {.decision = {.pick = P4_BATTLE_PICK_PASS}};
    p4_battle_decider_add(3, 0, scripted, applied, &pass);
    p4_battle_decide_ready(actions[3]);
    assert(pass.applied == 1 && carrier_kind == 7 && P4_G16(actions[3] + 0x6c) == 7 && !P4_G16(actions[3] + 0x6a));
    p4_battle_deciders_remove_owner(3);
}
static void refusals_keep_the_enemy_choice(void)
{
    const P4BattleDecision bad[] = {
        {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_INACTIVE},
        {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_SINGLE, .has_target = 1, .target = 3}, /* itself */
        {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_SELF, .has_target = 1, .target = 0},
        {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_SINGLE, .has_target = 1, .target = 11},
    };
    for (unsigned i = 0; i < sizeof bad / sizeof *bad; ++i) {
        native_pick();
        Script script = {.decision = bad[i]};
        p4_battle_decider_add(4, 0, scripted, applied, &script);
        p4_battle_decide_ready(actions[3]);
        assert(script.calls == 1 && !script.applied && !carrier_calls);
        assert(P4_G16(actions[3] + 0x6e) == SKILL_SINGLE && P4_G32(actions[3] + 0x38) == actions[1]);
        p4_battle_deciders_remove_owner(4);
    }
    /* No legal target: every party member is down. */
    native_pick();
    for (unsigned i = 0; i < 3; ++i) P4_G16(records[i] + 8) = 0;
    Script none = {.decision = {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_ALL}};
    p4_battle_decider_add(4, 0, scripted, applied, &none);
    p4_battle_decide_ready(actions[3]);
    assert(!none.applied && P4_G16(actions[3] + 0x6e) == SKILL_SINGLE);
    p4_battle_deciders_remove_owner(4);
    for (unsigned i = 0; i < 3; ++i) P4_G16(records[i] + 8) = (uint16_t)(50 + 10 * i);
    /* The expanded carrier refusing the rebind keeps the record untouched. */
    native_pick();
    carrier_result = P4_TABLE_CONFLICT;
    Script refused = {.decision = {.pick = P4_BATTLE_PICK_SKILL, .skill = SKILL_SELF}};
    p4_battle_decider_add(4, 0, scripted, applied, &refused);
    p4_battle_decide_ready(actions[3]);
    assert(carrier_calls == 1 && !refused.applied && P4_G16(actions[3] + 0x6e) == SKILL_SINGLE);
    p4_battle_deciders_remove_owner(4);
    /* A failing callback (Lua error) changes nothing and enters the owner's
     * recovery; the stage is usable again afterwards. */
    native_pick();
    unsigned failures = 0;
    Script after = {.decision = {.pick = P4_BATTLE_PICK_PASS}};
    p4_battle_decider_add(4, 0, failing, NULL, &failures);
    p4_battle_decider_add(5, 1, scripted, applied, &after);
    if (!setjmp(recovery)) {
        p4_battle_decide_ready(actions[3]);
        assert(0);
    }
    assert(failures == 1 && failed_owner == 4 && !after.calls && P4_G16(actions[3] + 0x6c) == 2);
    p4_battle_deciders_remove_owner(4);
    p4_battle_decide_ready(actions[3]);
    assert(after.applied == 1);
    p4_battle_deciders_remove_owner(5);
}
static void disabled_and_removed_deciders_do_not_run(void)
{
    native_pick();
    Script script = {.decision = {.pick = P4_BATTLE_PICK_PASS}};
    P4BattleDecider *d = p4_battle_decider_add(6, 0, scripted, applied, &script);
    assert(!p4_battle_decider_enable(d, 0) && !p4_battle_decide_interest);
    p4_battle_decide_ready(actions[3]);
    assert(!script.calls && P4_G16(actions[3] + 0x6c) == 2);
    assert(!p4_battle_decider_enable(d, 1) && p4_battle_decide_interest);
    assert(!p4_battle_decider_remove(d) && !p4_battle_decide_interest);
    assert(p4_battle_decider_enable(d, 1) == -1);
    p4_battle_decide_ready(actions[3]);
    assert(!script.calls);
    p4_battle_deciders_remove_owner(6);
}
int main(void)
{
    if (p4t_guest_map(0x00764000u, 0x1000u)) return 77;
    build();
    snapshot_and_keep();
    retarget_substitute_and_pass();
    refusals_keep_the_enemy_choice();
    disabled_and_removed_deciders_do_not_run();
    puts("battle_decide: snapshot, priority composition, collector legality, record writes, pass and refusals");
    return 0;
}
