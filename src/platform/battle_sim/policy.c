/* Tool party policies. Native menus validate/commit every choice and native
 * target collectors determine legality. No damage preview, affinity-table read,
 * speculative state mutation, rollback, host RNG or Lua is used here.
 * 1 greedy, 2 cautious, 3 typical, 5 attentive: a careful player who guards
 * while an enemy is visibly charged (public DatUnit +0x14 & 0xC0000), keeps the
 * protagonist above 70% and allies above 50%, revives, explores elements to find
 * weaknesses, remembers elements a target was seen absorbing or nulling, and
 * focuses the last damaged enemy and any enemy seen healing its side. */
#include "p4pc/battle_sim.h"
#include "p4pc/battle_events.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"
#include <limits.h>
#include <stdint.h>

#define GP(off) P4_GPDATA(uint32_t, off)

enum { WHEEL_ATTACK = 3, WHEEL_SKILL = 4, WHEEL_ITEM = 6, WHEEL_GUARD = 2 };
typedef struct Choice {
    int64_t score;
    uint32_t target;
    uint64_t target_uid;
    uint16_t skill, item, wheel, menu_index;
} Choice;
static struct {
    uint32_t mode, latency, scratch, skill_base, skill_rows, observer;
    uint32_t actor, decision_update;
    uint64_t actor_uid;
    int pending;
    const char *error;
    Choice choice;
    struct {
        uint32_t unit_uid, tried, weak;
        uint32_t unchanged, bad; /* attentive: shown no damage once / absorbed or nulled (public outcome) */
        uint16_t native_id;
        uint8_t valid;
    } knowledge[64];
    unsigned next_knowledge;
    /* Attentive (5) only, from observed hits: the enemy the party last damaged,
     * and enemies seen healing an ally. */
    uint32_t focus_uid, healer_uid[8];
    unsigned healers;
} policy;

extern int32_t btlTargetCheckCommandAvailability(void *action, int64_t kind);
extern int32_t datCalcThresholdTrigger(void *unit, int32_t skill);
extern uint32_t datCalcSkillCost(void *unit, int32_t skill);
extern int32_t datCalcSkillElement(void *unit, uint16_t skill);
extern int32_t datCalcDefaultSkillId(void *unit);
extern uint16_t datCalcUnderlyingSkillId(int64_t item);
extern uint16_t datCalcMaxHpById(int16_t id);
extern uint16_t datCalcMaxSpById(int16_t id);
extern int32_t datCalcHasBuffMask(void *unit, int32_t mask);
extern uint16_t btlSkillClassifyTargets(void *action, void *selection, uint16_t skill, uint32_t reverse);
extern void btlEffectCollectTargetUnits(void *action, void *selection, uint32_t sides, uint32_t filters, uint32_t status);
extern void btlFormationEntrySetSelected(int32_t selection, uint64_t *key);
extern void p4pc_pad_inject(unsigned bits, unsigned frames);

