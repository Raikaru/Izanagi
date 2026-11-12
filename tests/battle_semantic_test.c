/* Semantic battle events (src/platform/mods/battle_semantic.c) from synthetic
 * observer packets: decoding per local://api-research.md section 6, commit
 * order, interest gating, the bounded queue and the frame safe-point bit; and the
 * native notifications (battle_notify.h) over synthetic guest records: One More
 * causes, the battle result's reward, level-ups and learned skills. */
#include "guest_test.h"
#include "p4pc/battle_notify.h"
#include "p4pc/battle_semantic.h"
#include "p4pc/mod_entry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

uint32_t p4_mod_safe_point;
static unsigned observers, removed;
uint32_t p4_battle_observer_add(const P4BattleObserver *observer)
{
    assert(observer && observer->hit && observer->command && observer->admission);
    return ++observers;
}
int p4_battle_observer_remove(uint32_t token) { assert(token == observers); ++removed; return 0; }
static uint32_t hero_exp;
uint32_t datPersonaGetExpById(int16_t unit) { assert(unit == 1); return hero_exp; }
uint8_t datLevelFromExp(int32_t exp) { return (uint8_t)(exp / 100 + 1); }

static P4BattleActor actor(uint64_t uid, uint16_t id, uint8_t side)
{
    return (P4BattleActor){uid, (uint32_t)uid + 100, id, side, 1};
}
static P4BattleHit hit(uint16_t effect, int32_t delta, uint16_t before, uint16_t after)
{
    return (P4BattleHit){.source = actor(1, 0x2b0, 1), .target = actor(2, 2, 0), .requested_hp_delta = delta,
        .skill_id = 10, .action_kind = 2, .hp_before = before, .hp_after = after, .effect = effect,
        .target_flags = 1, .resources_applied = 1, .statuses_applied = 1};
}
static size_t drain(P4SemanticEvent *out)
{
    uint32_t lost = 0;
    const size_t count = p4_semantic_drain(out, 300, &lost);
    assert(!lost);
    return count;
}

