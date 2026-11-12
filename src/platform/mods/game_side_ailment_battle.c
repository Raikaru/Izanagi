/* Battle readers of the side-ailment service (hardmode-design §4.F): riders,
 * the single slot, cures, defeat, the lost turn, evasion and the physical
 * critical bonus. Every entry returns before reading a unit unless a battle
 * runs with published definitions, so without the declaring package retail
 * flow, RNG draws and results are unchanged. Main-thread battle code. */
#include "p4pc/game_side_ailment.h"
#include "p4pc/game_side_ailment_native.h"
#include "p4pc/game_elements_native.h"
#include "p4pc/gmem.h"
#include <stddef.h>
#include <stdint.h>

extern int32_t datCalcIsGuarding(uint16_t *unit);
extern uint8_t datCalcGetBattleStat(void *unit, int32_t stat);
extern int8_t datCalcGetBuffLevel(void *unit, int32_t buff);
extern int32_t datCalcHasSkill(void *unit, int32_t skill);
extern int32_t datCalcSkillElement(void *unit, uint16_t skill);
extern uint32_t RpRandom(void);

#define UNIT_FLAGS(unit) (*(const uint16_t *)(unit))
#define UNIT_BAD(unit) (*(const uint32_t *)((const uint8_t *)(unit) + 0xC))
#define UNIT_ENEMY 4u
#define BAD_AILMENTS 0xFFFFFu        /* the 002397D0 test, Dizzy and dead included */
#define BAD_DEAD 0x80000u
#define AFF_NO_EFFECT 0x27000000u    /* 00238940: null, repel, drain or ailment-immune */
#define STAT_LUCK 4                  /* datCalcFormulaRatioOfProducts terms 0xD/0xE, 4 */
#define BUFF_EXPOSED 7               /* 00238940: x2 against a positive level */
#define SKILL_AILMENT_BOOST 0x21E    /* 00238940: x1.25 for every ailment */
#define ENEMY_SOURCE_SCALE P4_GPDATA(float, 0x811c) /* fGpffff811c, 0.8 on the disc */

static uint32_t guest(void *unit) { return P4_H2G(unit); }

static const P4SideAilmentDef *held(void *unit)
{
    const P4SideAilmentDef *def = NULL;
    return p4_side_ailment_query(guest(unit), &def) ? def : NULL;
}

int p4_side_ailment_native_occupied(void *unit)
{
    if (!unit || !p4_side_ailment_active()) return 0;
    return held(unit) != NULL || p4_side_ailment_landed(guest(unit), NULL);
}

int p4_side_ailment_native_lose_turn(void *unit)
{
    if (!unit || !p4_side_ailment_active()) return 0;
    const P4SideAilmentDef *def = held(unit);
    return def && (def->flags & P4_SIDE_LOSE_TURN);
}

int32_t p4_side_ailment_native_crit_bonus(void *target, uint32_t affinity)
{
    if (!target || (affinity & P4_AFF_RESIST) || !p4_side_ailment_active()) return 0;
    const P4SideAilmentDef *def = held(target);
    return def ? (int32_t)def->crit_bonus : 0;
}

/* SMT V's prior-affliction decay of a susceptibility percent R after n
 * landings: R - n * (2R + 100) / 10, never below zero. */
static uint32_t decayed(uint32_t percent, uint32_t landings)
{
    const uint32_t step = (2u * percent + 100u) / 10u;
    return landings >= (percent + step - 1u) / step ? 0u : percent - landings * step;
}

/* 00238940 with the rider's rate in place of the skill row's +0x19 byte:
 * datCalcFormulaRatioOfProducts (rate x status affinity x (100 + source Luck)
 * / (100 + target Luck), x the game's enemy-source scale fGpffff811c) truncated
 * by datScriptRunCalc, then x status affinity again, x2 against an exposed
 * target, x1.25 with Ailment Boost, truncated and clamped to 0..99. enemy_scale,
 * when an enemy hits a party member, multiplies the formula with the game's
 * own enemy-source scale. A base of 100 or more with no scale never reaches
 * here: the caller lands it without a draw. */
