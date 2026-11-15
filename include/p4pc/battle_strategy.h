/* The strategic candidate's decision rules (battle_sim party policy 8,
 * strategic-player-candidate-v1). Pure functions over facts policy.c has
 * already gathered from state a player sees or remembers: buffs the party
 * cast and their lapse, outcomes the party saw land, item/skill descriptions,
 * and the protagonist's own stocked Personas. No enemy HP, affinity table, RNG
 * or next action reaches them. They are hypotheses motivated by expert play,
 * not a validated expert model. */
#ifndef P4PC_BATTLE_STRATEGY_H
#define P4PC_BATTLE_STRATEGY_H
#include <stdint.h>

/* One direction-pure buff or debuff on a target in a durable fight. An attack
 * scores 200 + power, +400 on the focused enemy, +160 for an untried element,
 * so this beats one of power below 600 there (440 with an untried element), and
 * stays below every urgent recovery score (SP 9000+, cure 14000+, heal, revive). */
#define P4_STRATEGY_SETUP 1200
/* A stocked Persona must beat the current best decision by more than this. */
#define P4_STRATEGY_SWITCH_MARGIN 300
/* datCalcHasBuffMask stat bits: 2i raises stat i, 2i+1 lowers it (i 0..4). */
#define P4_STRATEGY_BUFF_RAISE 0x155u
#define P4_STRATEGY_BUFF_LOWER 0x2AAu
/* datCalcSkillCost (0023D9B0) halves an SP cost for Spell Master (0x20A) and an
 * HP cost for Arms Master (0x20B) held in the active Persona's skill list. */
#define P4_STRATEGY_SPELL_MASTER 0x20Au
#define P4_STRATEGY_ARMS_MASTER 0x20Bu

enum { P4_STRATEGY_PARTY = 0, P4_STRATEGY_ENEMY = 1 };
/* Committed party decisions, for the setup tempo. */
enum { P4_STRATEGY_COMMIT_OTHER = 0, P4_STRATEGY_COMMIT_DAMAGE = 1, P4_STRATEGY_COMMIT_SETUP = 2 };

/* What the party has seen of one enemy: elements it hit with (tried) and the
 * native WEAK it saw (weak); living from the public KO state. */
typedef struct P4StrategyEnemy {
    uint32_t tried, weak;
    uint8_t living;
} P4StrategyEnemy;
/* Setup tempo within one encounter: a setup has been committed and no damage
 * decision has followed it yet. */
typedef struct P4StrategyTempo {
    uint8_t setup_pending;
} P4StrategyTempo;

/* 1 when a skill's buff mask moves only in the direction that helps the party
 * on that side: only raised bits on a party member, only lowered bits on an
 * enemy. Mixed or other-direction masks keep their inherited score. */
int p4_strategy_setup_direction(uint32_t mask, unsigned side);
/* 1 when some enemy is alive, every living enemy has already taken a party hit
 * and survived, and none has shown a weakness: no knockdown route is known, so
 * the fight continues past this turn and setup affects later decisions. */
int p4_strategy_durable(const P4StrategyEnemy *enemies, unsigned count);
/* A boosted setup needs a party damage decision since the last setup, so
 * setups never follow each other however many attacks came before. */
int p4_strategy_setup_allowed(const P4StrategyTempo *tempo);
void p4_strategy_tempo_commit(P4StrategyTempo *tempo, unsigned kind);
/* 1 when the enemies on the field share no unit with those of the previous
 * decision (a chained encounter's next wave): per-encounter state restarts. */
int p4_strategy_new_encounter(const uint32_t *previous, unsigned previous_count, const uint32_t *current,
                              unsigned current_count);
/* The score of a buff/debuff on one target that does not hold it yet. */
int64_t p4_strategy_setup_value(uint32_t mask, unsigned side, int durable, const P4StrategyTempo *tempo,
                                int64_t inherited);
/* The score of a heal on an ally below its threshold. A fixed heal (its power
 * is the HP it restores, as the item or skill description says) loses the part
 * of the ally's missing HP it leaves uncovered, so a larger fixed heal wins and
 * a heal whose amount is not fixed keeps its inherited score. */
int64_t p4_strategy_heal_value(int64_t inherited, int fixed, uint32_t power, uint32_t hp, uint32_t max_hp);
/* 1 when a stocked Persona's skill costs exactly what the active Persona's
 * pricing (datCalcSkillCost/datCalcThresholdTrigger) reports, so the reopened
 * Skill list can pay for it: free, or a flat SP cost while no cost stage is
 * active and both Personas agree on Spell Master. HP and percentage costs
 * follow the Persona's maximum HP/SP passives, so they are never priced. */
int p4_strategy_switch_priceable(unsigned cost_kind, int percent, int cost_stage, int active_master, int candidate_master);
/* 1 when the protagonist should change to the stocked Persona whose best
 * decision is `candidate`: not already changed this command, and strictly more
 * than the margin above the current best decision. */
int p4_strategy_switch(int64_t current, int64_t candidate, int switched);

#endif