static uint64_t action_uid(uint32_t action)
{
    return (uint64_t)P4_G32(action) | (uint64_t)P4_G32(action + 4) << 32;
}
static uint32_t record_of(uint32_t action)
{
    return P4_G32(P4_G32(action + 0x30) + 0xa64);
}
static int is_healing(unsigned effect)
{
    return effect == 2 || effect == 5 || effect == 7 || effect == 9 || effect == 11 || effect == 15;
}
static int is_damage(unsigned effect)
{
    return effect == 1 || effect == 4 || effect == 8 || effect == 10 || effect == 12 || effect == 13 || effect == 14;
}
static unsigned percent(unsigned current, unsigned maximum)
{
    if (!maximum) return 0;
    return current >= maximum ? 100 : current * 100 / maximum;
}
static unsigned knowledge_slot(uint32_t uid, uint16_t id, int create)
{
    for (unsigned i = 0; i < 64; i++)
        if (policy.knowledge[i].valid && policy.knowledge[i].unit_uid == uid && policy.knowledge[i].native_id == id) return i;
    if (!create) return 64;
    unsigned i = policy.next_knowledge++ % 64;
    policy.knowledge[i].unit_uid = uid;
    policy.knowledge[i].native_id = id;
    policy.knowledge[i].tried = policy.knowledge[i].weak = 0;
    policy.knowledge[i].unchanged = policy.knowledge[i].bad = 0;
    policy.knowledge[i].valid = 1;
    return i;
}
static void observe_hit(const P4BattleHit *event, void *context)
{
    (void)context;
    /* These are deliberately the only outcome fields fed into policy memory.
     * Enemy exact HP/SP, hidden stats/affinities, input and RNG stay excluded. */
    if (event->skill_id >= policy.skill_rows) return;
    const uint32_t row = policy.skill_base + event->skill_id * 0x28u;
    if (event->source.side == 1 && event->target.side == 1) {
        if (policy.mode != 5 || !is_healing(P4_G8(row + 0x11))) return;
        for (unsigned i = 0; i < policy.healers; i++)
            if (policy.healer_uid[i] == event->source.unit_uid) return;
        if (policy.healers < 8) policy.healer_uid[policy.healers++] = event->source.unit_uid;
        return;
    }
    if (event->source.side != 0 || event->target.side != 1) return;
    if (!is_damage(P4_G8(row + 0x11))) return; /* KO channels are not damage knowledge. */
    policy.focus_uid = event->target.unit_uid;
    const uint32_t battle = GP(0xb3ac);
    if (!battle) return;
    uint32_t action = P4_G32(battle + 0x174);
    for (unsigned n = 0; action && n < 12; n++, action = P4_G32(action + 0x450)) {
        if (action_uid(action) != event->source.action_uid) continue;
        const int32_t element = datCalcSkillElement(P4_G2H(record_of(action)), event->skill_id);
        if (element < 0 || element >= 32) return;
        const unsigned slot = knowledge_slot(event->target.unit_uid, event->target.native_id, 1);
        const uint32_t bit = UINT32_C(1) << element;
        policy.knowledge[slot].tried |= bit; /* Attempted is not an affinity inference. */
        if (event->target_flags & 4u) policy.knowledge[slot].weak |= bit; /* Native WEAK, not CRITICAL. */
        /* What the screen shows: the target healed (absorb), or twice took no
         * damage (null; once could be a miss). */
        if (event->hp_after > event->hp_before) policy.knowledge[slot].bad |= bit;
        else if (event->hp_after == event->hp_before) {
            if (policy.knowledge[slot].unchanged & bit) policy.knowledge[slot].bad |= bit;
            policy.knowledge[slot].unchanged |= bit;
        }
        return;
    }
}
static void observe_admission(const P4BattleAdmission *event, void *context)
{
    (void)context;
    if (policy.pending && event->actor.action_uid == policy.actor_uid) policy.pending = 0;
}
static void observe_command(const P4BattleCommand *event, void *context)
{
    (void)context;
    if (policy.pending && event->actor.action_uid == policy.actor_uid) policy.pending = 0;
}

int p4_battle_policy_begin(uint32_t mode, uint32_t latency)
{
    policy.mode = mode;
    policy.latency = latency;
    if (!mode) return 1;
    if (mode > 3 && mode != 5) { policy.error = "unsupported_native_party_policy"; return 0; }
    if (!p4_game_table_extent_rows(0x007644a8u, 0x28, &policy.skill_base, &policy.skill_rows)) {
        policy.error = "native_policy_skill_catalog_unavailable"; return 0;
    }
    policy.scratch = p4_gport_alloc(0x40, 16); /* Full native BtlTargetSelection, not obsolete0x38. */
    if (!policy.scratch) { policy.error = "native_policy_target_scratch"; return 0; }
    const P4BattleObserver observer = {0, observe_admission, 0, observe_hit, observe_command, 0};
    policy.observer = p4_battle_observer_add(&observer);
    if (!policy.observer) { policy.error = "native_policy_observer_registration"; return 0; }
    return 1;
}
const char *p4_battle_policy_error(void)
{
    return policy.error;
}

