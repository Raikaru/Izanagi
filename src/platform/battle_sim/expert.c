#include "expert.h"
#include "p4pc/battle_strategy.h"
#include <limits.h>

static int damage(unsigned e) { return e == 1 || e == 4 || e == 8 || e == 10 || e == 12 || e == 13 || e == 14; }
static int healing(unsigned e) { return e == 2 || e == 5 || e == 7 || e == 9 || e == 11 || e == 15; }
static unsigned percent(unsigned n, unsigned d) { return !d ? 0 : n >= d ? 100 : n * 100 / d; }

static int64_t target_score(const struct bsim_observation *o, const struct bsim_candidate_observation *c, unsigned index,
                            int expert)
{
    const struct bsim_unit_observation *u = &o->units[index];
    const uint32_t ailments = c->status_mode == 1 ? c->status_mask & 0x1eu : 0;
    const int32_t power = c->power > 0 ? c->power : 0;
    if (u->side == 1) {
        int64_t technical = 0;
        if (ailments && !(u->status & ailments)) {
            if (u->vulnerable & ailments) technical += 2600;
            else if (!(u->probed2 & ailments)) technical += 500;
        }
        static const uint32_t paired[] = {0, 2, 4, 8, 16};
        if (damage(c->hp_effect) && c->element >= 1 && c->element <= 4 &&
            (u->status & paired[c->element]) && !(u->weak & (1u << c->element))) technical += 1500;
        if (damage(c->hp_effect)) {
            int64_t score = 200 + power;
            if (c->element >= 0 && c->element < 32) {
                const uint32_t bit = UINT32_C(1) << c->element;
                if (u->weak & bit) score += 3000;
                else if (u->bad & bit) return -score;
                else if (!(u->tried & bit)) score += 160;
            }
            if (u->unit_uid == o->focus_uid) score += 400;
            for (unsigned i = 0; i < o->healer_count; i++) if (u->unit_uid == o->healer_uid[i]) score += 700;
            return score + technical;
        }
        if (c->buff_mask && !(c->held_targets & (1u << index))) {
            P4StrategyTempo tempo = {o->setup_pending};
            return p4_strategy_setup_value(c->buff_mask, P4_STRATEGY_ENEMY, o->durable, &tempo, 240) + technical;
        }
        return technical;
    }
    if (u->side || u->native_id < 1 || u->native_id > 10) return 0;
    if (damage(c->hp_effect)) return -20000;
    const unsigned hp_pct = percent(u->hp, u->max_hp), sp_pct = percent(u->sp, u->max_sp);
    const int removes = c->status_mode == 2;
    if (!u->hp && removes && (c->status_mask & 0x80000u)) return 30000;
    const unsigned threshold = u->native_id == 1 ? 70 : 50;
    if (u->hp && healing(c->hp_effect) && hp_pct <= threshold) {
        if (c->hp_effect == 5 && hp_pct > 25 && (uint32_t)power * 4 <= u->max_hp) return 300;
        uint32_t amount = (uint32_t)power;
        if (expert && c->hp_effect == 11) {
            amount = (uint32_t)u->max_hp * amount / 100u;
            if (!amount) amount = 1;
        }
        return p4_strategy_heal_value(24000 + (100 - hp_pct) * 20 + (u->native_id == 1 ? 500 : 0),
                                      c->hp_effect == 5 || (expert && c->hp_effect == 11), amount, u->hp, u->max_hp);
    }
    if (removes && (u->status & c->status_mask)) return 14000;
    if (u->hp && healing(c->sp_effect) && sp_pct < 35) return 9000 + (100 - sp_pct) * 10;
    if (u->hp && c->buff_mask && !(c->held_targets & (1u << index))) {
        P4StrategyTempo tempo = {o->setup_pending};
        return p4_strategy_setup_value(c->buff_mask, P4_STRATEGY_PARTY, o->durable, &tempo, 260);
    }
    return 0;
}

/* Do not change Persona for a damaging skill if the relevant attack stat is
 * lower than the equipped Persona's; support skills are judged separately. */
static int persona_attack_not_weaker(const struct bsim_observation *o, const struct bsim_candidate_observation *c)
{
    if (c->persona_slot == UINT8_MAX || !damage(c->hp_effect)) return 1;
    const struct bsim_persona_observation *lead = 0, *candidate = 0;
    for (unsigned i = 0; i < o->persona_count; i++) {
        if (o->personas[i].slot == o->lead_slot) lead = &o->personas[i];
        if (o->personas[i].slot == c->persona_slot) candidate = &o->personas[i];
    }
    if (!lead || !candidate) return 0;
    if (c->element == 0) return candidate->stats[0] >= lead->stats[0];
    if (c->element > 0) return candidate->stats[1] >= lead->stats[1];
    return candidate->stats[0] >= lead->stats[0] && candidate->stats[1] >= lead->stats[1];
}

