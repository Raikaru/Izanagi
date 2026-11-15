/* Recovery, skill cost, status/KO chance, wear-off and buff stages and the
 * affliction count: order, single conversion, clamps, refusals and the native
 * scalar boundaries. The shared registry is the damage stage's;
 * game_damage_stage_test covers its order and refusals in depth. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "p4pc/battle_events.h"
#include "p4pc/game_afflictions_native.h"
#include "p4pc/game_chance_stage.h"
#include "p4pc/game_chance_stage_native.h"
#include "p4pc/game_cost_stage.h"
#include "p4pc/game_cost_stage_native.h"
#include "p4pc/game_damage_stage_native.h"
#include "p4pc/game_buff_stage.h"
#include "p4pc/game_buff_stage_native.h"
#include "p4pc/game_recovery_stage.h"
#include "p4pc/game_recovery_stage_native.h"
#include "p4pc/game_reward_stage.h"
#include "p4pc/game_reward_stage_native.h"
#include "p4pc/game_unit_stage.h"
#include "p4pc/game_wearoff_stage.h"
#include "p4pc/game_wearoff_stage_native.h"

static char order[16];
static size_t order_length;
/* Later queries need not reset the recorder; retain the full invocation count
 * while storing only the prefix used by the ordering assertions. */
static void record_order(char tag)
{
    if (order_length < sizeof(order)) order[order_length] = tag;
    ++order_length;
}
typedef struct Step { char tag; double value; int apply; } Step;

static P4GameCostHit cost_seen;
static int cost_step(void *user, const P4GameCostHit *hit, double *value)
{
    const Step *s = user;
    cost_seen = *hit;
    record_order(s->tag);
    *value = s->value;
    return s->apply;
}
static void cost_stage(void)
{
    assert(!p4_game_cost_native_active() && !p4_game_cost_count());
    Step a = {'a', 1.25, 1}, b = {'b', 2.0, 1}, c = {'c', 1.0, 0};
    const P4GameCostModifierDesc add = {"t.add", 0, 0, P4_COST_ADD, cost_step, &b};
    const P4GameCostModifierDesc mul = {"t.mul", 0, 1, P4_COST_MULTIPLY, cost_step, &a};
    const P4GameCostModifierDesc skip = {"t.skip", -5, 0, P4_COST_MULTIPLY, cost_step, &c};
    uint32_t h[3];
    assert(p4_game_cost_register(&add, &h[0]) == P4_DAMAGE_OK);
    assert(p4_game_cost_register(&mul, &h[1]) == P4_DAMAGE_OK);
    assert(p4_game_cost_register(&skip, &h[2]) == P4_DAMAGE_OK);
    assert(p4_game_cost_count() == 3 && p4_game_cost_native_active());
    /* Priority first, then MULTIPLY before ADD: 12 * 1.25 + 2 = 17. */
    order_length = 0;
    assert(p4_game_cost_native_amount(0x2, 0, 1, 2, 12, 0x2a, 31) == 17);
    assert(order_length == 3 && !memcmp(order, "cab", 3));
    assert(cost_seen.skill_id == 2 && cost_seen.side == P4_DAMAGE_SIDE_PARTY && cost_seen.unit_id == 1 &&
        cost_seen.resource == P4_COST_RESOURCE_SP && cost_seen.stock == 12 && cost_seen.persona == 0x2a &&
        cost_seen.persona_level == 31);
    /* Round half up once: 3 * 1.25 + 2 = 5.75 -> 6; 2 * 1.25 + 2 = 4.5 -> 5. */
    assert(p4_game_cost_native_amount(0x2, 0, 1, 2, 3, 0x2a, 31) == 6);
    assert(p4_game_cost_native_amount(0x2, 0, 1, 2, 2, 0x2a, 31) == 5);
    /* A 00113520 row prices the persona record it shows (a stocked one here)
     * for the call it encloses; HP is kind 1. */
    p4_game_cost_native_persona_begin(0x55, 60);
    (void)p4_game_cost_native_amount(0x80, 0, 1, 1, 10, 0x2a, 31);
    assert(cost_seen.persona == 0x55 && cost_seen.persona_level == 60 && cost_seen.resource == P4_COST_RESOURCE_HP);
    p4_game_cost_native_persona_end();
    (void)p4_game_cost_native_amount(0x80, 0, 1, 1, 10, 0x2a, 31);
    assert(cost_seen.persona == 0x2a && cost_seen.persona_level == 31);
    /* An enemy never reports a persona, whatever row is open. */
    p4_game_cost_native_persona_begin(0x55, 60);
    (void)p4_game_cost_native_amount(0x80, 1, 1, 2, 10, 0x2a, 31);
    p4_game_cost_native_persona_end();
    assert(cost_seen.side == P4_DAMAGE_SIDE_ENEMY && !cost_seen.persona && !cost_seen.persona_level);
    for (int i = 0; i < 3; ++i) assert(p4_game_cost_unregister(h[i]) == P4_DAMAGE_OK);
    /* A nonzero cost never falls below 1, and is clamped at P4_COST_MAX. */
    Step zero = {'z', 0.0, 1}, huge = {'h', 1e9, 1};
    const P4GameCostModifierDesc low = {"t.low", 0, 0, P4_COST_MULTIPLY, cost_step, &zero};
    assert(p4_game_cost_register(&low, &h[0]) == P4_DAMAGE_OK);
    assert(p4_game_cost_native_amount(0x2, 0, 1, 2, 9, 1, 1) == 1);
    assert(p4_game_cost_unregister(h[0]) == P4_DAMAGE_OK);
    const P4GameCostModifierDesc high = {"t.high", 0, 0, P4_COST_MULTIPLY, cost_step, &huge};
    assert(p4_game_cost_register(&high, &h[0]) == P4_DAMAGE_OK);
    assert(p4_game_cost_native_amount(0x2, 0, 1, 2, 9, 1, 1) == P4_COST_MAX);
    assert(p4_game_cost_unregister(h[0]) == P4_DAMAGE_OK);
    /* Refusals. */
    const P4GameCostModifierDesc bad_kind = {"t.bad", 0, 0, (P4GameCostKind)7, cost_step, &a};
    const P4GameCostModifierDesc bad_owner = {"has space", 0, 0, P4_COST_MULTIPLY, cost_step, &a};
    const P4GameCostModifierDesc no_fn = {"t.nofn", 0, 0, P4_COST_MULTIPLY, NULL, &a};
    assert(p4_game_cost_register(&bad_kind, &h[0]) == P4_DAMAGE_INVALID);
    assert(p4_game_cost_register(&bad_owner, &h[0]) == P4_DAMAGE_INVALID);
    assert(p4_game_cost_register(&no_fn, &h[0]) == P4_DAMAGE_INVALID);
    assert(p4_game_cost_unregister(12345u) == P4_DAMAGE_NOT_FOUND);
    assert(!p4_game_cost_native_active());
}

