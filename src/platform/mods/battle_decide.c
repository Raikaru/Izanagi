/* Battle decision stage (capability battle.decide@1): include/p4pc/battle_decide.h.
 * Layouts: btl_action_internal.h, btl_unit_internal.h and btl_ai_internal.h, NTSC-U.
 * The released command record lives in the BtlAction at +0x38: targets[12] are
 * BtlAction words, +0x6A their count, +0x6C the kind (1 attack, 2 skill, 7 wait),
 * +0x6E the skill. READY (001a4800) consumes exactly these fields. */
#include "p4pc/battle_decide.h"
#include "p4pc/game_elements_ai_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_failure.h"
#include "p4pc/packages.h"
#include "p4pc/table_result.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern uint16_t btlSkillClassifyTargets(void *action, void *selection, uint16_t skill, uint32_t reverse);
extern uint32_t datCalcIsDead(void *unit, int32_t hp_delta);
extern uint16_t datCalcMaxHp(void *unit);
extern uint16_t datCalcMaxSp(void *unit);

#define BATTLE_STATE_GP 0xb3acu   /* iGpffffb3ac: BtlBattleState */
#define BATTLE_ACTIONS 0x174u
#define BATTLE_ROUND 0x20u        /* protagonistTurns: base turns of the protagonist */
#define ACTION_NEXT 0x450u
#define ACTION_PRESENT 0x1au
#define ACTION_ONE_MORE 0x28u
#define ACTION_EXTRA 0x29u
#define ACTION_UNIT 0x30u
#define RECORD_TARGETS 0x38u
#define RECORD_COUNT 0x6au
#define RECORD_KIND 0x6cu
#define RECORD_SKILL 0x6eu
#define UNIT_GENUS 0xa2u
#define UNIT_ID 0xa4u
#define UNIT_UID 0xa8u
#define UNIT_RECORD 0xa64u
#define SELECTION_COUNT 0x38u
#define SELECTION_BYTES 0x40u     /* the full native BtlTargetSelection */
#define KIND_SKILL 2u
#define KIND_WAIT 7u

struct P4BattleDecider {
    size_t owner;
    int32_t priority;
    int enabled, removed;
    P4BattleDecideFn callback;
    P4BattleDecideAppliedFn applied;
    void *user;
    P4BattleDecider *next;
};
static P4BattleDecider *deciders;
static unsigned deciding;
static uint32_t scratch; /* port-owned guest BtlTargetSelection, never freed */
uint32_t p4_battle_decide_interest;

static void update_interest(void)
{
    uint32_t interest = 0;
    for (P4BattleDecider *d = deciders; d; d = d->next)
        if (d->enabled && !d->removed) interest = 1;
    p4_battle_decide_interest = interest;
}
P4BattleDecider *p4_battle_decider_add(size_t owner, int32_t priority, P4BattleDecideFn callback,
                                       P4BattleDecideAppliedFn applied, void *user)
{
    if (!callback || deciding) return NULL;
    P4BattleDecider *d = calloc(1, sizeof *d);
    if (!d) return NULL;
    *d = (P4BattleDecider){owner, priority, 1, 0, callback, applied, user, NULL};
    P4BattleDecider **link = &deciders;
    while (*link && ((*link)->priority < priority || ((*link)->priority == priority && (*link)->owner <= owner)))
        link = &(*link)->next;
    d->next = *link;
    *link = d;
    update_interest();
    return d;
}
int p4_battle_decider_enable(P4BattleDecider *d, int enabled)
{
    if (!d || d->removed || deciding) return -1;
    d->enabled = !!enabled;
    update_interest();
    return 0;
}
int p4_battle_decider_remove(P4BattleDecider *d)
{
    if (!d || deciding) return -1;
    d->removed = 1;
    d->enabled = 0;
    update_interest();
    return 0;
}
void p4_battle_deciders_remove_owner(size_t owner)
{
    for (P4BattleDecider **link = &deciders; *link;) {
        P4BattleDecider *d = *link;
        if (d->owner == owner || d->removed) { *link = d->next; free(d); }
        else link = &d->next;
    }
    update_interest();
}

