/* Semantic battle events: include/p4pc/battle_semantic.h. Decoding follows the
 * hit rows btlTargetResolveSelectedSkill (0x001F14F0) writes and
 * btlTargetApplyHpSpChange (overrides/001f3010.c) commits:
 *   effect 1 landed, 2/4 missed or evaded, 0x100 nulled, 0x400 drained;
 *   target_flags (shared by a target's rows) 4 weak, 2 critical, 1 plain;
 *   result_flags 0x10 endured, 0x80 guard toggle, 0x100 ailment tick;
 *   a reflected hit is re-resolved against the attacker: source == target.
 * Self packets that are not reflected damage (regen, ticks, guard, recovery,
 * a drain skill's heal) are no damage event. */
#include "p4pc/battle_semantic.h"
#include "p4pc/battle_notify.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include "p4pc/mod_entry.h"
#include <string.h>

extern uint32_t datPersonaGetExpById(int16_t unitId);
extern uint8_t datLevelFromExp(int32_t exp);

#define QUEUE 256u

static const char *const names[P4_SEMANTIC_KIND_COUNT] = {
    "turn_start", "skill_used", "damage", "miss", "ailment", "knockdown", "ko", "revive",
    "one_more", "reward", "level_up", "skill_learned"
};
static unsigned interest[P4_SEMANTIC_KIND_COUNT];
static P4SemanticEvent queue[QUEUE];
static size_t queued;
static uint32_t dropped, sequence;
static uint32_t observer;

int p4_semantic_kind(const char *name)
{
    for (int kind = 0; name && kind < P4_SEMANTIC_KIND_COUNT; ++kind)
        if (!strcmp(name, names[kind])) return kind;
    return -1;
}
const char *p4_semantic_name(int kind)
{
    return kind >= 0 && kind < P4_SEMANTIC_KIND_COUNT ? names[kind] : NULL;
}

/* What the current action's hits did, for a One More's cause. */
static struct { uint32_t uid; uint8_t weak, critical; } action_hits;

static void forward_admission(const P4BattleAdmission *event, void *context) { (void)context; p4_semantic_admission(event); }
static void forward_command(const P4BattleCommand *event, void *context) { (void)context; p4_semantic_command(event); }
static void forward_hit(const P4BattleHit *event, void *context) { (void)context; p4_semantic_hit(event); }

int p4_semantic_interest(int kind, int delta)
{
    if (kind < 0 || kind >= P4_SEMANTIC_KIND_COUNT || (delta < 0 && interest[kind] < (unsigned)-delta)) return -1;
    interest[kind] += (unsigned)delta;
    unsigned any = 0;
    for (int i = 0; i < P4_SEMANTIC_KIND_COUNT; ++i) any |= interest[i];
    /* The moments the observer does not carry: One More commits and the battle result. */
    const uint32_t notify = (interest[P4_SEMANTIC_ONE_MORE] ? P4_NOTIFY_ONE_MORE : 0u) |
        (interest[P4_SEMANTIC_REWARD] || interest[P4_SEMANTIC_LEVEL_UP] ? P4_NOTIFY_REWARD : 0u) |
        (interest[P4_SEMANTIC_LEVEL_UP] || interest[P4_SEMANTIC_SKILL_LEARNED] ? P4_NOTIFY_LEVEL_UP : 0u);
    static const P4BattleNotifySink sink = {p4_semantic_one_more, p4_semantic_reward, p4_semantic_level_up};
    p4_battle_notify_sink(notify ? &sink : NULL, notify);
    if (any && !observer) {
        static const P4BattleObserver forwarder = {NULL, forward_admission, NULL, forward_hit, forward_command, NULL};
        observer = p4_battle_observer_add(&forwarder);
        if (!observer) { interest[kind] -= (unsigned)delta; return -1; }
    } else if (!any && observer) {
        p4_battle_observer_remove(observer);
        observer = 0;
        queued = 0;
    }
    return 0;
}

static P4SemanticUnit unit_of(const P4BattleActor *actor)
{
    return (P4SemanticUnit){actor->unit_uid, actor->native_id, actor->side, actor->present};
}
static P4SemanticEvent *push(int kind)
{
    if (!interest[kind]) return NULL;
    if (queued == QUEUE) { ++dropped; return NULL; }
    P4SemanticEvent *event = &queue[queued++];
    memset(event, 0, sizeof *event);
    event->sequence = ++sequence;
    event->kind = (uint8_t)kind;
    p4_mod_safe_point |= P4_MOD_SAFE_SEMANTIC;
    return event;
}

