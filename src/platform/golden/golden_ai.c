/* Clean-room implementation of ec69d7898 ai-commands.md. This semantic
 * engine deliberately knows neither Switch offsets nor host/guest pointers. */
#include "golden_ai.h"
#include <stdio.h>
#include <stdlib.h>

static void require(int condition)
{
    if (!condition) {
        fputs("p4pc: Golden AI contract/backing failure\n", stderr);
        abort();
    }
}
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)u16(p) | (uint32_t)u16(p + 2) << 16;
}
static int has(const P4GoldenAiUnit *unit, uint16_t skill)
{
    require(unit && unit->skills);
    require((unit->flags & 4) ? unit->id < 368 : unit->id <= 10);
    for (unsigned i = 0; i < 8; ++i) if (unit->skills[i] == skill) return 1;
    if (!(unit->flags & 4) && unit->equipment_ids[2] >= 0) {
        require(unit->equipment[2] != NULL);
        return unit->equipment[2]->granted_skill == skill;
    }
    return 0;
}
uint32_t p4_golden_ai_recovery_mask(const P4GoldenAiUnit *unit)
{
    static const uint16_t skills[] = {0x219,0x21a,0x21b,0x21c,0x21d,0x21e,
        0x255,0x256,0x25b,0x25c,0x25d,0x257,0x258,0x259,0x25a,0x25f,0x260,0x261};
    uint32_t mask = 0;
    for (unsigned i = 0; i < sizeof skills / sizeof skills[0]; ++i)
        if (has(unit,skills[i])) mask |= 1u << i;
    return mask;
}
static int effect(const P4GoldenAiUnit *unit, unsigned slot, unsigned id)
{
    if (unit->flags & 4) return 0;
    require(unit->equipment[slot] != NULL);
    return unit->equipment[slot]->effect == id;
}
static int effect_either(const P4GoldenAiUnit *unit, unsigned id)
{
    return effect(unit, 0, id) || effect(unit, 1, id);
}
uint32_t p4_golden_ai_maximum(const P4GoldenAiUnit *unit, int sp)
{
    require(unit != NULL);
    uint32_t base = sp ? unit->base_sp : unit->base_hp, value = base;
    unsigned first = sp ? 0x213 : 0x210;
    for (unsigned i = 0; i < 3; ++i)
        if (has(unit, (uint16_t)(first + i))) value += base * (10u * (i + 1)) / 100u;
    if (has(unit, (uint16_t)(sp ? 0x24e : 0x24d))) value += base / 2u;
    if (has(unit, (uint16_t)(sp ? 0x22d : 0x22c))) value += base;
    if (has(unit, (uint16_t)(sp ? 0x22c : 0x22d))) value -= base / 2u;
    for (unsigned slot = 0; slot < 2; ++slot) {
        for (unsigned i = 0; i < 5; ++i)
            if (effect(unit, slot, (sp ? 0x1f : 0x1a) + i)) value += 10u * (i + 1);
        if (effect(unit, slot, sp ? 0x8a : 0x5b)) value += 100;
    }
    if (!(unit->flags & 4) && value > 999) value = 999;
    return value;
}
static const uint8_t *row(const P4GoldenAi *ai, uint16_t skill)
{
    require(ai && ai->skill_rows && skill < 472);
    return ai->skill_rows + 44u * skill;
}
uint32_t p4_golden_ai_cost(const P4GoldenAi *ai, const P4GoldenAiUnit *unit, uint16_t skill)
{
    const uint8_t *r = row(ai, skill);
    require(unit != NULL);
    uint32_t type = r[6], a = u16(r + 8), b = u16(r + 10), cost;
    if (type == 1) {
        if (unit->flags & 4) return 0;
        cost = a * (uint16_t)p4_golden_ai_maximum(unit, 0) / 100u + b;
        if (has(unit, 0x22b)) cost >>= 1;
    } else if (type == 2) {
        if ((unit->flags & 4) && (unit->enemy_flags & 2)) return 0;
        cost = (u32(r) & 0x10) ? a * (uint16_t)p4_golden_ai_maximum(unit, 1) / 100u + b : a + b;
        if (!cost) return 0;
        if (effect_either(unit, 0x8c)) cost *= 3;
        if (has(unit, 0x22a)) cost >>= 1;
    } else return 0;
    return cost ? cost : 1;
}
uint32_t p4_golden_ai_trigger(const P4GoldenAi *ai, const P4GoldenAiUnit *unit, uint16_t skill)
{
    const uint8_t *r = row(ai, skill);
    uint32_t flags = u32(r);
    if (flags & 0x200) return 0;
    require(unit != NULL);
    if (unit->status & 8) return 4;
    uint32_t cost = p4_golden_ai_cost(ai, unit, skill);
    if (r[6] == 1) return (flags & 1) ? unit->hp < cost : unit->hp <= cost;
    if (r[6] == 2 && unit->sp < cost) return 2;
    return 0;
}
int p4_golden_ai_affinity_bit29(const P4GoldenAiUnit *unit, int16_t index)
{
    if (index == -1 || (index >= 16 && index <= 18)) return 0;
    require(index >= 0 && index < 16 && unit && unit->affinity);
    require((unit->flags & 4) ? unit->affinity_id < 368 : unit->affinity_id < 256);
    uint16_t entry = unit->affinity[index];
    if (!(entry & 0x2000)) return 0;
    if (index < 1 || index > 4 || !(entry & 0x800) || (unit->flags & 0x200)) return 1;
    unsigned n = (unit->secondary[(index - 1) / 2] >> (4 * ((index - 1) & 1))) & 15;
    if (n < 1 || n > 7) return 1;
    unsigned skill = 0x1dc + 4u * (unsigned)(index - 1);
    static const uint16_t equip_a[4] = {0x90,0x91,0x93,0x92};
    unsigned equipment = equip_a[index - 1];
    if (!(entry & 0x700) && (has(unit, (uint16_t)skill) || effect_either(unit, equipment))) return 1;
    if (!(entry & 0x600) && (has(unit, (uint16_t)(skill + 1)) || has(unit, 0x1fb))) return 1;
    if (!(entry & 0x400) && (has(unit, (uint16_t)(skill + 2)) || effect_either(unit, equipment + 4))) return 1;
    return has(unit, (uint16_t)(skill + 3)) || effect_either(unit, equipment + 8);
}
void p4_golden_ai_battle_categories(P4GoldenAiBattle *state, uint32_t flags,
                                   int32_t guard, uint16_t category)
{
    require(state != NULL);
    state->flags = flags;
    state->category_guard = guard;
    state->category = category;
}
void p4_golden_ai_battle_assist(P4GoldenAiBattle *state, uint16_t parameter)
{
    require(state != NULL);
    state->assist_choreography = parameter;
}
void p4_golden_ai_battle_eligible_start(P4GoldenAiBattle *state)
{
    require(state != NULL);
    /* Defined signed interpretation of the wrapping 32-bit counter. */
    if (state->category_guard == INT32_MAX) state->category_guard = INT32_MIN;
    else ++state->category_guard;
}
void p4_golden_ai_battle_continuous_entry(P4GoldenAiBattle *state)
{
    require(state != NULL);
    state->category_guard = 0;
}
uint16_t p4_golden_ai_assist_selector(const P4GoldenAiBattle *state)
{
    require(state != NULL);
    return state->assist_choreography == 1 ? 0x21 : state->assist_choreography == 3 ? 0x22 : 0x20;
}
int p4_golden_ai_message_wait_latch(P4GoldenAiEntity *actor)
{
    require(actor != NULL);
    if (!actor->message_parameter) return 0;
    int snapshot = !!(actor->message_flags & 2);
    actor->message_flags &= (uint8_t)~2u;
    return snapshot;
}
int p4_golden_ai_arity(P4GameMode mode, uint32_t command)
{
    if (mode != P4_GAME_MODE_GOLDEN) return -1;
    switch (command) {
    case 8468: case 8471: return 2;
    case 8472: case 8474: return 0;
    case 148: case 8469: case 8470: case 8473: case 8479:
    case 8481: case 8484: case 8485: return 1;
    default: return -1;
    }
}
static uint32_t random_index(P4GoldenAi *ai, unsigned count)
{
    require(ai->additive && count);
    return p4_golden_ai_rng_next(ai->additive) % count;
}
static int alive(const P4GoldenAiUnit *unit)
{
    require(unit != NULL);
    return !(unit->status & 0x80000) && unit->hp > 0;
}
static unsigned opposite(P4GoldenAi *ai, P4GoldenAiEntity **out)
{
    require(ai->actor && ai->actor->unit);
    unsigned side = ai->actor->unit->side, count = 0;
    for (P4GoldenAiEntity *e = ai->head; e; e = e->next) {
        require(e->unit != NULL);
        if (side > 1 || (e->flags & 9) != 9 || e->unit->side != (side ^ 1) ||
            !(e->unit->target_flags & 8) || !alive(e->unit)) continue;
        /* Retail's unchecked writes do not define a BF result on overflow.
         * Native execution terminates instead of manufacturing one. */
        require(count < 12);
        out[count++] = e;
    }
    return count;
}
static int16_t element(const P4GoldenAi *ai, const P4GoldenAiUnit *unit, uint16_t skill)
{
    require(ai->skill_elements && skill < 624);
    if (skill < 472 && unit && (u32(row(ai, skill)) & 2)) {
        if (unit->flags & 4) return unit->enemy_element;
        require(unit->equipment[0] != NULL);
        return (int16_t)unit->equipment[0]->element;
    }
    return (int8_t)ai->skill_elements[2u * skill];
}
static int64_t choose_skill(P4GoldenAi *ai, int16_t requested, int32_t preference)
{
    require(ai->actor && ai->actor->unit);
    P4GoldenAiUnit *unit = ai->actor->unit;
    require(unit->skills != NULL);
    uint16_t a[8], b[8];
    unsigned na = 0, nb = 0;
    if (element(ai, unit, 0) == requested) a[na++] = 0;
    for (unsigned i = 0; i < 8; ++i) {
        uint16_t skill = unit->skills[i];
        if (!skill || skill >= 472 || element(ai, unit, skill) != requested ||
            (unit->status & 0x80008) || !has(unit, skill) || p4_golden_ai_trigger(ai, unit, skill)) continue;
        if (row(ai, skill)[12]) { require(nb < 8); b[nb++] = skill; }
        else { require(na < 8); a[na++] = skill; }
    }
    if (!na && !nb) return -1;
    int64_t sa = na ? a[random_index(ai, na)] : -1;
    int64_t sb = nb ? b[random_index(ai, nb)] : -1;
    int prefer_a = preference == 1;
    if (preference != 0 && preference != 1) prefer_a = random_index(ai, 100) > 50;
    return prefer_a ? (na ? sa : sb) : (nb ? sb : sa);
}
int p4_golden_ai_execute(P4GameMode mode, P4GoldenAi *ai, uint32_t command,
                         const int64_t *args, P4GoldenAiResult *result)
{
    int arity = p4_golden_ai_arity(mode, command);
    if (arity < 0) return 0;
    require(ai && result && (!arity || args));
    int64_t value = -1;
    switch (command) {
    case 148:
        require(ai->sfmt != NULL);
        value = p4_golden_rng2(ai->sfmt, args[0]);
        break;
    case 8468: {
        uint32_t id = (uint32_t)args[0] & 0xfffffff, mask = (uint32_t)args[1] & 0xffffff;
        P4GoldenAiEntity *e = ai->head;
        while (e && e->id != id) e = e->next;
        require(e && e->unit);
        value = !!(e->unit->status & mask);
        break;
    }
    case 8469: case 8481:
        require(ai->actor != NULL);
        ai->actor->message_parameter = (uint16_t)args[0];
        ai->actor->action_kind = command == 8469 ? 8 : 7;
        if (command == 8481) ai->actor->message_flags |= 2;
        return 1; /* void: preserve both BF return fields */
    case 8470: {
        uint16_t p = (uint16_t)args[0];
        P4GoldenAiEntity *candidates[6];
        unsigned count = 0;
        for (P4GoldenAiEntity *e = ai->head; e; e = e->next) {
            require(e->unit != NULL);
            if ((e->flags & 9) != 9 || e->unit->side != 1 || !alive(e->unit) ||
                !(p <= 4 || p == 6 || p == 7)) continue;
            uint32_t bit = 8u * e->unit->character_id + p;
            require(ai->knowledge && bit / 32 < ai->knowledge_words);
            if (ai->knowledge[bit / 32] & (1u << (bit & 31))) continue;
            require(count < 6);
            candidates[count++] = e;
        }
        if (count) value = (uint32_t)(candidates[random_index(ai, count)]->id | 0x80000000u);
        break;
    }
    case 8471:
        value = choose_skill(ai, (int16_t)args[0], (int32_t)args[1]);
        break;
    case 8472: case 8474:
        require(ai->battle != NULL);
        value = !!(ai->battle->flags & 0x20000000);
        if (command == 8472) {
            if (ai->battle->category_guard) value = -1;
            else if (ai->battle->category == 1 || ai->battle->category == 2) value = ai->battle->category;
            else value *= 3;
        }
        break;
    case 8473: {
        require(ai->actor && ai->actor->unit);
        uint16_t mask = (uint16_t)(1u << (ai->actor->unit->side & 31));
        uint32_t current = 0, maximum = 0;
        for (P4GoldenAiEntity *e = ai->head; e; e = e->next) {
            require(e->unit != NULL);
            if ((e->flags & 9) != 9 || !(mask & (1u << (e->unit->side & 31))) || !alive(e->unit)) continue;
            current += e->unit->sp;
            maximum += (uint16_t)p4_golden_ai_maximum(e->unit, 1);
        }
        value = current * 100u <= maximum * ((uint32_t)args[0] & 0xffffff);
        break;
    }
    case 8479:
        p4_golden_ai_battle_assist(ai->battle, (uint16_t)args[0]);
        return 1;
    case 8484: case 8485: {
        P4GoldenAiEntity *candidates[12] = {0}, *survivors[12];
        unsigned count = opposite(ai, candidates), accepted = 0;
        if (command == 8485) {
            for (unsigned i = 0; i < 12; ++i) ai->actor->candidates[i] = candidates[i];
            ai->actor->candidate_count = (uint16_t)count;
            ai->actor->candidates_initialized = 1;
        }
        for (unsigned i = 0; i < count; ++i) {
            P4GoldenAiEntity *e = candidates[i];
            if (command == 8484) {
                if ((e->flags & 9) == 9 && !(e->unit->status & 0x80000) &&
                    p4_golden_ai_affinity_bit29(e->unit, (int16_t)args[0])) ++accepted;
            } else if ((e->flags & 1) && !(e->unit->status & 0x800ff) &&
                       !p4_golden_ai_affinity_bit29(e->unit, (int16_t)args[0])) survivors[accepted++] = e;
        }
        if (command == 8484) value = (uint16_t)accepted;
        else if (accepted) {
            P4GoldenAiEntity *selected = survivors[accepted == 1 ? 0 : random_index(ai, accepted)];
            ai->actor->selected = selected;
            ai->actor->selection = 1;
            value = (uint32_t)(selected->id | 0x80000000u);
        }
        break;
    }
    }
    result->value = value;
    result->type = 0;
    return 1;
}
