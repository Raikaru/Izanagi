#ifndef P4PC_GAME_REWARD_STAGE_NATIVE_H
#define P4PC_GAME_REWARD_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the battle reward stage for btlXpPrepareAwards
 * 0x0021E110 (EXP per recipient) and btlXpBaseAward 0x0021E050 /
 * btlXpApplyAwards 0x0021E9A0 (money). Each returns the composed award,
 * clamped to 0..P4_REWARD_MAX. */
int p4_game_reward_native_active(void);
int32_t p4_game_reward_native_hero(uint32_t level, int32_t stock);
int32_t p4_game_reward_native_party(uint32_t member, uint32_t level, int32_t stock);
int32_t p4_game_reward_native_persona(uint32_t persona, uint32_t level, uint32_t equipped, int32_t stock);
int32_t p4_game_reward_native_money(int32_t stock);
#endif