static uint32_t battle_state(void) { return P4_GPDATA(uint32_t, BATTLE_STATE_GP); }

static void snapshot(uint32_t action, P4BattleDecideState *state)
{
    memset(state, 0, sizeof *state);
    state->actor = UINT32_MAX;
    const uint32_t battle = battle_state();
    if (battle) state->round = P4_G16(battle + BATTLE_ROUND);
    uint32_t cursor = battle ? P4_G32(battle + BATTLE_ACTIONS) : 0;
    for (unsigned n = 0; cursor && n < 32 && state->count < P4_BATTLE_DECIDE_UNITS; ++n, cursor = P4_G32(cursor + ACTION_NEXT)) {
        if (!(P4_G8(cursor + ACTION_PRESENT) & 1u)) continue;
        const uint32_t unit = P4_G32(cursor + ACTION_UNIT);
        if (!unit || P4_G8(unit + UNIT_GENUS) > 1u) continue;
        const uint32_t record = P4_G32(unit + UNIT_RECORD);
        P4BattleDecideUnit *out = &state->units[state->count];
        out->action = cursor;
        out->uid = P4_G32(unit + UNIT_UID);
        out->id = P4_G16(unit + UNIT_ID);
        out->side = P4_G8(unit + UNIT_GENUS);
        if (record) {
            void *host = P4_G2H(record);
            out->alive = !datCalcIsDead(host, 0);
            out->hp = P4_G16(record + 8);
            out->sp = P4_G16(record + 10);
            out->hp_max = datCalcMaxHp(host);
            out->sp_max = datCalcMaxSp(host);
            out->status = P4_G32(record + 12);
            out->charged = (P4_G32(record + 0x14) & 0xC0000u) != 0;
        }
        if (cursor == action) state->actor = state->count;
        ++state->count;
    }
    state->native_kind = P4_G16(action + RECORD_KIND);
    state->skill = P4_G16(action + RECORD_SKILL);
    state->pick = state->native_kind == 1u || state->native_kind == KIND_SKILL ? P4_BATTLE_PICK_SKILL :
                  state->native_kind == KIND_WAIT ? P4_BATTLE_PICK_PASS : P4_BATTLE_PICK_OTHER;
    const uint32_t targets = P4_G16(action + RECORD_COUNT);
    for (uint32_t i = 0; i < targets && i < 12u; ++i) {
        const uint32_t target = P4_G32(action + RECORD_TARGETS + i * 4u);
        for (uint32_t u = 0; u < state->count; ++u)
            if (state->units[u].action == target) { state->targets[state->target_count++] = (uint8_t)u; break; }
    }
    state->one_more = P4_G8(action + ACTION_ONE_MORE);
    state->extra = P4_G8(action + ACTION_EXTRA);
}

int p4_battle_decide_candidates(const P4BattleDecideState *state, uint16_t skill,
                                uint8_t *indices, uint32_t *count, int *all)
{
    *count = 0;
    *all = 0;
    if (!state || state->actor >= state->count || skill > P4_SKILL_MAX_ID || !p4_game_skill_active(skill)) return -1;
    if (!scratch && !(scratch = p4_gport_alloc(SELECTION_BYTES, 16))) return -1;
    /* The collector the enemy AI's own target pick uses (001DBF20): it only
     * reads the battle's action list and writes this port-owned selection. */
    const uint16_t classification = btlSkillClassifyTargets(P4_G2H(state->units[state->actor].action),
                                                            P4_G2H(scratch), skill, 0);
    const uint32_t found = P4_G16(scratch + SELECTION_COUNT);
    for (uint32_t i = 0; i < found && i < 12u; ++i) {
        const uint32_t target = P4_G32(scratch + i * 4u);
        for (uint32_t u = 0; u < state->count; ++u)
            if (state->units[u].action == target) { indices[(*count)++] = (uint8_t)u; break; }
    }
    *all = classification != 0;
    return 0;
}

