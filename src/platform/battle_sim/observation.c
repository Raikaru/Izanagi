#include "expert.h"
#include "policy_items.h"
#include "p4pc/gmem.h"
#include "p4pc/battle_strategy.h"
#include "p4pc/game_cost_stage_native.h"
#include "p4pc/game_skills_native.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

extern uint16_t datCalcMaxHpById(int16_t id);
extern uint16_t datCalcMaxSpById(int16_t id);
extern uint16_t datCalcUnderlyingSkillId(int64_t item);
extern int32_t datCalcDefaultSkillId(void *unit);
extern int32_t datCalcThresholdTrigger(void *unit, int32_t skill);
extern int32_t datCalcHasBuffMask(void *unit, int32_t mask);
extern uint32_t datCalcSkillCost(void *unit, int32_t skill);
extern int32_t datCalcSkillElement(void *unit, uint16_t skill);
extern int32_t btlTargetCheckCommandAvailability(void *action, int64_t kind);
extern uint16_t btlSkillClassifyTargets(void *action, void *selection, uint16_t skill, uint32_t reverse);
extern void btlEffectCollectTargetUnits(void *action, void *selection, uint32_t sides, uint32_t filters, uint32_t status);
extern int p4_battle_sim_is_boss(void);

extern void *datPersonaRosterSlot(int16_t slot);
extern int32_t datPersonaGetLeadSlot(void);
extern uint16_t datPersonaCountValidMembers(void);
extern uint16_t *datPersonaGetSkillsByPcId(uint16_t id);
static int is_damage_effect(unsigned e)
{
    return e == 1 || e == 4 || e == 8 || e == 10 || e == 12 || e == 13 || e == 14;
}


static int has_skill(uint32_t list, uint16_t skill)
{
    for (unsigned i = 0; i < 8; i++)
        if (P4_G16(list + i * 2) == skill) return 1;
    return 0;
}
static int add_candidate(struct bsim_observation *out, const uint32_t *actions,
                         uint32_t actor, uint32_t record, uint32_t scratch,
                         uint32_t skill_base, uint32_t skill_rows, uint16_t skill,
                         uint16_t item, uint8_t wheel, uint16_t menu_index)
{
    if (skill >= skill_rows || out->candidate_count >= BSIM_CANDIDATES_MAX) return 0;
    const uint32_t row = skill_base + skill * 0x28u;
    const unsigned classification = P4_G8(row + 8);
    const int group = classification == 1 || classification == 2;
    if (group) btlEffectCollectTargetUnits(P4_G2H(actor), P4_G2H(scratch), P4_G8(row + 9), P4_G8(row + 10), 0);
    else btlSkillClassifyTargets(P4_G2H(actor), P4_G2H(scratch), skill, 0);
    const unsigned count = P4_G16(scratch + 0x38);
    if (count > BSIM_UNITS_MAX) return 0;
    if (!count) return 1;
    struct bsim_candidate_observation *c = &out->candidates[out->candidate_count];
    c->persona_slot = UINT8_MAX;
    c->skill = skill; c->item = item; c->wheel = wheel; c->menu_index = menu_index;
    c->group = (uint8_t)group;
    c->buff_mask = P4_G32(row + 0x20); c->status_mask = P4_G32(row + 0x1c);
    c->cost_kind = P4_G8(row + 3); c->hp_effect = P4_G8(row + 0x11);
    c->sp_effect = P4_G8(row + 0x14); c->status_mode = P4_G8(row + 0x18);
    c->power = (int16_t)P4_G16(row + 0x12);
    c->element = (int16_t)datCalcSkillElement(P4_G2H(record), skill);
    c->cost = wheel == 4 ? datCalcSkillCost(P4_G2H(record), skill) : 0;
    const int queries_buff = c->buff_mask && !is_damage_effect(c->hp_effect);
    for (unsigned i = 0; i < count; i++) {
        const uint32_t target = P4_G32(scratch + i * 4);
        unsigned j = 0;
        for (; j < out->unit_count; j++) if (actions[j] == target) break;
        if (j == out->unit_count) return 0;
        c->targets |= (uint16_t)(1u << j);
        if (!i) c->first_target = (uint16_t)j;
        if (queries_buff &&
            (out->units[j].side == 1 || (out->units[j].native_id >= 1 && out->units[j].native_id <= 10))) {
            const uint32_t target_record = P4_G32(P4_G32(target + 0x30) + 0xa64);
            if (datCalcHasBuffMask(P4_G2H(target_record), (int32_t)c->buff_mask))
                c->held_targets |= (uint16_t)(1u << j);
        }
    }
    out->candidate_count++;
    return 1;
}