static void notifications(P4SemanticEvent *out)
{
    assert(p4_battle_notify_interest == (P4_NOTIFY_ONE_MORE | P4_NOTIFY_REWARD | P4_NOTIFY_LEVEL_UP));
    /* One More: the cause comes from the hits of the action that earned it. */
    const uint32_t action = P4_H2G(p4t_galloc(0x458, 16)), unit = P4_H2G(p4t_galloc(0xb0, 16));
    P4_G32(action + 0x30) = unit;
    P4_G8(unit + 0xa2) = 0;
    P4_G16(unit + 0xa4) = 3;
    P4_G32(unit + 0xa8) = 107;
    const P4BattleCommand chie = {actor(7, 3, 0), 2, 10, 0, 1, 16, 0, 0};
    P4BattleHit landed = hit(1, -30, 100, 70);
    landed.source = actor(7, 3, 0);
    landed.target = actor(1, 0x2b0, 1);
    p4_semantic_command(&chie);
    landed.target_flags = 2;
    p4_semantic_hit(&landed);
    p4_battle_notify_one_more(action);
    p4_semantic_command(&chie);
    landed.target_flags = 6;
    p4_semantic_hit(&landed);
    p4_battle_notify_one_more(action);
    p4_semantic_command(&chie);
    landed.target_flags = 1;
    p4_semantic_hit(&landed);
    p4_battle_notify_one_more(action);
    /* The result: 500 EXP for the protagonist (level 11 -> 16), Yosuke 300, two items, 1234 yen. */
    const uint32_t awards = P4_H2G(p4t_galloc(0x900, 16)), reward = P4_H2G(p4t_galloc(0x40, 16));
    P4_G32(awards + 4) = 500;
    P4_G16(awards + 0x69A) = 2;
    P4_G32(awards + 0x6A4) = 300;
    P4_G16(awards + 0x69C) = 3; /* Chie earned nothing: not listed */
    P4_G32(reward + 0x30) = 2;
    P4_G16(reward + 0x24) = 0x10; P4_G16(reward + 0x26) = 2;
    P4_G16(reward + 0x28) = 0x20; P4_G16(reward + 0x2A) = 1;
    hero_exp = 1500;
    p4_battle_notify_reward(awards, reward, 1234, 1000);
    /* Yosuke's Persona: 3 -> 6, learning skill 50; skill 51 did not fit. */
    const uint32_t persona = P4_H2G(p4t_galloc(0x30, 16)), delta = P4_H2G(p4t_galloc(0x88, 16));
    P4_G16(persona + 2) = 0x11;
    P4_G8(persona + 4) = 6;
    P4_G16(persona + 0x0C) = 1; P4_G16(persona + 0x0E) = 2; P4_G16(persona + 0x10) = 50;
    P4_G8(delta) = 3;
    P4_G16(delta + 2) = 50;
    P4_G16(delta + 4) = 51;
    p4_battle_notify_level_up(2, persona, delta);
    const size_t n = drain(out);
    const int expected[] = {P4_SEMANTIC_SKILL_USED, P4_SEMANTIC_DAMAGE, P4_SEMANTIC_ONE_MORE,
                            P4_SEMANTIC_SKILL_USED, P4_SEMANTIC_DAMAGE, P4_SEMANTIC_ONE_MORE,
                            P4_SEMANTIC_SKILL_USED, P4_SEMANTIC_DAMAGE, P4_SEMANTIC_ONE_MORE,
                            P4_SEMANTIC_REWARD, P4_SEMANTIC_LEVEL_UP, P4_SEMANTIC_LEVEL_UP, P4_SEMANTIC_SKILL_LEARNED};
    assert(n == sizeof expected / sizeof *expected);
    for (size_t i = 0; i < n; ++i) assert(out[i].kind == expected[i]);
    assert(out[2].result == P4_SEMANTIC_CAUSE_CRITICAL && out[2].source.id == 3 && out[2].source.uid == 107 && !out[2].source.side);
    assert(out[5].result == P4_SEMANTIC_CAUSE_WEAKNESS && out[8].result == P4_SEMANTIC_CAUSE_DOWN);
    const P4SemanticEvent *r = &out[9];
    assert(r->amount == 1234 && r->exp == 500 && r->item_count == 2 && r->items[0] == 0x10 && r->item_counts[0] == 2);
    assert(r->items[1] == 0x20 && r->item_counts[1] == 1 && r->party_count == 1 && r->party[0] == 2 && r->party_exp[0] == 300);
    assert(out[10].target.id == 1 && !out[10].persona && out[10].from == 11 && out[10].level == 16);
    assert(out[11].target.id == 2 && out[11].persona == 0x11 && out[11].from == 3 && out[11].level == 6);
    assert(out[12].target.id == 2 && out[12].persona == 0x11 && out[12].skill == 50);
    /* No level-up subscriber: the protagonist's level is not even read. */
    assert(!p4_semantic_interest(P4_SEMANTIC_LEVEL_UP, -1));
    hero_exp = 0;
    p4_battle_notify_reward(awards, reward, 1, 0);
    assert(drain(out) == 1 && out[0].kind == P4_SEMANTIC_REWARD);
    assert(!p4_semantic_interest(P4_SEMANTIC_LEVEL_UP, 1));
}