void p4_semantic_admission(const P4BattleAdmission *admission)
{
    P4SemanticEvent *event = push(P4_SEMANTIC_TURN_START);
    if (!event) return;
    event->source = unit_of(&admission->actor);
    event->flag = admission->queue_kind == P4_BATTLE_QUEUE_FOLLOWUP;
}
void p4_semantic_command(const P4BattleCommand *command)
{
    /* Attack, skill and item commands; Persona switches and Analyze are free
     * actions with their own events. */
    if (command->free_action || command->action_kind < 1 || command->action_kind > 3) return;
    action_hits.uid = command->actor.unit_uid;
    action_hits.weak = action_hits.critical = 0;
    P4SemanticEvent *event = push(P4_SEMANTIC_SKILL_USED);
    if (!event) return;
    event->source = unit_of(&command->actor);
    event->skill = command->skill_id;
    event->item = command->item_id;
    event->action_kind = command->action_kind;
}
static void status_event(int kind, const P4BattleHit *hit, uint32_t bit, int applied)
{
    P4SemanticEvent *event = push(kind);
    if (!event) return;
    event->source = unit_of(&hit->source);
    event->target = unit_of(&hit->target);
    event->skill = hit->skill_id;
    event->action_kind = hit->action_kind;
    event->status = bit;
    event->flag = (uint8_t)applied;
}
void p4_semantic_hit(const P4BattleHit *hit)
{
    const int self = hit->source.action_uid == hit->target.action_uid;
    if (!self && hit->source.unit_uid == action_hits.uid && hit->effect == 1) {
        action_hits.weak |= (hit->target_flags & 4u) != 0;
        action_hits.critical |= (hit->target_flags & 2u) != 0;
    }
    const int reflected = self && hit->requested_hp_delta < 0 && !(hit->result_flags & 0x180u) && hit->effect == 1;
    if (!self && (hit->effect == 2 || hit->effect == 4)) {
        P4SemanticEvent *event = push(P4_SEMANTIC_MISS);
        if (event) {
            event->source = unit_of(&hit->source);
            event->target = unit_of(&hit->target);
            event->skill = hit->skill_id;
            event->action_kind = hit->action_kind;
        }
        return;
    }
    const int damaging = hit->effect == 0x100 || hit->effect == 0x400 || (hit->effect == 1 && hit->requested_hp_delta < 0);
    if ((!self || reflected) && damaging) {
        P4SemanticEvent *event = push(P4_SEMANTIC_DAMAGE);
        if (event) {
            event->source = unit_of(&hit->source);
            event->target = unit_of(&hit->target);
            event->skill = hit->skill_id;
            event->action_kind = hit->action_kind;
            event->hp_before = hit->hp_before;
            event->hp_after = hit->hp_after;
            event->amount = (int32_t)hit->hp_before - (int32_t)hit->hp_after;
            event->result = hit->effect == 0x100 ? P4_SEMANTIC_NULL : hit->effect == 0x400 ? P4_SEMANTIC_DRAIN :
                            hit->target_flags & 4u ? P4_SEMANTIC_WEAK : hit->target_flags & 2u ? P4_SEMANTIC_CRITICAL :
                            P4_SEMANTIC_HIT;
            event->flag = (uint8_t)reflected;
            event->endured = (hit->result_flags & 0x10u) && hit->hp_after == 1;
        }
    }
    if (!hit->statuses_applied) return;
    const uint32_t before = hit->status_before, after = hit->status_after;
    const uint32_t applied = after & ~before & P4_SEMANTIC_AILMENT_BITS;
    const uint32_t removed = before & ~after & P4_SEMANTIC_AILMENT_BITS;
    for (uint32_t bit = 1; bit & P4_SEMANTIC_AILMENT_BITS; bit <<= 1) {
        if (applied & bit) status_event(P4_SEMANTIC_AILMENT, hit, bit, 1);
        if (removed & bit) status_event(P4_SEMANTIC_AILMENT, hit, bit, 0);
    }
    if (!(before & P4_SEMANTIC_DOWN_BIT) && (after & P4_SEMANTIC_DOWN_BIT))
        status_event(P4_SEMANTIC_KNOCKDOWN, hit, P4_SEMANTIC_DOWN_BIT, 1);
    if (!(before & P4_SEMANTIC_KO_BIT) && (after & P4_SEMANTIC_KO_BIT))
        status_event(P4_SEMANTIC_KO, hit, P4_SEMANTIC_KO_BIT, 1);
    if ((before & P4_SEMANTIC_KO_BIT) && !(after & P4_SEMANTIC_KO_BIT))
        status_event(P4_SEMANTIC_REVIVE, hit, P4_SEMANTIC_KO_BIT, 0);
}