static int better(const struct bsim_observation *o, int64_t score, unsigned ci, unsigned ti, const struct bsim_choice *best)
{
    if (score != best->score) return score > best->score;
    const struct bsim_candidate_observation *c = &o->candidates[ci], *b = &o->candidates[best->candidate];
    return c->skill < b->skill || (c->skill == b->skill &&
           (c->item < b->item || (c->item == b->item && o->units[ti].action_uid < o->units[best->target].action_uid)));
}

int bsim_choose(const struct bsim_observation *o, int expert, struct bsim_choice *result)
{
    if (!o || !result || o->actor_index >= o->unit_count) return 0;
    struct bsim_choice best = {INT64_MIN, 0, 0}, persona = {INT64_MIN, 0, 0};
    const struct bsim_unit_observation *actor = &o->units[o->actor_index];
    unsigned charged = 0;
    for (unsigned i = 0; i < o->unit_count; i++) if (o->units[i].side == 1 && !(o->units[i].status & 0x80000u)) {
        charged |= o->units[i].charged;
    }
    for (unsigned ci = 0; ci < o->candidate_count; ci++) {
        const struct bsim_candidate_observation *c = &o->candidates[ci];
        if (expert && !persona_attack_not_weaker(o, c)) continue;
        if (expert && !o->is_boss && percent(actor->sp, actor->max_sp) < 30 &&
            !damage(c->hp_effect) && (c->buff_mask & P4_STRATEGY_BUFF_RAISE)) continue;
        struct bsim_choice *selected = c->persona_slot == UINT8_MAX ? &best : &persona;
        if (c->wheel == 2) {
            int64_t score = charged ? (expert ? 4500000 : 20000) : percent(actor->hp, actor->max_hp) < 20 ? 450 : 10;
            if (score > selected->score) *selected = (struct bsim_choice){score, (uint16_t)ci, o->actor_index};
            continue;
        }
        int64_t penalty = (int64_t)c->cost * (c->cost_kind == 1 ? 4 : 5);
        if (c->cost_kind == 2 && percent(actor->sp, actor->max_sp) < 30) penalty += (int64_t)c->cost * 60;
        for (unsigned ti = 0; ti < o->unit_count; ti++) {
            if (!(c->targets & (1u << ti))) continue;
            if (c->group && ti != c->first_target) continue;
            int64_t score = -penalty, priority = 0;
            for (unsigned j = 0; j < o->unit_count; j++) {
                if (!(c->targets & (1u << j)) || (!c->group && j != ti)) continue;
                const struct bsim_unit_observation *u = &o->units[j];
                score += target_score(o, c, j, expert);
                if (!expert) continue;
                const unsigned hp = percent(u->hp, u->max_hp);
                if (!u->side && ((!u->hp && c->status_mode == 2 && (c->status_mask & 0x80000u)) ||
                    (u->hp && healing(c->hp_effect) && hp <= (u->native_id == 1 ? 70u : 50u)))) {
                    if (priority < 5000000) priority = 5000000;
                }
                /* Removing a party debuff is a tier; dispelling an enemy buff is not: it spent the hero's turn
                 * and lost 23 points to the strategic policy on the Persona cohort's first 100 seeds. */
                int negative = 0;
                for (unsigned k = 0; k < 5; k++) negative |= u->buffs[k] < 0;
                if (!u->side && negative && (c->buff_mask & 0x2000) && priority < 4000000) priority = 4000000;
                if (!u->side && !damage(c->hp_effect) && (c->buff_mask & u->expired_buffs & 0x45u)) score += 100;
                if (u->side && damage(c->hp_effect) && c->element >= 0 && c->element < 32 &&
                    (u->weak & (UINT32_C(1) << c->element)) && priority < 3500000) priority = 3500000;
            }
            score += priority;
            if (selected->score == INT64_MIN ||
                (selected == &persona && score == selected->score &&
                 c->persona_slot != o->candidates[selected->candidate].persona_slot
                    ? c->persona_slot < o->candidates[selected->candidate].persona_slot
                    : better(o, score, ci, ti, selected)))
                *selected = (struct bsim_choice){score, (uint16_t)ci, (uint16_t)ti};
        }
    }
    if (!o->switched && p4_strategy_switch(best.score, persona.score, 0)) best = persona;
    if (best.score == INT64_MIN) return 0;
    *result = best;
    return 1;
}