int main(void)
{
    P4SemanticEvent out[300];
    if (p4t_guest_init()) return 77;
    /* No subscriber: nothing is observed or queued. */
    P4BattleHit plain = hit(1, -30, 100, 70);
    p4_semantic_hit(&plain);
    assert(!drain(out) && !p4_mod_safe_point && !observers);

    for (int kind = 0; kind < P4_SEMANTIC_KIND_COUNT; ++kind) assert(!p4_semantic_interest(kind, 1));
    assert(observers == 1);
    assert(p4_semantic_kind("damage") == P4_SEMANTIC_DAMAGE && p4_semantic_kind("frame_start") == -1);
    assert(!strcmp(p4_semantic_name(P4_SEMANTIC_KO), "ko"));

    /* One action, in commit order: turn, command, a weak hit that poisons and
     * downs, a critical hit that KOs, a miss, a null and a drain. */
    const P4BattleAdmission turn = {actor(1, 0x2b0, 1), 1, 2, P4_BATTLE_QUEUE_ORDINARY, 0};
    p4_semantic_admission(&turn);
    const P4BattleCommand command = {actor(1, 0x2b0, 1), 2, 10, 0, 1, 16, 0, 0};
    p4_semantic_command(&command);
    const P4BattleCommand persona = {actor(1, 0x2b0, 1), 5, 0, 0, 0, 16, 1, 0};
    p4_semantic_command(&persona);
    P4BattleHit weak = hit(1, -40, 100, 60);
    weak.target_flags = 5;
    weak.status_before = 0x1;
    weak.status_after = 0x20 | 0x100000;
    p4_semantic_hit(&weak);
    P4BattleHit critical = hit(1, -80, 60, 0);
    critical.target_flags = 3;
    critical.status_before = 0x20 | 0x100000;
    critical.status_after = 0x80000;
    p4_semantic_hit(&critical);
    P4BattleHit miss = hit(4, 0, 0, 0);
    p4_semantic_hit(&miss);
    P4BattleHit nulled = hit(0x100, 0, 50, 50);
    p4_semantic_hit(&nulled);
    P4BattleHit drained = hit(0x400, 20, 50, 70);
    p4_semantic_hit(&drained);
    assert(p4_mod_safe_point & P4_MOD_SAFE_SEMANTIC);
    size_t n = drain(out);
    const int expected[] = {
        P4_SEMANTIC_TURN_START, P4_SEMANTIC_SKILL_USED,
        P4_SEMANTIC_DAMAGE, P4_SEMANTIC_AILMENT, P4_SEMANTIC_AILMENT, P4_SEMANTIC_KNOCKDOWN,
        P4_SEMANTIC_DAMAGE, P4_SEMANTIC_AILMENT, P4_SEMANTIC_KO,
        P4_SEMANTIC_MISS, P4_SEMANTIC_DAMAGE, P4_SEMANTIC_DAMAGE,
    };
    assert(n == sizeof expected / sizeof *expected);
    for (size_t i = 0; i < n; ++i) {
        assert(out[i].kind == expected[i]);
        assert(i == 0 || out[i].sequence == out[i - 1].sequence + 1);
    }
    assert(out[0].source.id == 0x2b0 && out[0].source.side == 1 && !out[0].flag);
    assert(out[1].skill == 10 && out[1].action_kind == 2);
    assert(out[2].result == P4_SEMANTIC_WEAK && out[2].amount == 40 && out[2].hp_after == 60 && out[2].target.id == 2);
    /* Dizzy (0x1) cured, Poison (0x20) applied, in bit order. */
    assert(out[3].status == 0x1 && !out[3].flag && out[4].status == 0x20 && out[4].flag);
    assert(out[5].status == P4_SEMANTIC_DOWN_BIT);
    assert(out[6].result == P4_SEMANTIC_CRITICAL && out[6].amount == 60);
    assert(out[7].status == 0x20 && !out[7].flag); /* KO clears Poison */
    assert(out[8].target.id == 2);
    assert(out[10].result == P4_SEMANTIC_NULL && out[10].amount == 0);
    assert(out[11].result == P4_SEMANTIC_DRAIN && out[11].amount == -20);

    /* Self packets: a reflected hit is damage to the attacker; regen, an
     * ailment tick and a guard toggle are not. */
    P4BattleHit reflected = hit(1, -25, 90, 65);
    reflected.target = reflected.source;
    p4_semantic_hit(&reflected);
    P4BattleHit tick = reflected;
    tick.result_flags = 0x100;
    p4_semantic_hit(&tick);
    P4BattleHit regen = reflected;
    regen.requested_hp_delta = 10;
    p4_semantic_hit(&regen);
    P4BattleHit endure = hit(1, -99, 100, 1);
    endure.result_flags = 0x10;
    p4_semantic_hit(&endure);
    P4BattleHit revive = hit(1, 50, 0, 50);
    revive.status_before = 0x80000;
    p4_semantic_hit(&revive);
    n = drain(out);
    assert(n == 3 && out[0].kind == P4_SEMANTIC_DAMAGE && out[0].flag && out[0].target.side == 1);
    assert(out[1].kind == P4_SEMANTIC_DAMAGE && out[1].endured);
    assert(out[2].kind == P4_SEMANTIC_REVIVE);
    notifications(out);

    /* Only kinds with subscribers queue; the queue is bounded and counts losses. */
    assert(!p4_semantic_interest(P4_SEMANTIC_DAMAGE, -1));
    p4_semantic_hit(&plain);
    assert(!drain(out));
    for (unsigned i = 0; i < 300; ++i) p4_semantic_admission(&turn);
    uint32_t lost = 0;
    assert(p4_semantic_drain(out, 300, &lost) == 256 && lost == 44);
    /* The last subscriber gone: the observer is removed and the queue cleared. */
    for (int kind = 0; kind < P4_SEMANTIC_KIND_COUNT; ++kind)
        if (kind != P4_SEMANTIC_DAMAGE) assert(!p4_semantic_interest(kind, -1));
    assert(removed == 1 && p4_semantic_interest(P4_SEMANTIC_DAMAGE, -1) == -1);
    assert(!p4_battle_notify_interest); /* and the notifications with it */
    puts("battle_semantic: decoding, commit order, interest, bounded queue, safe point and notifications");
    return 0;
}
