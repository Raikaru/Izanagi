/* 0021e110: btlXpPrepareAwards. Each recipient's EXP passes the reward stage
 * after the native award (level-difference scale, Growth skills, Lead boost,
 * EXP accessories) and before the level-up preview, so the results screen
 * and the applied EXP agree; the stage never runs while no modifier is
 * registered. An inactive Persona's Growth also counts when granted to it,
 * the protagonist or his equipment (game_passive_grant.h) and not already in
 * a slot; nothing is asked while nothing is granted. */
#include "p4pc/game_reward_stage_native.h"
#include "p4pc/game_passive_grant_native.h"
// Prepare the XP award blocks for the leader and the party without applying them.
/* measured: retail hoists loop-invariant expressions out of the roster loops in this function. */
#pragma push
#pragma opt_loop_invariants on
void btlXpPrepareAwards(u8 *awards, u8 *reward)
{
    s32 rosterIndex;
    s32 partyIndex;
    s32 partyOutput;
    s32 award;
    s32 baseAward;
    s32 rosterBaseAward;
    s32 partyBaseAward;
    s32 extraBaseAward;
    s32 partyAward;
    s32 expOffset;
    s32 idOffset;
    u16 *partyPersona;

    s32 skillAward;
    s32 rosterCount;
    s32 partyCount;
    s32 personaLevel;
    s32 memberId;
    s32 skills;
    s32 skill;
    s32 experience;
    s32 nextLevelExp;
    s32 partyLevel;
    s32 skillIndex;
    u8 heroLevel;
    u16 *heroPersona;
    u16 *rosterPersona;
    s32 heroPersonaId;

    memset(awards, 0, 0x8D4);
    partyOutput = 0;
    if (((BtlXpReward *)reward)->flags & 1) {
        ((BtlXpAwards *)awards)->flags |= 1;
    }
    heroLevel = datCalcGetLevelById(1) & 0xFF;
    award = btlXpComputeAward(heroLevel, reward);
    baseAward = award;
    if (datItemValue(datGetEquippedItemByUnit(1, 0)) == 0x86) {
        award += (s32)(0.5f * (f32)award);
    }
    if (datItemValue(datGetEquippedItemByUnit(1, 1)) == 0x86) {
        award += (s32)(0.5f * (f32)baseAward);
    }
    if (p4_game_reward_native_active()) {
        award = p4_game_reward_native_hero(heroLevel, award);
    }
    if (heroLevel != (datLevelFromExp(award + datPersonaGetExpById(1)) & 0xFF)) {
        ((BtlXpAwards *)awards)->flags |= 2;
    }
    ((BtlXpAwards *)awards)->unk698 = ((BtlXpReward *)reward)->unk10;
    ((BtlXpAwards *)awards)->heroExp = award;
    heroPersona = (u16 *)datPersonaGetRecord(1);
    datUnitGetRecord(1);
    heroPersonaId = heroPersona[1];
    rosterCount = datPersonaCountValidMembers() & 0xFFFF;
    for (rosterIndex = 0; rosterIndex < rosterCount; rosterIndex++) {
        rosterPersona = (u16 *)datPersonaRosterSlot((s16)rosterIndex);
        personaLevel = datPersonaGetLevel((s32)rosterPersona) & 0xFF;
        rosterBaseAward = btlXpComputeAward(personaLevel, reward);
        if (rosterPersona[1] == heroPersonaId) {
            award = rosterBaseAward;
            if (datPersonaLeadBoosted(1) != 0) {
                award += (s32)(0.5f * (f32)rosterBaseAward);
            }
            if (datItemValue(datGetEquippedItemByUnit(1, 0)) == 0x86) {
                award += (s32)(0.5f * (f32)rosterBaseAward);
            }
            if (datItemValue(datGetEquippedItemByUnit(1, 1)) == 0x86) {
                award += (s32)(0.5f * (f32)rosterBaseAward);
            }
        } else {
            skills = (s32)datPersonaGetSkills((struct DatPersonaRecord *)rosterPersona);
            award = 0;
            for (skillIndex = 0; skillIndex < 8; skillIndex++) {
                skill = ((u16 *)skills)[skillIndex];
                if (skill != 0) {
                    switch (skill) {
                    case 0x1FF:
                        award += (s32)(0.25f * (f32)rosterBaseAward);
                        break;
                    case 0x200:
                        award += (s32)(0.5f * (f32)rosterBaseAward);
                        break;
                    case 0x201:
                        award += (s32)(f32)rosterBaseAward;
                        break;
                    }
                }
            }
            for (skill = 0x1FF; p4_game_passive_live != 0u && skill <= 0x201; skill++) {
                for (skillIndex = 0; skillIndex < 8 && ((u16 *)skills)[skillIndex] != skill; skillIndex++) {
                }
                if (skillIndex < 8 || !P4_PASSIVE_ROSTER_GRANTED((u16 *)skills, skill)) {
                    continue;
                }
                if (skill == 0x1FF) {
                    award += (s32)(0.25f * (f32)rosterBaseAward);
                } else if (skill == 0x200) {
                    award += (s32)(0.5f * (f32)rosterBaseAward);
                } else {
                    award += (s32)(f32)rosterBaseAward;
                }
            }
            skillAward = award;
            if (rosterPersona[0] & 4) {
                award += (s32)(0.5f * (f32)award);
            }
            if (datItemValue(datGetEquippedItemByUnit(1, 0)) == 0x86) {
                award += (s32)(0.5f * (f32)skillAward);
            }
            if (datItemValue(datGetEquippedItemByUnit(1, 1)) == 0x86) {
                award += (s32)(0.5f * (f32)skillAward);
            }
        }
        if (p4_game_reward_native_active()) {
            award = p4_game_reward_native_persona(rosterPersona[1], (u32)personaLevel,
                rosterPersona[1] == heroPersonaId, award);
        }
        if (personaLevel < 99) {
            experience = datPersonaGetExp((s32)rosterPersona);
            nextLevelExp = (s32)datPersonaGetExpForLevel(rosterPersona, (u16)(personaLevel + 1));
            if (nextLevelExp <= experience + award) {
                ((BtlXpAwards *)awards)->flags |= 8;
                datPersonaPreviewLevelUp((u8 *)rosterPersona, awards + rosterIndex * 0x88 + 0x38, award);
            }
            if (award != 0) {
                ((BtlXpAwards *)awards)->rosterExp[rosterIndex] = award;
            }
        }
    }
    partyCount = ((BtlXpReward *)reward)->partyCount;
    for (partyIndex = 0; partyIndex < partyCount; partyIndex++) {
        memberId = ((BtlXpReward *)reward)->partyIds[partyIndex];
        if (memberId != 1) {
            partyPersona = (u16 *)datPersonaGetRecord((u16)memberId);
            partyLevel = datPersonaGetLevel((s32)partyPersona) & 0xFF;
            partyBaseAward = btlXpComputeAward(partyLevel, reward);
            partyAward = partyBaseAward;
            if (datItemValue(datGetEquippedItemByUnit((s16)memberId, 0)) == 0x86) {
                partyAward += (s32)(0.5f * (f32)partyBaseAward);
            }
            if (datItemValue(datGetEquippedItemByUnit((s16)memberId, 1)) == 0x86) {
                partyAward += (s32)(0.5f * (f32)partyBaseAward);
            }
            if (p4_game_reward_native_active()) {
                partyAward = p4_game_reward_native_party((u32)memberId, (u32)partyLevel, partyAward);
            }
            if (partyLevel < 99) {
                experience = datPersonaGetExp((s32)partyPersona);
                nextLevelExp = (s32)datPersonaGetExpForLevel(partyPersona, (u16)(partyLevel + 1));
                if (nextLevelExp <= experience + partyAward) {
                    ((BtlXpAwards *)awards)->flags |= 0x10;
                    datPersonaPreviewLevelUp((u8 *)partyPersona, awards + partyOutput * 0x88 + 0x6B4, partyAward);
                }
                ((BtlXpAwards *)awards)->partyExp[partyOutput] = partyAward;
            }
            ((BtlXpAwards *)awards)->partyIds[partyOutput] = memberId;
            partyOutput += 1;
        }
    }
    if (datGetFlag(0x38) != 0) {
        partyPersona = (u16 *)datPersonaGetRecord(5);
        partyLevel = datPersonaGetLevel((s32)partyPersona) & 0xFF;
        extraBaseAward = btlXpComputeAward(partyLevel, reward);
        partyAward = extraBaseAward;
        if (datItemValue(datGetEquippedItemByUnit(5, 0)) == 0x86) {
            partyAward += (s32)(0.5f * (f32)extraBaseAward);
        }
        if (datItemValue(datGetEquippedItemByUnit(5, 1)) == 0x86) {
            partyAward += (s32)(0.5f * (f32)extraBaseAward);
        }
        if (p4_game_reward_native_active()) {
            partyAward = p4_game_reward_native_party(5u, (u32)partyLevel, partyAward);
        }
        if (partyLevel < 99) {
            experience = datPersonaGetExp((s32)partyPersona);
            nextLevelExp = (s32)datPersonaGetExpForLevel(partyPersona, (u16)(partyLevel + 1));
            if (nextLevelExp <= experience + partyAward) {
                ((BtlXpAwards *)awards)->flags |= 0x10;
                datPersonaPreviewLevelUp((u8 *)partyPersona, awards + partyOutput * 0x88 + 0x6B4, partyAward);
            }
            expOffset = partyOutput * 4;
            MEMBER(u32, expOffset + (s32)awards, BtlXpAwards, partyExp[0]) = partyAward;
        }
        idOffset = partyOutput * 2;
        MEMBER(u16, idOffset + (s32)awards, BtlXpAwards, partyIds[0]) = 5;
    }
}
#pragma pop