static int32_t rider_rate(uint32_t base, void *source, void *target, uint32_t percent, int32_t options,
    uint32_t enemy_scale)
{
    const float affinity = (float)percent / 100.0f;
    float formula = (float)base * affinity;
    formula *= 100.0f + (float)datCalcGetBattleStat(source, STAT_LUCK);
    formula /= 100.0f + (float)datCalcGetBattleStat(target, STAT_LUCK);
    if (UNIT_FLAGS(source) & UNIT_ENEMY) formula *= ENEMY_SOURCE_SCALE;
    if (enemy_scale) formula *= (float)enemy_scale / 100.0f;
    float multiplier = affinity;
    if (!(options & 1)) {
        if (datCalcGetBuffLevel(target, BUFF_EXPOSED) > 0) multiplier *= 2.0f;
        if (datCalcHasSkill(source, SKILL_AILMENT_BOOST)) multiplier *= 1.25f;
    }
    const int32_t rate = (int32_t)((float)(int32_t)formula * multiplier);
    return rate > 99 ? 99 : rate < 0 ? 0 : rate;
}

void p4_side_ailment_native_riders(uint32_t skill_id, void *source, void *target,
    uint32_t outcome, uint32_t retail_statuses, int32_t options)
{
    if (!source || !target || (outcome & 0xFFFFu) != 1u || !p4_side_ailment_active()) return;
    size_t first, count;
    if (!p4_side_ailment_riders(skill_id, &first, &count)) return;
    for (size_t i = first; i < first + count; ++i) {
        const P4SideRiderDesc *rider = p4_side_ailment_rider_at(i);
        p4_side_ailment_use_rider(rider);
        const P4SideAilmentDef *def = p4_side_ailment_find_leaf(rider->leaf);
        if (!def) continue;
        /* The occupied slot and Guard refuse every rider before any roll, as
         * 002397D0 and 00238940 do for a retail ailment; a single-slot leaf
         * also yields to a held retail ailment or one this hit inflicts. */
        if (p4_side_ailment_native_occupied(target) || p4_side_ailment_protected(guest(target)) ||
            datCalcIsGuarding((uint16_t *)target)) return;
        if ((def->flags & P4_SIDE_SINGLE_SLOT) && ((UNIT_BAD(target) | retail_statuses) & BAD_AILMENTS)) continue;
        const uint32_t word = p4_game_elements_native_affinity_leaf(target, rider->leaf,
            P4_CHANNEL_STATUS, P4_ACTION_COMPONENT_STATUS);
        if (word & AFF_NO_EFFECT) continue;
        uint32_t percent = word & 0xFFFFu;
        if (def->flags & P4_SIDE_SMTV_DECAY)
            percent = decayed(percent, p4_side_ailment_landings(guest(target), rider->leaf));
        if (!percent) continue;
        /* An enemy hitting a party member rolls the scaled chance; a base of 100
         * with no scale (0 or 100) lands without a roll, as the +0x19 >= 100 path does. */
        const uint32_t scale = (UNIT_FLAGS(source) & UNIT_ENEMY) && !(UNIT_FLAGS(target) & UNIT_ENEMY) &&
            def->enemy_scale < 100u ? def->enemy_scale : 0u;
        const int landed = (rider->rate >= 100u && !scale) ||
            (int32_t)(RpRandom() % 100u) < rider_rate(rider->rate, source, target, percent, options, scale);
        if (landed && p4_side_ailment_mark_landed(guest(target), rider->leaf) == 0) {
            /* Analyze learns the landing's pair as it learns an ailment skill's. */
            if ((UNIT_FLAGS(target) & UNIT_ENEMY) && p4_game_elements_native_enabled())
                p4_game_elements_native_learn_leaf(target, rider->leaf);
            return;
        }
    }
}

/* The hit's delivery family: physical for the Phys leaf and Physical Split's
 * leaves (their family 0), from the action's own frame when routing is on. */
