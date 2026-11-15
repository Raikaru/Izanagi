/* 00233bc0: a unit's buff multiplier for one stat (datCalcBuffMultiplier).
 * For attack, magic, evasion, defense and accuracy the native multiplier
 * (rank table, Charge or Concentrate, Lead boost) passes the buff stage; the
 * stage never runs while no modifier is registered. The percentage table
 * keeps a three-byte row per side and the signed rank -1..1 indexes it. */
#include "p4pc/game_buff_stage_native.h"
/* The low-byte status index must be below 24. Retail diagnoses that
 * contract twice; all six executable callers pass constants 0..4.
 * Their third argument is an unused u16 skill ID; preserving that contract
 * avoids caller-side widening before the matching hit and damage routines. */
#pragma push
#pragma opt_propagation off
f32 datCalcBuffMultiplier(u8 *unit, s32 buffIndex, u16 unusedSkill)
{
    s32 index;
    u8 active;
    s32 enemyFlag;
    u32 enemy;
    s16 enemyPredicate;
    f32 multiplier;
    f32 percentage;
    s32 value;
    u8 *row;
    s32 status;

    index = buffIndex & 0xFF;
    if (index >= 24) {
        kAssertFail(D_00635938, 1265);
    }
    if (index >= 24) {
        kAssertFail(D_00635938, 1217);
    }
    status = (s8)(index < 16 ? datCalcGetBuffLevel((DatUnit *)unit, buffIndex) :
        (s8)(active = (*(u32 *)(unit + 20) & (1U << index)) != 0));
    if (status < -1 || status > 1) {
        kAssertFail(D_00635938, 1269);
    }
    enemyFlag = (*(u16 *)unit & 4);
    enemyPredicate = enemyFlag != 0;
    enemy = (u16)enemyPredicate;
    multiplier = 1.0f;
    row = (u8 *)iGpffffb408 + (enemy & 0xFFFF) * 3;
    switch (index) {
    case 0:
        value = row[1 + status];
        percentage = (f32)(u32)value;
        multiplier = percentage / 100.0f;
        active = (*(u32 *)(unit + 20) & 0x40000U) != 0;
        if ((s8)active > 0) {
            multiplier = multiplier * 2.5f;
        }
        if (enemyFlag == 0 && datPersonaLeadBoosted(*(u16 *)(unit + 2)) != 0) {
            multiplier = multiplier * iGpffff8110;
        }
        break;
    case 1:
        value = row[1 + status];
        percentage = (f32)(u32)value;
        multiplier = percentage / 100.0f;
        active = (*(u32 *)(unit + 20) & 0x80000U) != 0;
        if ((s8)active > 0) {
            multiplier = multiplier * 2.5f;
        }
        if (enemyFlag == 0 && datPersonaLeadBoosted(*(u16 *)(unit + 2)) != 0) {
            multiplier = multiplier * iGpffff8110;
        }
        break;
    case 2:
        value = row[7 - status];
        percentage = (f32)(u32)value;
        multiplier = percentage / 100.0f;
        if (enemyFlag == 0 && datPersonaLeadBoosted(*(u16 *)(unit + 2)) != 0) {
            multiplier = multiplier * 0.75f;
        }
        break;
    case 3:
        value = row[1 - status];
        percentage = (f32)(u32)value;
        multiplier = percentage / 100.0f;
        if (enemyFlag == 0 && datPersonaLeadBoosted(*(u16 *)(unit + 2)) != 0) {
            multiplier = multiplier * 0.75f;
        }
        break;
    case 4:
        value = row[7 + status];
        percentage = (f32)(u32)value;
        multiplier = percentage / 100.0f;
        if (enemyFlag == 0 && datPersonaLeadBoosted(*(u16 *)(unit + 2)) != 0) {
            multiplier = multiplier * iGpffff8110;
        }
        break;
    case 18:
        if (status > 0) {
            multiplier = 2.5f;
        }
        break;
    case 19:
        if (status > 0) {
            multiplier = 2.5f;
        }
        break;
    }
    /* The five stat multipliers pass the buff stage; Charge is the flag this
     * stat's attack consumes. */
    if (index < 5 && p4_game_buff_native_active()) {
        const u32 charged = index == 0 ? (*(u32 *)(unit + 20) & 0x40000U) != 0 :
            index == 1 ? (*(u32 *)(unit + 20) & 0x80000U) != 0 : 0;
        multiplier = p4_game_buff_native_factor(enemyFlag != 0, *(u16 *)(unit + 2), (u32)index, status, charged,
            multiplier);
    }
    return multiplier;
}

#pragma pop