static P4GameRecoveryHit recovery_seen;
static int recovery_step(void *user, const P4GameRecoveryHit *hit, double *value)
{
    recovery_seen = *hit;
    *value = *(const double *)user;
    return 1;
}
static void recovery_stage(void)
{
    assert(!p4_game_recovery_native_active());
    double factor = 1.2;
    const P4GameRecoveryModifierDesc desc = {"t.heal", 0, 0, P4_RECOVERY_MULTIPLY, recovery_step, &factor};
    uint32_t handle;
    assert(p4_game_recovery_register(&desc, &handle) == P4_DAMAGE_OK && p4_game_recovery_native_active());
    /* An estimate: truncated once (101 * 1.2 = 121.2 -> 121), origin unknown, persona reported. */
    assert(p4_game_recovery_native_amount(0xc0, 0, 1, 0, 2, 101, 0x2a, 31) == 121);
    assert(!recovery_seen.resolved && recovery_seen.origin == P4_DAMAGE_ORIGIN_UNKNOWN &&
        recovery_seen.source_persona == 0x2a && recovery_seen.source_persona_level == 31 &&
        recovery_seen.target_side == P4_DAMAGE_SIDE_PARTY && recovery_seen.target_id == 2 && recovery_seen.stock == 101);
    /* The 001f14f0 bracket around the same HP amount call resolves it. */
    p4_game_damage_native_hit_begin(0xc0, 0, 2, 0, 0, 0);
    (void)p4_game_recovery_native_amount(0xc0, 0, 1, 0, 2, 101, 0x2a, 31);
    p4_game_damage_native_hit_end();
    assert(recovery_seen.resolved && recovery_seen.origin == P4_DAMAGE_ORIGIN_SKILL && recovery_seen.source_persona == 0x2a);
    /* An enemy healer reports no persona; clamps are 1..P4_RECOVERY_MAX. */
    factor = 0.0;
    assert(p4_game_recovery_native_amount(0xc0, 1, 7, 1, 8, 50, 0x2a, 31) == P4_RECOVERY_MIN);
    assert(recovery_seen.source_side == P4_DAMAGE_SIDE_ENEMY && !recovery_seen.source_persona);
    factor = 1e9;
    assert(p4_game_recovery_native_amount(0xc0, 0, 1, 0, 2, 50, 0x2a, 31) == P4_RECOVERY_MAX);
    assert(p4_game_recovery_unregister(handle) == P4_DAMAGE_OK && !p4_game_recovery_native_active());
}