int bsim_observe(struct bsim_observation *out, uint32_t panel, uint32_t actor,
                 uint32_t scratch, uint32_t skill_base, uint32_t skill_rows,
                 const struct bsim_knowledge *knowledge, unsigned knowledge_count)
{
    if (!out || (!knowledge && knowledge_count)) return 0;
    memset(out, 0, sizeof *out);
    out->actor_index = UINT16_MAX;
    out->is_boss = (uint8_t)p4_battle_sim_is_boss();
    const uint32_t battle = P4_GPDATA(uint32_t, 0xb3ac);
    if (!battle || knowledge_count > BSIM_KNOWLEDGE_MAX) return 0;
    uint32_t actions[BSIM_UNITS_MAX];
    uint32_t action = P4_G32(battle + 0x174);
    for (unsigned n = 0; action && n < BSIM_UNITS_MAX; n++, action = P4_G32(action + 0x450)) {
        const uint32_t unit = P4_G32(action + 0x30);
        if (!unit) continue;
        const uint32_t record = P4_G32(unit + 0xa64);
        if (!record) continue;
        const uint8_t side = P4_G8(unit + 0xa2);
        if (side > 1) continue;
        const unsigned index = out->unit_count++;
        actions[index] = action;
        if (action == actor) out->actor_index = (uint16_t)index;
        struct bsim_unit_observation *v = &out->units[index];
        v->action_uid = (uint64_t)P4_G32(action) | (uint64_t)P4_G32(action + 4) << 32;
        v->unit_uid = P4_G32(unit + 0xa8);
        v->native_id = P4_G16(unit + 0xa4); v->side = side;
        /* Enemy KO and displayed Technical ailment icons are observable; other
         * native status bits do not belong in the policy snapshot. */
        v->status = P4_G32(record + 12) & (side ? 0x8001eu : UINT32_MAX);
        v->charged = side == 1 && !(v->status & 0x80000u) && (P4_G32(record + 0x14) & 0xc0000u) != 0;
        for (unsigned i = 0; i < 5; i++) {
            const unsigned packed = P4_G8(record + 0x1c + i / 2);
            const int nibble = (packed >> (4 * (i & 1))) & 15;
            v->buffs[i] = (int8_t)(nibble > 7 ? nibble - 15 : nibble);
        }
        if (!side && v->native_id >= 1 && v->native_id <= 10) {
            v->hp = P4_G16(record + 8); v->sp = P4_G16(record + 10);
            v->max_hp = datCalcMaxHpById((int16_t)v->native_id);
            v->max_sp = datCalcMaxSpById((int16_t)v->native_id);
        } else if (side == 1) {
            for (unsigned i = 0; i < knowledge_count; i++) {
                if (!knowledge[i].valid || knowledge[i].unit_uid != v->unit_uid || knowledge[i].native_id != v->native_id) continue;
                v->tried = knowledge[i].tried; v->weak = knowledge[i].weak; v->bad = knowledge[i].bad;
                v->vulnerable = knowledge[i].vulnerable;
                v->probed = knowledge[i].probed;
                v->probed2 = knowledge[i].probed2;
                break;
            }
        }
    }
    if (action || out->actor_index == UINT16_MAX) return 0;
    const uint32_t record = P4_G32(P4_G32(actor + 0x30) + 0xa64);
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 1) &&
        !add_candidate(out, actions, actor, record, scratch, skill_base, skill_rows,
                       (uint16_t)datCalcDefaultSkillId(P4_G2H(record)), 0, 3, 0)) return 0;
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 2)) {
        const unsigned count = P4_G16(panel + 0x1a4);
        if (count > 8) return 0;
        for (unsigned i = 0; i < count; i++) {
            const uint16_t skill = P4_G16(panel + 0x194 + i * 2);
            if (!datCalcThresholdTrigger(P4_G2H(record), skill) &&
                !add_candidate(out, actions, actor, record, scratch, skill_base, skill_rows, skill, 0, 4, (uint16_t)i)) return 0;
        }
    }
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 3)) {
        const unsigned count = P4_G16(panel + 0x5a6);
        if (count > BSIM_ITEMS_MAX) return 0;
        const P4GameItemBattleRow *items = p4_battle_policy_item_rows((uint8_t *)P4_G2H(panel), count);
        if (!items) return 0;
        for (unsigned i = 0; i < count; i++) {
            const uint16_t item = (uint16_t)items[i].id;
            if (!add_candidate(out, actions, actor, record, scratch, skill_base, skill_rows,
                               datCalcUnderlyingSkillId(item), item, 6, (uint16_t)i)) return 0;
        }
    }
    if (out->units[out->actor_index].native_id == 1 && btlTargetCheckCommandAvailability(P4_G2H(actor), 5)) {
        const int32_t lead = datPersonaGetLeadSlot();
        const unsigned count = datPersonaCountValidMembers();
        const uint16_t *active = datPersonaGetSkillsByPcId(1);
        if (lead < 0 || count > BSIM_PERSONAS_MAX || !active) return 0;
        out->lead_slot = (uint16_t)lead;
        const uint32_t active_list = P4_H2G(active);
        const int cost_stage = p4_game_cost_native_active();
        for (unsigned slot = 0; slot < count; slot++) {
            void *host = datPersonaRosterSlot((int16_t)slot);
            if (!host) continue;
            const uint32_t persona = P4_H2G(host);
            struct bsim_persona_observation *p = &out->personas[out->persona_count++];
            p->slot = (uint8_t)slot;
            p->id = P4_G16(persona + 2);
            for (unsigned j = 0; j < 5; j++) p->stats[j] = P4_G8(persona + 28 + j);
            for (unsigned j = 0; j < 8; j++) p->skills[j] = P4_G16(persona + 12 + j * 2);
            if ((int32_t)slot == lead || !p->id) continue;
            for (unsigned j = 0; j < 8; j++) {
                const uint16_t skill = p->skills[j];
                if (!skill || skill >= skill_rows || !p4_game_skill_active(skill)) continue;
                const uint32_t row = skill_base + skill * 0x28u;
                if (!(P4_G8(row + 1) & 2)) continue;
                if (!p4_strategy_switch_priceable(P4_G8(row + 3), (P4_G8(row) & 0x10) != 0, cost_stage,
                                                  has_skill(active_list, P4_STRATEGY_SPELL_MASTER),
                                                  has_skill(persona + 12, P4_STRATEGY_SPELL_MASTER))) continue;
                if (datCalcThresholdTrigger(P4_G2H(record), skill)) continue;
                const unsigned before = out->candidate_count;
                if (!add_candidate(out, actions, actor, record, scratch, skill_base, skill_rows,
                                   skill, 0, 4, (uint16_t)j)) return 0;
                if (out->candidate_count != before) out->candidates[before].persona_slot = (uint8_t)slot;
            }
        }
    }
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 7)) {
        if (out->candidate_count >= BSIM_CANDIDATES_MAX) return 0;
        struct bsim_candidate_observation *guard = &out->candidates[out->candidate_count++];
        guard->wheel = 2;
        guard->persona_slot = UINT8_MAX;
    }
    return 1;
}