/* BtlUnit (+0xA2 genus, +0xA4 ID, +0xA8 uid) of a BtlAction (+0x30). */
static P4SemanticUnit unit_of_action(uint32_t action)
{
    const uint32_t unit = action ? P4_G32(action + 0x30u) : 0;
    if (!unit) return (P4SemanticUnit){0};
    return (P4SemanticUnit){P4_G32(unit + 0xA8u), P4_G16(unit + 0xA4u), (uint8_t)(P4_G8(unit + 0xA2u) != 0), 1};
}
static P4SemanticUnit member_unit(uint32_t member) { return (P4SemanticUnit){0, (uint16_t)member, 0, 0}; }

void p4_semantic_one_more(uint32_t action)
{
    P4SemanticEvent *event = push(P4_SEMANTIC_ONE_MORE);
    if (!event) return;
    event->source = unit_of_action(action);
    const int same = event->source.uid == action_hits.uid;
    event->result = (uint8_t)(same && action_hits.weak ? P4_SEMANTIC_CAUSE_WEAKNESS
                              : same && action_hits.critical ? P4_SEMANTIC_CAUSE_CRITICAL : P4_SEMANTIC_CAUSE_DOWN);
}

/* BtlXpAwards: heroExp +4, partyIds[4] u16 +0x69A, partyExp[4] +0x6A4. BtlXpReward: items[3]
 * {s16 id, s16 count} +0x24, itemCount +0x30. */
void p4_semantic_reward(uint32_t awards, uint32_t reward, int32_t money, uint32_t hero_exp_before)
{
    P4SemanticEvent *event = push(P4_SEMANTIC_REWARD);
    if (event) {
        event->amount = money;
        event->exp = (int32_t)P4_G32(awards + 4u);
        const uint32_t items = P4_G32(reward + 0x30u);
        for (uint32_t i = 0; i < 3u && i < items; ++i) {
            const uint16_t id = P4_G16(reward + 0x24u + 4u * i), count = P4_G16(reward + 0x26u + 4u * i);
            if (!id || !count) continue;
            event->items[event->item_count] = id;
            event->item_counts[event->item_count++] = count;
        }
        for (uint32_t i = 0; i < 4u; ++i) {
            const uint16_t id = P4_G16(awards + 0x69Au + 2u * i);
            const int32_t exp = (int32_t)P4_G32(awards + 0x6A4u + 4u * i);
            if (!id || !exp) continue;
            event->party[event->party_count] = id;
            event->party_exp[event->party_count++] = exp;
        }
    }
    /* The protagonist's own level (btlXpApplyAwards sets it from his EXP). */
    if (!interest[P4_SEMANTIC_LEVEL_UP]) return;
    const uint8_t before = datLevelFromExp((int32_t)hero_exp_before);
    const uint8_t after = datLevelFromExp((int32_t)datPersonaGetExpById(1));
    if (after <= before) return;
    event = push(P4_SEMANTIC_LEVEL_UP);
    if (!event) return;
    event->target = member_unit(1);
    event->level = after;
    event->from = before;
}

/* DatPersonaRecord: id +2, level +4, skills[8] +0x0C. Level-up delta: levels gained +0, added[32] +2. */
void p4_semantic_level_up(uint32_t member, uint32_t persona, uint32_t delta)
{
    const uint16_t id = P4_G16(persona + 2u);
    const uint8_t level = P4_G8(persona + 4u), gained = P4_G8(delta);
    P4SemanticEvent *event = push(P4_SEMANTIC_LEVEL_UP);
    if (event) {
        event->target = member_unit(member);
        event->persona = id;
        event->level = level;
        event->from = (uint8_t)(level > gained ? level - gained : 0);
    }
    for (uint32_t i = 0; i < 32u; ++i) {
        const uint16_t skill = P4_G16(delta + 2u + 2u * i);
        if (!skill) continue;
        int known = 0;
        for (uint32_t s = 0; s < 8u; ++s) known |= P4_G16(persona + 0x0Cu + 2u * s) == skill;
        if (!known) continue; /* not learned: all 8 slots were full */
        event = push(P4_SEMANTIC_SKILL_LEARNED);
        if (!event) return;
        event->target = member_unit(member);
        event->persona = id;
        event->skill = skill;
    }
}

size_t p4_semantic_drain(P4SemanticEvent *out, size_t capacity, uint32_t *lost)
{
    size_t count = queued < capacity ? queued : capacity;
    memcpy(out, queue, count * sizeof *out);
    memmove(queue, queue + count, (queued - count) * sizeof *queue);
    queued -= count;
    if (lost) *lost = dropped;
    dropped = 0;
    return count;
}
