#ifndef P4PC_GAME_REWARD_STAGE_H
#define P4PC_GAME_REWARD_STAGE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_damage_stage.h"
/* Battle reward stage (docs/modding/tables.md "Reward stage"): one ordered
 * composition of registered modifiers per award a won battle pays. EXP:
 * btlXpPrepareAwards 0x0021E110, per recipient, after the native award (the
 * level-difference scale, Growth skills, Lead boost and EXP accessories) and
 * before the level-up preview, so the results screen and the applied EXP
 * agree. Money: the team's yen after the native flags, in both
 * btlXpBaseAward 0x0021E050 (the results screen asks it every frame it draws)
 * and btlXpApplyAwards 0x0021E9A0 (the payment), so a modifier must answer
 * the same query the same way every time. With no modifier registered the
 * functions are retail and the stage
 * never runs. */
#define P4_REWARD_EXP 1u
#define P4_REWARD_MONEY 2u
#define P4_REWARD_HERO 1u    /* the protagonist's own EXP */
#define P4_REWARD_PARTY 2u   /* a party member's (their persona's) EXP */
#define P4_REWARD_PERSONA 3u /* one of the protagonist's Personas */
#define P4_REWARD_MIN 0
#define P4_REWARD_MAX 999999
typedef enum P4GameRewardKind { P4_REWARD_MULTIPLY = 0, P4_REWARD_ADD = 1 } P4GameRewardKind;
/* One award. reward is P4_REWARD_EXP or P4_REWARD_MONEY. An EXP award names
 * its recipient: the hero (id 1), a party member (character id) or one of
 * the protagonist's Personas (persona id; equipped is 1 for the one he
 * carries, 0 for a stock Persona earning through Growth); level is the
 * recipient's. Money has recipient, id and level 0. stock is the native
 * award; 0 pays nothing. */
typedef struct P4GameRewardQuery {
    uint32_t reward;
    uint32_t recipient, id, level;
    uint32_t equipped;
    int32_t stock;
} P4GameRewardQuery;
typedef int (*P4GameRewardModifier)(void *user, const P4GameRewardQuery *query, double *value);
typedef struct P4GameRewardModifierDesc {
    const char *owner;
    int32_t priority;
    uint32_t load_order;
    P4GameRewardKind kind;
    P4GameRewardModifier modifier;
    void *user;
} P4GameRewardModifierDesc;
/* Same status codes as the damage stage (P4_DAMAGE_OK ... NOT_FOUND). */
int p4_game_reward_register(const P4GameRewardModifierDesc *desc, uint32_t *handle);
int p4_game_reward_unregister(uint32_t handle);
size_t p4_game_reward_count(void);
/* Composes in double, truncates once and clamps to P4_REWARD_MIN..MAX.
 * P4PC_REWARD_TRACE=1 traces each award. */
int32_t p4_game_reward_apply(const P4GameRewardQuery *query);
#endif