static P4GameChanceHit chance_seen;
static int chance_step(void *user, const P4GameChanceHit *hit, double *value)
{
    chance_seen = *hit;
    *value = *(const double *)user;
    return 1;
}
static void chance_stage(void)
{
    assert(!p4_game_chance_native_active());
    double factor = 1.15;
    const P4GameChanceModifierDesc desc = {"t.chance", 0, 0, P4_CHANCE_MULTIPLY, chance_step, &factor};
    uint32_t handle;
    assert(p4_game_chance_register(&desc, &handle) == P4_DAMAGE_OK && p4_game_chance_native_active());
    /* Truncated once (45 * 1.15 = 51.75 -> 51); an ailment attempt by a party skill. */
    assert(p4_game_chance_native_rate(0x53, 0, 1, 1, 40, 0x10, 0, 45, 0x2a, 31, 2) == 51);
    assert(chance_seen.component == P4_CHANCE_COMPONENT_AILMENT && chance_seen.statuses == 0x10 && !chance_seen.item &&
        chance_seen.source_persona == 0x2a && chance_seen.source_persona_level == 31 && chance_seen.stock == 45 &&
        chance_seen.target_side == P4_DAMAGE_SIDE_ENEMY && chance_seen.target_id == 40 && chance_seen.afflictions == 2);
    /* An instant-KO attempt; an item action reports no persona. */
    (void)p4_game_chance_native_rate(0x3d, 0, 1, 1, 40, 0x80000, 1, 30, 0x2a, 31, 0);
    assert(chance_seen.component == P4_CHANCE_COMPONENT_INSTANT_KO && chance_seen.item && !chance_seen.source_persona &&
        !chance_seen.source_persona_level);
    /* The native clamp follows the stage, so its value may leave 0..99. */
    factor = 3.0;
    assert(p4_game_chance_native_rate(0x53, 0, 1, 1, 40, 0x10, 0, 45, 0x2a, 31, 0) == 135);
    factor = -1.0; /* a negative factor is ignored for that attempt */
    assert(p4_game_chance_native_rate(0x53, 0, 1, 1, 40, 0x10, 0, 45, 0x2a, 31, 0) == 45);
    factor = NAN;
    assert(p4_game_chance_native_rate(0x53, 1, 7, 0, 2, 0x10, 0, 45, 0x2a, 31, 0) == 45);
    assert(chance_seen.source_side == P4_DAMAGE_SIDE_ENEMY && !chance_seen.source_persona);
    assert(p4_game_chance_unregister(handle) == P4_DAMAGE_OK && !p4_game_chance_native_active());
}

/* Ailment wear-off: every roll of every unit, then truncation and the 0..100
 * clamp the native compare can use. */