void bsim_trace_decision(void *stream, const struct bsim_observation *o,
                         const struct bsim_choice *choice, unsigned policy, unsigned update)
{
    FILE *f = stream;
    if (!f) return;
    fprintf(f, "{\"kind\":\"policy_decision\",\"update\":%u,\"policy\":%u,\"actor\":%u,"
               "\"is_boss\":%u,\"setup_pending\":%u,\"lead_slot\":%u,\"units\":[",
            update, policy, o->actor_index, o->is_boss, o->setup_pending, o->lead_slot);
    for (unsigned i = 0; i < o->unit_count; i++) {
        const struct bsim_unit_observation *u = &o->units[i];
        fprintf(f, "%s{\"id\":%u,\"uid\":%u,\"side\":%u,\"hp\":%u,\"max_hp\":%u,\"sp\":%u,\"max_sp\":%u,\"status\":%u,"
                   "\"charged\":%u,\"tried\":%u,\"weak\":%u,\"bad\":%u,\"expired_buffs\":%u,"
                   "\"buffs\":[%d,%d,%d,%d,%d]}",
                i ? "," : "", u->native_id, u->unit_uid, u->side, u->hp, u->max_hp, u->sp, u->max_sp,
                u->side == 0 ? u->status : u->status & 0x8001eu,
                u->charged, u->tried, u->weak, u->bad, u->expired_buffs,
                u->buffs[0], u->buffs[1], u->buffs[2], u->buffs[3], u->buffs[4]);
    }
    fputs("],\"personas\":[", f);
    for (unsigned i = 0; i < o->persona_count; i++) {
        const struct bsim_persona_observation *p = &o->personas[i];
        fprintf(f, "%s{\"slot\":%u,\"id\":%u,\"stats\":[%u,%u,%u,%u,%u]}",
                i ? "," : "", p->slot, p->id, p->stats[0], p->stats[1], p->stats[2], p->stats[3], p->stats[4]);
    }
    fputs("],\"candidates\":[", f);
    for (unsigned i = 0; i < o->candidate_count; i++) {
        const struct bsim_candidate_observation *c = &o->candidates[i];
        fprintf(f, "%s{\"wheel\":%u,\"skill\":%u,\"item\":%u,\"menu_index\":%u,\"persona_slot\":%u,\"targets\":%u,"
                   "\"group\":%u,\"first_target\":%u,\"element\":%d,\"hp_effect\":%u,"
                   "\"power\":%d,\"cost_kind\":%u,\"cost\":%u,\"buff_mask\":%u,\"held_targets\":%u}",
                i ? "," : "", c->wheel, c->skill, c->item, c->menu_index, c->persona_slot, c->targets,
                c->group, c->first_target, c->element, c->hp_effect, c->power,
                c->cost_kind, c->cost, c->buff_mask, c->held_targets);
    }
    fprintf(f, "],\"selected\":{\"candidate\":%u,\"target\":%u}}\n", choice->candidate, choice->target);
}