static int physical_hit(void *source, uint32_t skill_id)
{
    return p4_game_elements_native_enabled() ?
        p4_game_elements_native_presentation_leaf(source, skill_id) == 0 :
        datCalcSkillElement(source, (uint16_t)skill_id) == 0;
}

/* P5's Shock: a damaging physical hit on a holder moves the ailment to the
 * attacker with chance transfer x the attacker's own susceptibility (decayed
 * like a rider's), when the attacker could take it; no draw otherwise. */
static void transfer(void *unit, void *source, uint32_t skill_id, const P4SideAilmentDef *def)
{
    const uint32_t attacker = guest(source);
    if (!def->transfer || source == unit || held(source) || p4_side_ailment_protected(attacker) ||
        (UNIT_BAD(source) & BAD_DEAD) || ((def->flags & P4_SIDE_SINGLE_SLOT) && (UNIT_BAD(source) & BAD_AILMENTS)) ||
        !physical_hit(source, skill_id)) return;
    const uint32_t word = p4_game_elements_native_affinity_leaf(source, def->leaf,
        P4_CHANNEL_STATUS, P4_ACTION_COMPONENT_STATUS);
    if (word & AFF_NO_EFFECT) return;
    uint32_t percent = word & 0xFFFFu;
    if (def->flags & P4_SIDE_SMTV_DECAY) percent = decayed(percent, p4_side_ailment_landings(attacker, def->leaf));
    const uint32_t rate = def->transfer * percent / 100u;
    if (!rate || (rate < 100u && RpRandom() % 100u >= rate)) return;
    if (p4_side_ailment_clear(guest(unit), P4_SIDE_REASON_TRANSFER) == def->leaf)
        (void)p4_side_ailment_set(attacker, def->leaf, P4_SIDE_REASON_TRANSFER);
}

void p4_side_ailment_native_commit(void *unit, uint32_t removed_status, int32_t hp_delta,
    void *source, uint32_t skill_id)
{
    uint32_t leaf;
    if (!unit || !p4_side_ailment_active()) return;
    /* An ailment held before this packet reacts to its damage first, so a hit
     * never both breaks or moves an ailment and lands it again. */
    const P4SideAilmentDef *before = held(unit);
    if (before && hp_delta < 0) {
        if (before->flags & P4_SIDE_BREAK_ON_HIT) (void)p4_side_ailment_clear(guest(unit), P4_SIDE_REASON_BROKEN);
        else if (source) transfer(unit, source, skill_id, before);
    }
    if (p4_side_ailment_landed(guest(unit), &leaf)) (void)p4_side_ailment_set(guest(unit), leaf, P4_SIDE_REASON_RIDER);
    const P4SideAilmentDef *def = held(unit);
    if (def && (removed_status & def->cure_mask) == def->cure_mask)
        (void)p4_side_ailment_clear(guest(unit), P4_SIDE_REASON_CURE);
}

void p4_side_ailment_native_defeat(void *unit)
{
    if (!unit || !p4_side_ailment_active()) return;
    (void)p4_side_ailment_clear(guest(unit), P4_SIDE_REASON_DEFEAT);
}

int p4_side_ailment_native_spend_turn(void *unit)
{
    if (!unit || !p4_side_ailment_active()) return 0;
    return p4_side_ailment_turn_lost(guest(unit));
}

void p4_side_ailment_native_turn_end(void *unit)
{
    if (p4_side_ailment_native_lose_turn(unit)) (void)p4_side_ailment_clear(guest(unit), P4_SIDE_REASON_RECOVERED);
}

void p4_side_ailment_native_action_begin(void)
{
    if (p4_side_ailment_active()) p4_side_ailment_action_end();
}

int p4_side_ailment_native_covers(void *unit, uint32_t mask)
{
    if ((mask & 0xFFu) != 0xFFu || !unit || !p4_side_ailment_active()) return 0;
    return held(unit) != NULL;
}