static P4GameWearoffRoll wearoff_seen;
static int wearoff_step(void *user, const P4GameWearoffRoll *roll, double *value)
{
    const Step *s = user;
    wearoff_seen = *roll;
    record_order(s->tag);
    *value = s->value;
    return s->apply;
}
static void wearoff_stage(void)
{
    assert(!p4_game_wearoff_native_active() && !p4_game_wearoff_count());
    Step add = {'a', 15.0, 1}, mul = {'m', 1.5, 1};
    const P4GameWearoffModifierDesc plus = {"t.plus", 0, 0, P4_WEAROFF_ADD, wearoff_step, &add};
    const P4GameWearoffModifierDesc times = {"t.times", 0, 1, P4_WEAROFF_MULTIPLY, wearoff_step, &mul};
    uint32_t h[2];
    assert(p4_game_wearoff_register(&plus, &h[0]) == P4_DAMAGE_OK);
    assert(p4_game_wearoff_register(&times, &h[1]) == P4_DAMAGE_OK && p4_game_wearoff_native_active());
    /* MULTIPLY before ADD: 25 * 1.5 + 15 = 52.5 -> 52; an enemy's fear on its second turn. */
    order_length = 0;
    assert(p4_game_wearoff_native_rate(1, 0x2b, 0x04, 22, 9, 2, 3, 25, 3) == 52);
    assert(order_length == 2 && !memcmp(order, "ma", 2));
    assert(wearoff_seen.side == P4_DAMAGE_SIDE_ENEMY && wearoff_seen.id == 0x2b && wearoff_seen.status == 0x04 &&
        wearoff_seen.level == 22 && wearoff_seen.luck == 9 && wearoff_seen.turns == 2 && wearoff_seen.limit == 3 &&
        wearoff_seen.stock == 25 && wearoff_seen.afflictions == 3);
    assert(!strcmp(p4_game_wearoff_status_name(0x04), "fear") && !p4_game_wearoff_status_name(0x20));
    /* The roll can only use 0..100: always and never. */
    mul.value = 10.0;
    assert(p4_game_wearoff_native_rate(0, 1, 0x80, 30, 20, 4, 6, 25, 1) == 100);
    assert(wearoff_seen.side == P4_DAMAGE_SIDE_PARTY && wearoff_seen.status == 0x80);
    mul.value = 0.0; add.value = -40.0;
    assert(p4_game_wearoff_native_rate(0, 1, 0x80, 30, 20, 4, 6, 25, 1) == 0);
    assert(p4_game_wearoff_unregister(h[0]) == P4_DAMAGE_OK && p4_game_wearoff_unregister(h[1]) == P4_DAMAGE_OK);
    assert(!p4_game_wearoff_native_active());
}

/* Afflictions: counting is active while a chance or wear-off modifier is
 * registered; landings count per unit; Dizzy and KO never count; the battle
 * constructor clears every count. */
static int chance_identity(void *user, const P4GameChanceHit *hit, double *value)
{
    (void)user; (void)hit;
    *value = 1.0;
    return 1;
}
static void afflictions(void)
{
    static int units[3];
    assert(!p4_game_afflictions_native_active());
    const P4GameChanceModifierDesc desc = {"t.decay", 0, 0, P4_CHANCE_MULTIPLY, chance_identity, NULL};
    uint32_t handle;
    assert(p4_game_chance_register(&desc, &handle) == P4_DAMAGE_OK && p4_game_afflictions_native_active());
    p4_game_afflictions_native_landed(&units[0], 0x10);
    p4_game_afflictions_native_landed(&units[0], 0x04);
    p4_game_afflictions_native_landed(&units[0], 0x01);    /* Dizzy */
    p4_game_afflictions_native_landed(&units[0], 0x80000); /* KO */
    p4_game_afflictions_native_landed(&units[1], 0x20);
    assert(p4_game_afflictions_native_count(&units[0]) == 2 && p4_game_afflictions_native_count(&units[1]) == 1 &&
        !p4_game_afflictions_native_count(&units[2]));
    /* Another request stage leaves the counts; TASK_BEGIN clears them. */
    p4_battle_publish_request(&(P4BattleRequest){.stage = P4_BATTLE_TASK_CREATED});
    assert(p4_game_afflictions_native_count(&units[0]) == 2);
    p4_battle_publish_request(&(P4BattleRequest){.stage = P4_BATTLE_TASK_BEGIN});
    assert(!p4_game_afflictions_native_count(&units[0]) && !p4_game_afflictions_native_count(&units[1]));
    p4_game_afflictions_native_landed(&units[1], 0x02);
    assert(p4_game_afflictions_native_count(&units[1]) == 1);
    assert(p4_game_chance_unregister(handle) == P4_DAMAGE_OK && !p4_game_afflictions_native_active());
}

