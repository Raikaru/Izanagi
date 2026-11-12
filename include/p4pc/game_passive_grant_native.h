#ifndef P4PC_GAME_PASSIVE_GRANT_NATIVE_H
#define P4PC_GAME_PASSIVE_GRANT_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary of the passive grant table (game_passive_grant.h) for
 * the translated passive checks: datCalcHasRequestedSkill and
 * datCalcHasFixedSkill (overrides/static), the hand-inlined slot searches of
 * datCalcSkillCost 0x0023D9B0, datCalcRollSkillChance 0x00242360 and
 * datCalcResolveSkillId 0x00242990, the AI Counter test 0x001DB360, Growth in
 * btlXpPrepareAwards 0x0021E110, the Camp duplicate-skill choice
 * 0x00138490 and the field encounter check 0x00172BA0. Each asks only after the
 * game's own slots and accessory found nothing, so a granted skill a unit
 * already has counts once. p4_game_passive_live is zero while nothing is
 * granted; every check is then one load and the original result. */
extern uint32_t p4_game_passive_live;
/* unit: a live DatUnit (flags +0, id +2). An enemy (flags bit 2) sees the
 * grants of its enemy ID; a party member those of its member ID, its current
 * Persona and the three items it has equipped. */
int p4_game_passive_native_unit(const void *unit, uint32_t skill);
/* skills: the slot array (+0x0C) of one of the protagonist's roster Persona
 * records, as datPersonaGetSkills returns it: that Persona's grants and the
 * protagonist's own (member 1 and his equipment). */
int p4_game_passive_native_roster(const void *skills, uint32_t skill);
/* The protagonist with his current Persona (the field's leader checks). */
int p4_game_passive_native_leader(uint32_t skill);
/* A party member's own and its equipment's grants, not its Persona's: the
 * field check 0x00172BA0 reads the leader Persona's slots but only the
 * accessory-granted skill of every active member (ITEMTBL row 0x270 is the
 * one item whose granted skill, +0x24, is 0x205). */
int p4_game_passive_native_member(uint32_t member, uint32_t skill);

#define P4_PASSIVE_UNIT_GRANTED(unit, skill) \
    (p4_game_passive_live != 0u && p4_game_passive_native_unit((const void *)(unit), (uint32_t)(skill)))
#define P4_PASSIVE_ROSTER_GRANTED(skills, skill) \
    (p4_game_passive_live != 0u && p4_game_passive_native_roster((const void *)(skills), (uint32_t)(skill)))
#define P4_PASSIVE_LEADER_GRANTED(skill) \
    (p4_game_passive_live != 0u && p4_game_passive_native_leader((uint32_t)(skill)))
#define P4_PASSIVE_MEMBER_GRANTED(member, skill) \
    (p4_game_passive_live != 0u && p4_game_passive_native_member((uint32_t)(member), (uint32_t)(skill)))
/* Counter, Counterstrike or High Counter (0x1F6..0x1F8), the passives the AI's
 * physical-target test avoids. */
#define P4_PASSIVE_COUNTER_GRANTED(unit) \
    (p4_game_passive_live != 0u && (p4_game_passive_native_unit((const void *)(unit), 0x1F6u) || \
        p4_game_passive_native_unit((const void *)(unit), 0x1F7u) || \
        p4_game_passive_native_unit((const void *)(unit), 0x1F8u)))
#endif