static int contains(const uint8_t *indices, uint32_t count, uint32_t index)
{
    for (uint32_t i = 0; i < count; ++i)
        if (indices[i] == index) return 1;
    return 0;
}
/* 1 applied, 0 refused (reason printed, native choice kept). */
static int apply(uint32_t action, const P4BattleDecideState *state, const P4BattleDecision *decision, size_t owner)
{
    const char *refusal = NULL;
    uint8_t targets[12];
    uint32_t count = 0;
    if (decision->pick == P4_BATTLE_PICK_PASS) {
        if (p4_game_elements_ai_native_decide(P4_G2H(action), KIND_WAIT, 0) != P4_TABLE_OK)
            refusal = "the enemy AI carrier refused the pass";
        else {
            P4_G16(action + RECORD_KIND) = KIND_WAIT;
            P4_G16(action + RECORD_COUNT) = 0;
            return 1;
        }
    } else {
        int all = 0;
        if (p4_battle_decide_candidates(state, decision->skill, targets, &count, &all))
            refusal = "the skill is not an active skill";
        else if (!count)
            refusal = "the skill has no legal target now";
        else if (decision->has_target && !contains(targets, count, decision->target))
            refusal = "the target is not a legal target of that skill";
        else if (!all) {
            /* One target: the mod's, else the native pick's when still legal,
             * else the collector's first; never a random draw. */
            uint32_t chosen = targets[0];
            if (decision->has_target) chosen = decision->target;
            else if (state->target_count == 1 && contains(targets, count, state->targets[0])) chosen = state->targets[0];
            targets[0] = (uint8_t)chosen;
            count = 1;
        }
        if (!refusal) {
            const int result = p4_game_elements_ai_native_decide(P4_G2H(action), KIND_SKILL, decision->skill);
            if (result != P4_TABLE_OK)
                refusal = result == P4_TABLE_NOT_FOUND ? "the skill is not an active skill"
                                                       : "the enemy AI carrier refused the command";
        }
        if (!refusal) {
            P4_G16(action + RECORD_KIND) = KIND_SKILL;
            P4_G16(action + RECORD_SKILL) = decision->skill;
            for (uint32_t i = 0; i < count; ++i) P4_G32(action + RECORD_TARGETS + i * 4u) = state->units[targets[i]].action;
            P4_G16(action + RECORD_COUNT) = (uint16_t)count;
            return 1;
        }
    }
    const P4PackageInfo *info = p4_packages_info(owner);
    if (decision->pick == P4_BATTLE_PICK_PASS)
        fprintf(stderr, "p4pc: mod %s: p4.battle.decide: pass refused: %s; the enemy's own choice stands\n",
                info ? info->id : "?", refusal);
    else
        fprintf(stderr, "p4pc: mod %s: p4.battle.decide: skill %u refused: %s; the enemy's own choice stands\n",
                info ? info->id : "?", decision->skill, refusal);
    return 0;
}

void p4_battle_decide_ready(uint32_t action)
{
    if (deciding || !action) return;
    const uint32_t unit = P4_G32(action + ACTION_UNIT);
    if (!unit || P4_G8(unit + UNIT_GENUS) != 1u) return; /* decide@1: enemies only */
    deciding = 1;
    for (P4BattleDecider *d = deciders; d; d = d->next) {
        if (!d->enabled || d->removed) continue;
        P4BattleDecideState state;
        snapshot(action, &state);
        if (state.actor >= state.count) break;
        P4BattleDecision decision = {0};
        if (d->callback(&state, &decision, d->user)) {
            /* A Lua error: the enemy's own choice stands and the owner enters
             * controlled mod recovery, as a failing authorizer does. */
            deciding = 0;
            p4_mod_failure(d->owner, "Native p4.battle.decide callback failed");
        }
        if (decision.pick == P4_BATTLE_PICK_OTHER) continue;
        if (apply(action, &state, &decision, d->owner) && d->applied) d->applied(d->user);
    }
    deciding = 0;
}
