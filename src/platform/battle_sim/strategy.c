/* The strategic candidate's pure decision rules (include/p4pc/battle_strategy.h). */
#include "p4pc/battle_strategy.h"

int p4_strategy_setup_direction(uint32_t mask, unsigned side)
{
    const uint32_t helps = side == P4_STRATEGY_PARTY ? P4_STRATEGY_BUFF_RAISE : P4_STRATEGY_BUFF_LOWER;
    return side <= P4_STRATEGY_ENEMY && mask && !(mask & ~helps);
}
int p4_strategy_durable(const P4StrategyEnemy *enemies, unsigned count)
{
    unsigned living = 0;
    for (unsigned i = 0; i < count; i++) {
        if (!enemies[i].living) continue;
        if (!enemies[i].tried || enemies[i].weak) return 0;
        living++;
    }
    return living != 0;
}
int p4_strategy_setup_allowed(const P4StrategyTempo *tempo)
{
    return !tempo->setup_pending;
}
void p4_strategy_tempo_commit(P4StrategyTempo *tempo, unsigned kind)
{
    if (kind == P4_STRATEGY_COMMIT_SETUP) tempo->setup_pending = 1;
    else if (kind == P4_STRATEGY_COMMIT_DAMAGE) tempo->setup_pending = 0;
}
int p4_strategy_new_encounter(const uint32_t *previous, unsigned previous_count, const uint32_t *current,
                              unsigned current_count)
{
    if (!previous_count || !current_count) return 0;
    for (unsigned i = 0; i < current_count; i++)
        for (unsigned j = 0; j < previous_count; j++)
            if (current[i] == previous[j]) return 0;
    return 1;
}
int64_t p4_strategy_setup_value(uint32_t mask, unsigned side, int durable, const P4StrategyTempo *tempo,
                                int64_t inherited)
{
    if (!durable || !p4_strategy_setup_direction(mask, side) || !p4_strategy_setup_allowed(tempo)) return inherited;
    return P4_STRATEGY_SETUP > inherited ? P4_STRATEGY_SETUP : inherited;
}
int64_t p4_strategy_heal_value(int64_t inherited, int fixed, uint32_t power, uint32_t hp, uint32_t max_hp)
{
    if (!fixed || hp >= max_hp) return inherited;
    const uint32_t missing = max_hp - hp;
    return missing > power ? inherited - (int64_t)(missing - power) : inherited;
}
int p4_strategy_switch_priceable(unsigned cost_kind, int percent, int cost_stage, int active_master, int candidate_master)
{
    if (cost_kind != 1 && cost_kind != 2) return 1; /* datCalcSkillCost prices only kinds 1 (HP) and 2 (SP). */
    return cost_kind == 2 && !percent && !cost_stage && !active_master == !candidate_master;
}
int p4_strategy_switch(int64_t current, int64_t candidate, int switched)
{
    if (switched || candidate == INT64_MIN) return 0;
    /* current is a real score or INT64_MIN (no legal command yet); neither overflows. */
    return current < INT64_MAX - P4_STRATEGY_SWITCH_MARGIN && candidate > current + P4_STRATEGY_SWITCH_MARGIN;
}
