/* Native battle notifications: include/p4pc/battle_notify.h. Core (p4port),
 * because the mirror calls it in every build; the sink lives with the mods. */
#include "p4pc/battle_notify.h"
#include <stddef.h>

uint32_t p4_battle_notify_interest;
static P4BattleNotifySink current;

void p4_battle_notify_sink(const P4BattleNotifySink *sink, uint32_t interest)
{
    current = sink ? *sink : (P4BattleNotifySink){0};
    p4_battle_notify_interest = sink ? interest : 0;
}
void p4_battle_notify_one_more(uint32_t action)
{
    if ((p4_battle_notify_interest & P4_NOTIFY_ONE_MORE) && current.one_more) current.one_more(action);
}
void p4_battle_notify_reward(uint32_t awards, uint32_t reward, int32_t money, uint32_t hero_exp_before)
{
    if ((p4_battle_notify_interest & P4_NOTIFY_REWARD) && current.reward) current.reward(awards, reward, money, hero_exp_before);
}
void p4_battle_notify_level_up(uint32_t member, uint32_t persona, uint32_t delta)
{
    if ((p4_battle_notify_interest & P4_NOTIFY_LEVEL_UP) && current.level_up) current.level_up(member, persona, delta);
}
