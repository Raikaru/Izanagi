/* 0021e050: btlXpBaseAward, the yen the results screen shows. The team's
 * money passes the reward stage exactly as btlXpApplyAwards (0021e9a0) pays
 * it; the stage never runs while no modifier is registered. */
#include "p4pc/game_reward_stage_native.h"
// Return the base XP award for one persona after the award flags.
s32 btlXpBaseAward(u8 *reward)
{
    s32 value;

    value = ((BtlXpReward *)reward)->money;
    if (value <= 0) {
        value = 0;
    } else if ((((BtlXpReward *)reward)->flags & 8) == 0 && (datGetFlag(0x1403) != 0)) {
        if (datGetFlag(0x1420) != 0) {
            value = (s32)(2.0f * (f32)value);
        } else if (datGetFlag(0x1421) != 0) {
            value = 1;
        }
    }
    if (p4_game_reward_native_active()) {
        value = p4_game_reward_native_money(value);
    }
    return value;
}