static int64_t target_utility(uint32_t target, uint32_t row, int32_t element)
{
    const uint32_t unit = P4_G32(target + 0x30);
    const uint8_t side = P4_G8(unit + 0xa2);
    const uint16_t id = P4_G16(unit + 0xa4);
    const unsigned hp_effect = P4_G8(row + 0x11), sp_effect = P4_G8(row + 0x14);
    const int32_t power = (int16_t)P4_G16(row + 0x12);
    if (side == 1) {
        if (is_damage(hp_effect)) {
            int64_t score = 200 + (power > 0 ? power : 0);
            const unsigned slot = knowledge_slot(P4_G32(unit + 0xa8), id, 0);
            if (element >= 0 && element < 32) {
                const uint32_t bit = UINT32_C(1) << element;
                if (slot < 64 && (policy.knowledge[slot].weak & bit)) score += 3000;
                /* A target seen absorbing or nulling the element is worth nothing:
                 * it cancels its own share of a spread instead of vetoing it. */
                else if (policy.mode == 5 && slot < 64 && (policy.knowledge[slot].bad & bit)) return -score;
                else if (slot == 64 || !(policy.knowledge[slot].tried & bit)) score += policy.mode == 1 || policy.mode == 5 ? 160 : 60;
            }
            if (policy.mode == 5) {
                const uint32_t uid = P4_G32(unit + 0xa8);
                if (uid == policy.focus_uid) score += 400;
                for (unsigned i = 0; i < policy.healers; i++)
                    if (policy.healer_uid[i] == uid) score += 700;
            }
            return score;
        }
        /* Current buff levels/icons are public. This reads no base defense. */
        const uint32_t mask = P4_G32(row + 0x20);
        if (mask && !datCalcHasBuffMask(P4_G2H(record_of(target)), (int32_t)mask)) return policy.mode == 1 ? 80 : 240;
        return 0;
    }
    if (side != 0 || id < 1 || id > 10) return 0;
    if (is_damage(hp_effect)) return -20000; /* Do not voluntarily damage an ally. */
    const uint32_t record = record_of(target);
    const unsigned hp = P4_G16(record + 8), sp = P4_G16(record + 10);
    const unsigned hp_percent = percent(hp, datCalcMaxHpById((int16_t)id));
    const unsigned sp_percent = percent(sp, datCalcMaxSpById((int16_t)id));
    const uint32_t status = P4_G32(record + 12), mask = P4_G32(row + 0x1c);
    const unsigned removes_status = P4_G8(row + 0x18) == 2;
    if (!hp && removes_status && (mask & 0x80000u)) return policy.mode == 1 ? 9000 : 30000;
    const unsigned heal_threshold = policy.mode == 1 ? 20 : policy.mode == 2 ? (id == 1 ? 60 : 45) :
                                    policy.mode == 5 ? (id == 1 ? 70 : 50) : (id == 1 ? 45 : 35);
    if (hp && is_healing(hp_effect) && hp_percent <= heal_threshold) {
        /* A small fixed heal (a quarter of the ally's HP or less) is not worth
         * a turn unless the ally is critical. */
        if (policy.mode == 5 && hp_effect == 5 && hp_percent > 25 &&
            (uint32_t)(power > 0 ? power : 0) * 4u <= datCalcMaxHpById((int16_t)id)) return 300;
        return (policy.mode == 1 ? 7000 : 24000) + (100 - hp_percent) * 20 + (id == 1 ? 500 : 0);
    }
    if (removes_status && (status & mask)) return policy.mode == 1 ? 100 : 14000;
    if (hp && is_healing(sp_effect) && sp_percent < (policy.mode == 2 || policy.mode == 5 ? 35u : 20u)) return 9000 + (100 - sp_percent) * 10;
    const uint32_t buff = P4_G32(row + 0x20);
    if (hp && buff && !datCalcHasBuffMask(P4_G2H(record), (int32_t)buff)) return policy.mode == 1 ? 60 : 260;
    return 0;
}
static void consider(uint32_t actor, uint16_t skill, uint16_t item, uint16_t wheel, uint16_t menu_index)
{
    if (skill >= policy.skill_rows) { policy.error = "native_policy_skill_outside_loaded_catalog"; return; }
    const uint32_t row = policy.skill_base + skill * 0x28u, record = record_of(actor);
    const uint16_t classification = P4_G8(row + 8);
    const int group = classification == 1 || classification == 2;
    if (group)
        btlEffectCollectTargetUnits(P4_G2H(actor), P4_G2H(policy.scratch), P4_G8(row + 9), P4_G8(row + 10), 0);
    else
        btlSkillClassifyTargets(P4_G2H(actor), P4_G2H(policy.scratch), skill, 0);
    const unsigned count = P4_G16(policy.scratch + 0x38);
    if (count > 12) { policy.error = "native_policy_target_count_outside_engine_capacity"; return; }
    if (!count) return;
    const int32_t element = datCalcSkillElement(P4_G2H(record), skill);
    const uint32_t cost = wheel == WHEEL_SKILL ? datCalcSkillCost(P4_G2H(record), skill) : 0;
    const unsigned cost_kind = P4_G8(row + 3);
    const uint32_t unit = P4_G32(actor + 0x30);
    const uint16_t actor_id = P4_G16(unit + 0xa4);
    const unsigned sp_percent = percent(P4_G16(record + 10), datCalcMaxSpById((int16_t)actor_id));
    int64_t penalty = cost_kind == 1 ? (int64_t)cost * (policy.mode == 1 ? 1 : 4) : (int64_t)cost * (policy.mode == 1 ? 2 : 5);
    if ((policy.mode == 3 || policy.mode == 5) && cost_kind == 2 && sp_percent < 30) penalty += (int64_t)cost * 60;
    const unsigned choices = group ? 1 : count;
    for (unsigned i = 0; i < choices; i++) {
        const uint32_t target = P4_G32(policy.scratch + i * 4);
        int64_t score = -penalty;
        if (group) {
            for (unsigned j = 0; j < count; j++) score += target_utility(P4_G32(policy.scratch + j * 4), row, element);
        } else score += target_utility(target, row, element);
        const uint64_t uid = action_uid(target);
        if (score > policy.choice.score || (score == policy.choice.score &&
            (skill < policy.choice.skill || (skill == policy.choice.skill &&
             (item < policy.choice.item || (item == policy.choice.item && uid < policy.choice.target_uid)))))) {
            policy.choice = (Choice){score, target, uid, skill, item, wheel, menu_index};
        }
    }
}
/* Any living enemy showing the public charge state (Power/Mind Charge). */
static int enemy_charged(void)
{
    const uint32_t battle = GP(0xb3ac);
    if (!battle) return 0;
    uint32_t action = P4_G32(battle + 0x174);
    for (unsigned n = 0; action && n < 12; n++, action = P4_G32(action + 0x450)) {
        const uint32_t unit = P4_G32(action + 0x30);
        if (!unit || P4_G8(unit + 0xa2) != 1) continue;
        const uint32_t record = P4_G32(unit + 0xa64);
        if (record && !(P4_G32(record + 12) & 0x80000u) && (P4_G32(record + 0x14) & 0xC0000u)) return 1;
    }
    return 0;
}
static void choose(uint32_t panel, uint32_t actor)
{
    policy.choice = (Choice){.score = INT64_MIN};
    const uint32_t record = record_of(actor);
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 1))
        consider(actor, (uint16_t)datCalcDefaultSkillId(P4_G2H(record)), 0, WHEEL_ATTACK, 0);
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 2)) {
        const unsigned count = P4_G16(panel + 0x1a4);
        if (count > 8) { policy.error = "native_policy_skill_menu_capacity"; return; }
        for (unsigned i = 0; i < count; i++) {
            const uint16_t skill = P4_G16(panel + 0x194 + i * 2);
            if (!datCalcThresholdTrigger(P4_G2H(record), skill)) consider(actor, skill, 0, WHEEL_SKILL, (uint16_t)i);
        }
    }
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 3)) {
        const unsigned count = P4_G16(panel + 0x5a6);
        if (count > 256) { policy.error = "native_policy_expanded_item_menu_unqualified"; return; }
        for (unsigned i = 0; i < count; i++) {
            const uint16_t item = P4_G16(panel + 0x1a6 + i * 4);
            const uint16_t skill = datCalcUnderlyingSkillId((int64_t)item);
            consider(actor, skill, item, WHEEL_ITEM, (uint16_t)i);
        }
    }
    if (btlTargetCheckCommandAvailability(P4_G2H(actor), 7)) {
        const uint32_t unit = P4_G32(actor + 0x30);
        const uint16_t id = P4_G16(unit + 0xa4);
        const unsigned hp = percent(P4_G16(record + 8), datCalcMaxHpById((int16_t)id));
        int64_t score = policy.mode != 1 && hp < 20 ? 450 : 10;
        /* A visible charge is the boss's warning: below reviving and healing a
         * low ally, above any attack. */
        if (policy.mode == 5 && enemy_charged()) score = 20000;
        if (score > policy.choice.score) policy.choice = (Choice){.score = score, .wheel = WHEEL_GUARD};
    }
    if (policy.choice.score == INT64_MIN) policy.error = "native_policy_has_no_legal_command";
}
static void select_row(uint32_t selection, unsigned index)
{
    const unsigned count = P4_G16(selection + 8), page = P4_G16(selection + 6);
    if (index >= count) { policy.error = "native_policy_menu_choice_invalidated"; return; }
    unsigned first = index >= page && page ? index - page + 1 : 0;
    const unsigned maximum_first = count > page ? count - page : 0;
    if (first > maximum_first) first = maximum_first;
    P4_G16(selection + 2) = (uint16_t)first;
    P4_G16(selection + 4) = (uint16_t)index;
}
int p4_battle_policy_input(uint32_t selection)
{
    const uint32_t battle = GP(0xb3ac);
    if (!battle) return -1;
    const uint32_t task = P4_G32(battle + 0xdd4);
    if (!task) return -1;
    const uint32_t panel = P4_G32(task + 0x38) + 0x9c;
    const uint32_t actor = P4_G32(panel + 0x178);
    if (!actor) return -1;
    if (selection == panel + 0x40) {
        p4_battle_sim_command_ready(actor);
        if (!policy.mode) return -1;
        if (policy.error) return 0;
        const uint64_t uid = action_uid(actor);
        if (!policy.pending || policy.actor_uid != uid) {
            policy.actor = actor;
            policy.actor_uid = uid;
            policy.decision_update = p4_battle_sim_update_count();
            policy.pending = 1;
            choose(panel, actor);
        }
        if (policy.error || p4_battle_sim_update_count() - policy.decision_update < policy.latency) return 0;
        select_row(selection, policy.choice.wheel);
        return policy.error ? 0 : 2;
    }
    if (!policy.mode) return -1;
    if (selection != panel + 0x74 && selection != panel + 0xa8 && selection != panel + 0x144) return -1;
    if (!policy.pending || policy.actor != actor) { policy.error = "native_policy_unowned_submenu"; return 0; }
    if ((selection == panel + 0x74 && policy.choice.wheel == WHEEL_SKILL) ||
        (selection == panel + 0xa8 && policy.choice.wheel == WHEEL_ITEM)) {
        select_row(selection, policy.choice.menu_index);
        return policy.error ? 0 : 2;
    }
    policy.error = "native_policy_unexpected_submenu";
    return 0;
}
void p4_battle_policy_target(uint32_t actor)
{
    if (!policy.mode || !policy.pending || policy.error || actor != policy.actor || !policy.choice.target) return;
    const uint32_t selection = actor + 0x98;
    const unsigned count = P4_G16(selection + 0x38);
    if (count > 12) { policy.error = "native_policy_target_capacity"; return; }
    for (unsigned i = 0; i < count; i++) {
        if (P4_G32(selection + i * 4) != policy.choice.target) continue;
        btlFormationEntrySetSelected((int32_t)selection, P4_G2H(policy.choice.target));
        return;
    }
    policy.error = "native_policy_target_invalidated";
}
void p4_battle_policy_before_update(void)
{
    /* Native menu handlers consume policy input codes; normal packets still
     * own tutorials, Help, target confirmation, All-Out and result prompts. */
    if (policy.mode && p4_battle_sim_update_count() % 24 == 0) p4pc_pad_inject(0x4000, 1);
}
