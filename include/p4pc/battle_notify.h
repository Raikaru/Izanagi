#ifndef P4PC_BATTLE_NOTIFY_H
#define P4PC_BATTLE_NOTIFY_H
/* Native battle moments the observer (battle_events.h) does not carry, reached
 * through mirror fixes and the btlXpApplyAwards override, and forwarded to one
 * sink, the semantic events (battle_semantic.h):
 *   one_more  btlTargetStatePacketUpdate 0x001F37D0, the 0x701 packet that adds
 *             a One More to its action (BtlAction+0x28);
 *   reward    the end of btlXpApplyAwards 0x0021E9A0, once per battle result,
 *             after the protagonist's EXP, the money and the items are given;
 *   level_up  btlXpAddRosterExp 0x0021EB60 / btlXpAddPartyExp 0x0021EC40,
 *             after a Persona's datPersonaApplyLevelUp.
 * Arguments are guest addresses. With no sink the mirror tests one word
 * (p4_battle_notify_interest) and calls nothing. */
#include <stdint.h>

enum { P4_NOTIFY_ONE_MORE = 1u, P4_NOTIFY_REWARD = 2u, P4_NOTIFY_LEVEL_UP = 4u };
extern uint32_t p4_battle_notify_interest;

typedef struct P4BattleNotifySink {
    void (*one_more)(uint32_t action);
    /* awards: BtlXpAwards; reward: BtlXpReward; money: the yen given after
     * Arcana effects and reward modifiers; hero_exp_before: the protagonist's EXP before the award. */
    void (*reward)(uint32_t awards, uint32_t reward, int32_t money, uint32_t hero_exp_before);
    /* member: 1 for the protagonist's Personas, else the party member; persona: its DatPersonaRecord;
     * delta: the applied level-up preview (levels gained +0, learned skills +2). */
    void (*level_up)(uint32_t member, uint32_t persona, uint32_t delta);
} P4BattleNotifySink;

/* Installs the sink (NULL removes it) and the moments it wants. */
void p4_battle_notify_sink(const P4BattleNotifySink *sink, uint32_t interest);
void p4_battle_notify_one_more(uint32_t action);
void p4_battle_notify_reward(uint32_t awards, uint32_t reward, int32_t money, uint32_t hero_exp_before);
void p4_battle_notify_level_up(uint32_t member, uint32_t persona, uint32_t delta);
#endif
