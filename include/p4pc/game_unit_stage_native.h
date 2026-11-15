#ifndef P4PC_GAME_UNIT_STAGE_NATIVE_H
#define P4PC_GAME_UNIT_STAGE_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the unit stage for datCalcMaxHp 0x00231F80,
 * datCalcMaxSp 0x00232290, datCalcGetBattleStat 0x00232C70 and an enemy's
 * starting HP/SP in datUnitInit 0x00231AF0. Returns the composed value,
 * clamped to the stat's range (game_unit_stage.h). */
#define P4_UNIT_MAX_HP 0u
#define P4_UNIT_MAX_SP 1u
#define P4_UNIT_STRENGTH 2u /* battle stats: datCalcGetBattleStat index + 2 */
#define P4_UNIT_MAGIC 3u
#define P4_UNIT_ENDURANCE 4u
#define P4_UNIT_AGILITY 5u
#define P4_UNIT_LUCK 6u
int p4_game_unit_native_active(void);
int32_t p4_game_unit_native_amount(uint32_t enemy, uint32_t id, uint32_t stat, uint32_t level, int32_t stock);
#endif