/* Buff multipliers: a rebalance's one rank x1.2 and Charge x1.8 written as
 * modifiers over a synthetic table value of 1.5 and Charge's native x2.5. */
static P4GameBuffQuery buff_seen;
static int buff_rebalance(void *user, const P4GameBuffQuery *query, double *value)
{
    (void)user;
    buff_seen = *query;
    if (query->stat != P4_BUFF_ATTACK && query->stat != P4_BUFF_MAGIC) return 0;
    double factor = query->rank > 0 ? 1.2 / 1.5 : 1.0;
    if (query->charged) factor *= 1.8 / 2.5;
    *value = factor;
    return 1;
}
static void buff_stage(void)
{
    assert(!p4_game_buff_native_active() && !p4_game_buff_count());
    const P4GameBuffModifierDesc desc = {"t.rebalance", 0, 0, P4_BUFF_MULTIPLY, buff_rebalance, NULL};
    uint32_t handle;
    assert(p4_game_buff_register(&desc, &handle) == P4_DAMAGE_OK && p4_game_buff_native_active());
    /* A charged attack at rank +1: 1.5 * 2.5 -> 1.2 * 1.8. */
    assert(fabsf(p4_game_buff_native_factor(0, 1, P4_BUFF_ATTACK, 1, 1, 3.75f) - 2.16f) < 1e-5f);
    assert(buff_seen.side == P4_DAMAGE_SIDE_PARTY && buff_seen.id == 1 && buff_seen.rank == 1 && buff_seen.charged &&
        buff_seen.stat == P4_BUFF_ATTACK && !strcmp(p4_game_buff_stat_name(P4_BUFF_ATTACK), "attack"));
    /* Another stat passes untouched, exactly. */
    assert(p4_game_buff_native_factor(1, 0x2b, P4_BUFF_ACCURACY, -1, 0, 0.75f) == 0.75f);
    assert(buff_seen.side == P4_DAMAGE_SIDE_ENEMY && buff_seen.stat == P4_BUFF_ACCURACY && buff_seen.rank == -1);
    assert(!strcmp(p4_game_buff_stat_name(P4_BUFF_EVASION), "evasion") && !p4_game_buff_stat_name(18));
    assert(p4_game_buff_unregister(handle) == P4_DAMAGE_OK && !p4_game_buff_native_active());
}

/* Battle rewards: a challenge's level cap (0 pays nothing) and a party-wide
 * EXP boost, each recipient's identity, money, and the 0..P4_REWARD_MAX clamp. */
static P4GameRewardQuery reward_seen;
static int reward_rules(void *user, const P4GameRewardQuery *query, double *value)
{
    (void)user;
    reward_seen = *query;
    if (query->reward == P4_REWARD_MONEY) { *value = 3.0; return 1; }
    if (query->level >= 20) { *value = 0.0; return 1; }
    *value = 1.5;
    return 1;
}
static void reward_stage(void)
{
    assert(!p4_game_reward_native_active() && !p4_game_reward_count());
    const P4GameRewardModifierDesc desc = {"t.reward", 0, 0, P4_REWARD_MULTIPLY, reward_rules, NULL};
    uint32_t handle;
    assert(p4_game_reward_register(&desc, &handle) == P4_DAMAGE_OK && p4_game_reward_native_active());
    assert(p4_game_reward_native_hero(12, 101) == 151); /* 151.5 truncated once */
    assert(reward_seen.reward == P4_REWARD_EXP && reward_seen.recipient == P4_REWARD_HERO && reward_seen.id == 1 &&
        reward_seen.level == 12 && reward_seen.stock == 101 && !reward_seen.equipped);
    assert(p4_game_reward_native_party(3, 25, 400) == 0); /* capped at level 20 */
    assert(reward_seen.recipient == P4_REWARD_PARTY && reward_seen.id == 3 && reward_seen.level == 25);
    assert(p4_game_reward_native_persona(0x2a, 14, 7, 80) == 120);
    assert(reward_seen.recipient == P4_REWARD_PERSONA && reward_seen.id == 0x2a && reward_seen.equipped == 1);
    assert(p4_game_reward_native_persona(0x31, 9, 0, 0) == 0 && !reward_seen.equipped);
    assert(p4_game_reward_native_money(500000) == P4_REWARD_MAX);
    assert(reward_seen.reward == P4_REWARD_MONEY && !reward_seen.recipient && !reward_seen.id && !reward_seen.level);
    assert(p4_game_reward_unregister(handle) == P4_DAMAGE_OK && !p4_game_reward_native_active());
}

