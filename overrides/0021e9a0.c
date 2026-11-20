/* 0021e9a0: btlXpApplyAwards. The team's money passes the reward stage
 * exactly as btlXpBaseAward (0021e050) shows it; the stage never runs while
 * no modifier is registered. */
#include "p4pc/game_reward_stage_native.h"
#include "p4pc/guest.h"
#include "p4pc/battle_notify.h"
// Apply the prepared level and skill-point awards to the leader and party.
void btlXpApplyAwards(u8 *awards, u8 *reward)
{
    s32 heroExperience;
    s32 hero;
    s32 previousExperience;
    s32 money;
    s32 itemIndex;
    u8 *itemCursor;
    u8 *item;
    s32 previousCount;
    s16 sum;
    hero = (s32)datUnitGetRecord(1);
    previousExperience = (s32)datPersonaGetExpById(1);
    if ((datLevelFromExp(previousExperience) & 0xFF) < 0x63) {
        heroExperience = previousExperience + ((BtlXpAwards *)awards)->heroExp;
        datSetHeroExp(1, heroExperience);
        datCalcSetLevel((u8 *)hero, datLevelFromExp(heroExperience));
    }
    money = ((BtlXpReward *)reward)->money;
    if (money <= 0) {
        money = 0;
    } else if ((((BtlXpReward *)reward)->flags & 8) == 0 && datGetFlag(0x1403) != 0) {
        if (datGetFlag(0x1420) != 0) {
            money = (s32)(2.0f * (f32)money);
        } else if (datGetFlag(0x1421) != 0) {
            money = 1;
        }
    }
    if (p4_game_reward_native_active()) {
        money = p4_game_reward_native_money(money);
    }
    datAddMoney(money);
    itemIndex = 0;
    while (itemIndex < ((BtlXpReward *)reward)->itemCount) {
        itemCursor = reward + itemIndex * 4;
        item = itemCursor + 0x24;
        previousCount = (s16)(datItemCount(((BtlXpItem *)(itemCursor + 0x24))->id) & 0xFF);
        sum = previousCount + ((BtlXpItem *)(itemCursor + 0x24))->count;
        if (sum > 0x63) {
            sum = 0x63;
        }
        datItemSetCount((s32)*(s16 *)item, (u8)sum);
        itemIndex += 1;
    }
    if (p4_battle_notify_interest)
        p4_battle_notify_reward(P4_H2G(awards), P4_H2G(reward), money, (u32)previousExperience);
}