/* Unit values: hard mode's enemy HP/SP x1.5 and +5 Strength as one ADD
 * modifier, each value's range, and party values left alone. */
static P4GameUnitQuery unit_seen;
static int unit_rules(void *user, const P4GameUnitQuery *query, double *value)
{
    (void)user;
    unit_seen = *query;
    if (query->side != P4_DAMAGE_SIDE_ENEMY) return 0;
    if (query->stat == P4_UNIT_MAX_HP || query->stat == P4_UNIT_MAX_SP) { *value = query->stock * 0.5; return 1; }
    if (query->stat == P4_UNIT_STRENGTH) { *value = 5.0; return 1; }
    return 0;
}
static void unit_stage(void)
{
    assert(!p4_game_unit_native_active() && !p4_game_unit_count());
    const P4GameUnitModifierDesc desc = {"t.unit", 0, 0, P4_UNIT_ADD, unit_rules, NULL};
    uint32_t handle;
    assert(p4_game_unit_register(&desc, &handle) == P4_DAMAGE_OK && p4_game_unit_native_active());
    /* 101 + 50.5 = 151.5, truncated once. */
    assert(p4_game_unit_native_amount(1, 0x2b, P4_UNIT_MAX_HP, 12, 101) == 151);
    assert(unit_seen.side == P4_DAMAGE_SIDE_ENEMY && unit_seen.id == 0x2b && unit_seen.level == 12 &&
        unit_seen.stat == P4_UNIT_MAX_HP && !strcmp(p4_game_unit_stat_name(unit_seen.stat), "max_hp"));
    /* An enemy's pool is a u16. */
    assert(p4_game_unit_native_amount(1, 0x2b, P4_UNIT_MAX_HP, 12, 50000) == P4_UNIT_ENEMY_POOL_MAX);
    /* Stats stay 1..99: 97 + 5 clamps to 99. */
    assert(p4_game_unit_native_amount(1, 0x2b, P4_UNIT_STRENGTH, 12, 97) == P4_UNIT_STAT_MAX);
    assert(!strcmp(p4_game_unit_stat_name(P4_UNIT_LUCK), "luck") && !p4_game_unit_stat_name(7));
    /* A party member's value passes untouched, and its pool cap is 999. */
    assert(p4_game_unit_native_amount(0, 2, P4_UNIT_MAX_SP, 30, 250) == 250);
    assert(unit_seen.side == P4_DAMAGE_SIDE_PARTY && unit_seen.stat == P4_UNIT_MAX_SP);
    assert(p4_game_unit_unregister(handle) == P4_DAMAGE_OK && !p4_game_unit_native_active());
}
/* A modifier that re-enters its own stage leaves the inner value at stock. */
static int32_t nested_cost;
static int cost_reenter(void *user, const P4GameCostHit *hit, double *value)
{
    (void)user;
    if (hit->stock == 7) nested_cost = (int32_t)p4_game_cost_native_amount(0x2, 0, 1, 2, 9, 1, 1);
    *value = 2.0;
    return 1;
}
static void reentry(void)
{
    const P4GameCostModifierDesc desc = {"t.reenter", 0, 0, P4_COST_MULTIPLY, cost_reenter, NULL};
    uint32_t handle;
    assert(p4_game_cost_register(&desc, &handle) == P4_DAMAGE_OK);
    assert(p4_game_cost_native_amount(0x2, 0, 1, 2, 7, 1, 1) == 14 && nested_cost == 9);
    assert(p4_game_cost_unregister(handle) == P4_DAMAGE_OK);
}

int main(void)
{
    cost_stage();
    recovery_stage();
    chance_stage();
    wearoff_stage();
    afflictions();
    buff_stage();
    reward_stage();
    unit_stage();
    reentry();
    puts("game_amount_stages: recovery, skill cost, status chance, wear-off, buff, reward and unit order, "
        "conversion, clamps, brackets and refusals, and affliction counts passed");
    return 0;
}
