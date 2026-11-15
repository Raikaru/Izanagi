/* The passive grant table (src/platform/mods/game_passive_grant.c) and the
 * game's own passive checks over it, with scripted engine facts: the static
 * overrides datCalcHasRequestedSkill, datCalcHasFixedSkill and
 * cmpSkillListContains and Growth in btlXpPrepareAwards (overrides/0021e110.c).
 * A synthetic witness, not a gameplay qualification. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
#define TYPE_H
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef float f32;
#include "p4pc/game_passive_grant.h"

typedef struct DatUnit {
    u16 flags, id, id2; u8 level, pad07; u16 hp, sp; u32 bad; u8 rest[0x20];
} DatUnit;
typedef struct DatPersonaRecord {
    u16 flags, id; u8 level, unknown05, unknown06[2]; s32 exp; u16 skills[8]; u8 rest[0x14];
} DatPersonaRecord;
_Static_assert(offsetof(DatUnit, id) == 2 && offsetof(DatPersonaRecord, id) == 2 &&
    offsetof(DatPersonaRecord, skills) == 0x0C && sizeof(DatPersonaRecord) == 0x30, "record ABI");

/* ---- engine facts ------------------------------------------------------- */
static u16 persona_of[11];      /* each party member's current Persona */
static s16 equipped[11][3];     /* weapon, armor, accessory; -1 empty */
static u16 accessory_skill[16]; /* datItemGrantedSkill by item */
static u16 party_slots[11][8], enemy_slots[64][8];
static unsigned persona_reads;
uint16_t datPersonaGetRecordId(uint16_t party) { assert(party < 11); ++persona_reads; return persona_of[party]; }
/* The retail skill split (game_skills_native.c): below 440 active, below 576 registered. */
int p4_game_skill_registered(uint32_t id) { return id < 576u; }
int p4_game_skill_active(uint32_t id) { return id < 440u; }
int16_t datGetEquippedItemByUnit(int16_t party, int16_t slot)
{
    assert(party >= 0 && party < 11 && slot >= 0 && slot < 3); return equipped[party][slot];
}
static u32 datItemGrantedSkill(s16 item) { return item >= 0 && item < 16 ? accessory_skill[item] : 0u; }
static u32 datCalcGetTableLength(DatUnit *unit) { (void)unit; return 8; }
static u8 *datCalcSkillList(DatUnit *unit)
{
    if (unit->flags & 4) { assert(unit->id < 64); return (u8 *)enemy_slots[unit->id]; }
    assert(unit->id < 11); return (u8 *)party_slots[unit->id];
}
static u8 *datCalcFixedSkillTable(u8 *unit) { return datCalcSkillList((DatUnit *)unit); }
#include "../overrides/static/datCalcHasRequestedSkill.c"
#include "../overrides/static/datCalcHasFixedSkill.c"
#include "../overrides/static/cmpSkillListContains.c"

static void clear_facts(void)
{
    memset(persona_of, 0, sizeof persona_of); memset(equipped, 0xff, sizeof equipped);
    memset(accessory_skill, 0, sizeof accessory_skill);
    memset(party_slots, 0, sizeof party_slots); memset(enemy_slots, 0, sizeof enemy_slots);
    persona_reads = 0;
}
static DatUnit party_unit(u16 id) { DatUnit unit = {0}; unit.id = id; return unit; }
static DatUnit enemy_unit(u16 id) { DatUnit unit = {0}; unit.flags = 4; unit.id = id; return unit; }

/* ---- the table ---------------------------------------------------------- */
static void table_rules(void)
{
    assert(!p4_game_passive_count() && !p4_game_passive_live);
    /* Out-of-range scope, key, skill or owner changes nothing. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_SCOPES, 1, 0x1f6) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 0, 0x1f6) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 9, 0x1f6) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ITEM, 0x8000, 0x1f6) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ENEMY, 0x10000, 0x1f6) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 1, 0) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 1, P4_PASSIVE_SKILL_MAX + 1) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant(NULL, 1, P4_PASSIVE_PARTY, 1, 0x1f6) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("", 1, P4_PASSIVE_PARTY, 1, 0x1f6) == P4_PASSIVE_INVALID);
    char long_owner[P4_PASSIVE_OWNER_SIZE + 1];
    memset(long_owner, 'x', P4_PASSIVE_OWNER_SIZE); long_owner[P4_PASSIVE_OWNER_SIZE] = 0;
    assert(p4_game_passive_grant(long_owner, 1, P4_PASSIVE_PARTY, 1, 0x1f6) == P4_PASSIVE_INVALID);
    assert(!p4_game_passive_count() && !p4_game_passive_live);
    /* Only registered passive skills: a navigator or other active skill, or
     * an unregistered ID, is refused like any invalid tuple. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 1, 0x10c) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 1, 1) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 1, 600) == P4_PASSIVE_INVALID);
    assert(!p4_game_passive_count() && !p4_game_passive_live);
    /* Set semantics per owner; owners hold independently. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 2, 0x1f6) == 1);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 2, 0x1f6) == 0);
    assert(p4_game_passive_grant("b", 2, P4_PASSIVE_PARTY, 2, 0x1f6) == 1);
    assert(p4_game_passive_count() == 2 && p4_game_passive_live == 2 && p4_game_passive_held(P4_PASSIVE_PARTY, 2, 0x1f6));
    assert(p4_game_passive_revoke(1, P4_PASSIVE_PARTY, 2, 0x1f6) == 1);
    assert(p4_game_passive_revoke(1, P4_PASSIVE_PARTY, 2, 0x1f6) == 0);
    assert(p4_game_passive_held(P4_PASSIVE_PARTY, 2, 0x1f6)); /* b's remains */
    assert(p4_game_passive_revoke(2, P4_PASSIVE_PARTY, 2, 0x1f6) == 1);
    assert(!p4_game_passive_held(P4_PASSIVE_PARTY, 2, 0x1f6) && !p4_game_passive_live);
    assert(p4_game_passive_revoke(1, P4_PASSIVE_SCOPES, 2, 0x1f6) == P4_PASSIVE_INVALID);
    /* The listing never depends on the order of grants. */
    static const u32 tuples[][4] = {{P4_PASSIVE_ENEMY, 40, 0x1fb, 2}, {P4_PASSIVE_PARTY, 1, 0x205, 1},
        {P4_PASSIVE_ITEM, 7, 0x1f8, 1}, {P4_PASSIVE_PERSONA, 0x2a, 0x1f7, 2}, {P4_PASSIVE_PARTY, 1, 0x1f6, 2},
        {P4_PASSIVE_PARTY, 1, 0x1f6, 1}};
    const size_t n = sizeof tuples / sizeof tuples[0];
    P4PassiveGrant forward[8], backward[8];
    for (size_t i = 0; i < n; ++i)
        assert(p4_game_passive_grant(tuples[i][3] == 1 ? "a" : "b", tuples[i][3], tuples[i][0], tuples[i][1], tuples[i][2]) == 1);
    for (size_t i = 0; i < n; ++i) assert(!p4_game_passive_at(i, &forward[i]));
    assert(p4_game_passive_at(n, &forward[n]) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_revoke_owner(1) == 3 && p4_game_passive_revoke_owner(2) == 3 && !p4_game_passive_live);
    for (size_t i = n; i-- > 0;)
        assert(p4_game_passive_grant(tuples[i][3] == 1 ? "a" : "b", tuples[i][3], tuples[i][0], tuples[i][1], tuples[i][2]) == 1);
    for (size_t i = 0; i < n; ++i) {
        assert(!p4_game_passive_at(i, &backward[i]));
        assert(forward[i].scope == backward[i].scope && forward[i].key == backward[i].key &&
            forward[i].skill == backward[i].skill && forward[i].load_order == backward[i].load_order &&
            !strcmp(forward[i].owner, backward[i].owner));
        if (i) assert(forward[i - 1].scope < forward[i].scope || (forward[i - 1].scope == forward[i].scope &&
            (forward[i - 1].key < forward[i].key || (forward[i - 1].key == forward[i].key &&
            (forward[i - 1].skill < forward[i].skill || forward[i - 1].load_order < forward[i].load_order)))));
    }
    assert(forward[0].scope == P4_PASSIVE_PARTY && forward[0].skill == 0x1f6 && forward[0].load_order == 1);
    assert(p4_game_passive_revoke_owner(1) == 3 && p4_game_passive_revoke_owner(2) == 3);
    /* A full table refuses, and a revocation makes room again. */
    for (u32 i = 0; i < P4_PASSIVE_GRANT_MAX; ++i)
        assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, i, 0x1f6) == 1);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 999, 0x1f6) == P4_PASSIVE_FULL);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 5, 0x1f6) == 0); /* still held, not refused */
    assert(p4_game_passive_revoke(1, P4_PASSIVE_PERSONA, 5, 0x1f6) == 1);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 999, 0x1f6) == 1);
    assert(p4_game_passive_revoke_owner(1) == P4_PASSIVE_GRANT_MAX && !p4_game_passive_live);
}

/* ---- the game's passive checks ------------------------------------------ */
static int has(DatUnit unit, u32 skill)
{
    const int requested = datCalcHasRequestedSkill(&unit, (s32)skill);
    /* Hit/critical/evasion checks and every other check agree on every unit. */
    assert(datCalcHasFixedSkill((u8 *)&unit, (s32)skill) == requested);
    return requested;
}
static void scoped_checks(void)
{
    clear_facts();
    party_slots[2][3] = 0x1f6; accessory_skill[9] = 0x1f7; equipped[2][2] = 9; enemy_slots[40][0] = 0x1b8;
    persona_of[2] = 0x2a; persona_of[3] = 0x2b; persona_of[1] = 0x10;
    /* Nothing granted: the original search, and the grant table is never asked. */
    assert(has(party_unit(2), 0x1f6) && has(party_unit(2), 0x1f7) && !has(party_unit(2), 0x1f8));
    assert(has(enemy_unit(40), 0x1b8) && !has(enemy_unit(40), 0x1f6) && !has(party_unit(3), 0x1f6));
    assert(!persona_reads);
    /* A member grant reaches that member only, in every check. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 3, 0x1f8) == 1);
    assert(has(party_unit(3), 0x1f8) && !has(party_unit(2), 0x1f8) && !has(enemy_unit(3), 0x1f8));
    assert(!persona_reads); /* No Persona grant exists: no Persona lookup. */
    /* A Persona grant reaches whoever has that Persona now. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 0x2a, 0x1e0) == 1);
    assert(has(party_unit(2), 0x1e0) && !has(party_unit(3), 0x1e0) && persona_reads);
    persona_of[2] = 0x2c;
    assert(!has(party_unit(2), 0x1e0));
    persona_of[3] = 0x2a;
    assert(has(party_unit(3), 0x1e0));
    /* An item grant reaches whoever has the item equipped, in any slot. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ITEM, 9, 0x20b) == 1);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ITEM, 4, 0x20a) == 1);
    assert(has(party_unit(2), 0x20b) && !has(party_unit(2), 0x20a));
    equipped[2][0] = 4;
    assert(has(party_unit(2), 0x20a));
    equipped[2][0] = -1; equipped[2][2] = -1;
    assert(!has(party_unit(2), 0x20b) && !has(party_unit(2), 0x1f7)); /* the accessory left with its grant */
    /* An enemy grant reaches every enemy of that kind and no party member. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ENEMY, 40, 0x1f6) == 1);
    assert(has(enemy_unit(40), 0x1f6) && !has(enemy_unit(41), 0x1f6));
    /* A skill the unit has natively stays had when also granted; revoking the
     * grant never removes the native one. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ENEMY, 40, 0x1b8) == 1);
    assert(p4_game_passive_revoke(1, P4_PASSIVE_ENEMY, 40, 0x1b8) == 1 && has(enemy_unit(40), 0x1b8));
    /* Unrelated skills ask nothing of the natives. */
    persona_reads = 0;
    assert(!has(party_unit(2), 0x1ff) && !persona_reads);
    /* The Persona is read only while a Persona tuple of the asked skill
     * exists: member 2 lacks member 3's 0x1F8 without a Persona lookup,
     * although Persona tuples of other skills exist. */
    persona_reads = 0;
    assert(!has(party_unit(2), 0x1f8) && !persona_reads);
    /* The roster and leader checks: a roster Persona sees its own grants and
     * the protagonist's (member 1 and his equipment); the leader, the
     * protagonist with his current Persona. */
    DatPersonaRecord roster[2] = {{0}};
    roster[0].id = 0x10; roster[1].id = 0x2a;
    assert(!cmpSkillListContains(roster[0].skills, 0x20a) && !P4_PASSIVE_LEADER_GRANTED(0x205));
    roster[0].skills[7] = 0x20a;
    assert(cmpSkillListContains(roster[0].skills, 0x20a)); /* its own slot */
    assert(cmpSkillListContains(roster[1].skills, 0x1e0) && !cmpSkillListContains(roster[0].skills, 0x1e0));
    equipped[1][1] = 4;
    assert(cmpSkillListContains(roster[1].skills, 0x20a) && P4_PASSIVE_LEADER_GRANTED(0x20a));
    equipped[1][1] = -1;
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 1, 0x205) == 1);
    assert(cmpSkillListContains(roster[1].skills, 0x205) && P4_PASSIVE_LEADER_GRANTED(0x205));
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 0x10, 0x206) == 1);
    assert(P4_PASSIVE_LEADER_GRANTED(0x206) && !cmpSkillListContains(roster[1].skills, 0x206));
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 2, 0x207) == 1);
    assert(!P4_PASSIVE_LEADER_GRANTED(0x207) && !cmpSkillListContains(roster[0].skills, 0x207));
    /* Counter, Counterstrike or High Counter (the AI's physical test). */
    const DatUnit enemy40 = enemy_unit(40), member3 = party_unit(3), member2 = party_unit(2);
    assert(P4_PASSIVE_COUNTER_GRANTED(&enemy40) && P4_PASSIVE_COUNTER_GRANTED(&member3) &&
        !P4_PASSIVE_COUNTER_GRANTED(&member2));
    /* The member-only view (the field check's active members) sees member and
     * equipment grants but not the member's Persona's. */
    assert(P4_PASSIVE_UNIT_GRANTED(&member3, 0x1e0) && !P4_PASSIVE_MEMBER_GRANTED(3, 0x1e0));
    assert(P4_PASSIVE_MEMBER_GRANTED(3, 0x1f8) && !P4_PASSIVE_MEMBER_GRANTED(3, 0x20b));
    equipped[3][2] = 9;
    assert(P4_PASSIVE_MEMBER_GRANTED(3, 0x20b));
    equipped[3][2] = -1;
    assert(p4_game_passive_revoke_owner(1) == 8 && !p4_game_passive_live);
    assert(!has(party_unit(3), 0x1f8) && !P4_PASSIVE_LEADER_GRANTED(0x205));
}

/* ---- the added-coverage sources ------------------------------------------ */
static u32 visited[16];
static size_t visited_count;
static int record(void *user, uint32_t skill)
{
    (void)user; assert(visited_count < 16); visited[visited_count++] = skill; return 0;
}
static int stop(void *user, uint32_t skill) { (void)skill; return *(int *)user; }
static void sources(void)
{
    clear_facts();
    u8 slots[16] = {0};
    slots[0] = 0xf6; slots[1] = 0x01; /* 0x1f6 in slot 0 */
    persona_of[2] = 0x2a; equipped[2][0] = 3;
    assert(p4_game_passive_grant("b", 2, P4_PASSIVE_PARTY, 2, 0x1fd) == 1);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 2, 0x1fd) == 1);   /* a second owner */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 0x2a, 0x1f6) == 1); /* already in a slot */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ITEM, 3, 0x1c0) == 1);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ITEM, 3, 0x1f8) == 1);   /* the accessory's skill */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ENEMY, 2, 0x1b9) == 1);  /* enemy 2, not member 2 */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 3, 0x1ba) == 1);
    visited_count = 0;
    assert(!p4_game_passive_unit_sources(0, 2, slots, 8, 0x1f8, record, NULL));
    assert(visited_count == 2 && visited[0] == 0x1c0 && visited[1] == 0x1fd);
    visited_count = 0;
    assert(!p4_game_passive_unit_sources(1, 2, slots, 8, 0, record, NULL));
    assert(visited_count == 1 && visited[0] == 0x1b9);
    int result = 77;
    assert(p4_game_passive_unit_sources(0, 3, slots, 8, 0, stop, &result) == 77);
    assert(p4_game_passive_unit_sources(0, 3, NULL, 8, 0, record, NULL) == P4_PASSIVE_INVALID);
    assert(p4_game_passive_party_has(2, 0x2a, 0x1f6) && !p4_game_passive_party_has(2, 0x2b, 0x1f6));
    assert(p4_game_passive_enemy_has(2, 0x1b9) && !p4_game_passive_enemy_has(3, 0x1b9));
    assert(p4_game_passive_revoke_owner(1) == 6 && p4_game_passive_revoke_owner(2) == 1);
    visited_count = 0;
    assert(!p4_game_passive_unit_sources(0, 2, slots, 8, 0, record, NULL) && !visited_count);
}

/* ---- Growth (overrides/0021e110.c) --------------------------------------- */
typedef struct BtlXpAwards {
    u32 flags; s32 heroExp; s32 rosterExp[12]; u8 rosterPreviews[12][0x88]; s16 unk698; u16 partyIds[4];
    u8 pad6A2[2]; s32 partyExp[4]; u8 partyPreviews[4][0x88];
} BtlXpAwards;
typedef struct BtlXpReward {
    u16 flags; u8 pad02[2]; s32 experience, money, encounterIndex; s16 unk10; u16 partyIds[5]; s32 partyCount, divisor;
    struct { s16 id, count; } items[3]; s32 itemCount; u8 pad34[4]; s32 enemyLevel;
} BtlXpReward;
_Static_assert(sizeof(BtlXpAwards) == 0x8D4 && offsetof(BtlXpAwards, partyPreviews) == 0x6B4 &&
    offsetof(BtlXpAwards, rosterPreviews) == 0x38, "award ABI");
#define MEMBER(type, base, record, field) (*(type *)(uintptr_t)((uintptr_t)(u32)(base) + offsetof(record, field)))
static DatPersonaRecord *roster;
static u32 roster_count;
static u8 *low(s32 address) { return (u8 *)(uintptr_t)(u32)address; }
static u8 datCalcGetLevelById(s32 id) { assert(id == 1); return 20; }
static s32 btlXpComputeAward(u8 level, u8 *reward) { (void)level; (void)reward; return 100; }
static s32 datItemValue(s16 item) { (void)item; return 0; }
static u8 datLevelFromExp(s32 exp) { (void)exp; return 20; }
static s32 datPersonaGetExpById(s32 id) { (void)id; return 0; }
static u8 *datPersonaGetRecord(u16 id) { assert(id == 1); return (u8 *)&roster[0]; }
static u8 *datUnitGetRecord(s32 id) { (void)id; return NULL; }
static s32 datPersonaCountValidMembers(void) { return (s32)roster_count; }
static u8 *datPersonaRosterSlot(s16 index) { assert(index >= 0 && (u32)index < roster_count); return (u8 *)&roster[index]; }
static u8 datPersonaGetLevel(s32 persona) { return ((DatPersonaRecord *)low(persona))->level; }
static s32 datPersonaLeadBoosted(s32 id) { (void)id; return 0; }
static u16 *datPersonaGetSkills(struct DatPersonaRecord *persona) { return persona->skills; }
static s32 datPersonaGetExp(s32 persona) { (void)persona; return 0; }
static s32 datPersonaGetExpForLevel(u16 *persona, u16 level) { (void)persona; (void)level; return 1 << 30; }
static void datPersonaPreviewLevelUp(u8 *persona, u8 *out, s32 award) { (void)persona; (void)out; (void)award; abort(); }
static s32 datGetFlag(s32 flag) { assert(flag == 0x38); return 0; }
int p4_game_reward_native_active(void) { return 0; }
int32_t p4_game_reward_native_hero(uint32_t level, int32_t stock) { (void)level; return stock; }
int32_t p4_game_reward_native_party(uint32_t member, uint32_t level, int32_t stock) { (void)member; (void)level; return stock; }
int32_t p4_game_reward_native_persona(uint32_t persona, uint32_t level, uint32_t equipped_, int32_t stock)
{ (void)persona; (void)level; (void)equipped_; return stock; }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpointer-to-int-cast"
#pragma clang diagnostic ignored "-Wint-to-pointer-cast"
#pragma clang diagnostic ignored "-Wint-conversion"
#include "../overrides/0021e110.c"
#pragma clang diagnostic pop
static void growth(u8 *arena)
{
    clear_facts();
    BtlXpAwards *awards = (BtlXpAwards *)arena;
    BtlXpReward *reward = (BtlXpReward *)(arena + 0x1000);
    roster = (DatPersonaRecord *)(arena + 0x2000);
    roster_count = 3;
    memset(roster, 0, 3 * sizeof *roster); memset(reward, 0, sizeof *reward);
    for (u32 i = 0; i < 3; ++i) { roster[i].id = (u16)(0x10 * (i + 1)); roster[i].level = 10; }
    roster[1].skills[2] = 0x1ff; /* Growth 1 in a slot */
    /* Retail: the active Persona the full award, Growth 1 a quarter, none nothing. */
    btlXpPrepareAwards((u8 *)awards, (u8 *)reward);
    assert(awards->rosterExp[0] == 100 && awards->rosterExp[1] == 25 && awards->rosterExp[2] == 0);
    /* A Persona's own Growth grant: Growth 3 the full award; a granted Growth 1
     * it already has counts once; a granted Growth 2 adds a half. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 0x30, 0x201) == 1);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 0x20, 0x1ff) == 1);
    btlXpPrepareAwards((u8 *)awards, (u8 *)reward);
    assert(awards->rosterExp[0] == 100 && awards->rosterExp[1] == 25 && awards->rosterExp[2] == 100);
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PERSONA, 0x20, 0x200) == 1);
    btlXpPrepareAwards((u8 *)awards, (u8 *)reward);
    assert(awards->rosterExp[1] == 75);
    /* The protagonist's own and his equipment's Growth reach every inactive
     * Persona; another member's never does; the active Persona never grows. */
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_PARTY, 2, 0x201) == 1);
    btlXpPrepareAwards((u8 *)awards, (u8 *)reward);
    assert(awards->rosterExp[0] == 100 && awards->rosterExp[1] == 75 && awards->rosterExp[2] == 100);
    equipped[1][2] = 6;
    assert(p4_game_passive_grant("a", 1, P4_PASSIVE_ITEM, 6, 0x1ff) == 1);
    btlXpPrepareAwards((u8 *)awards, (u8 *)reward);
    assert(awards->rosterExp[0] == 100 && awards->rosterExp[1] == 75 && awards->rosterExp[2] == 125);
    assert(p4_game_passive_revoke_owner(1) == 5);
    btlXpPrepareAwards((u8 *)awards, (u8 *)reward);
    assert(awards->rosterExp[0] == 100 && awards->rosterExp[1] == 25 && awards->rosterExp[2] == 0);
}

int main(void)
{
    table_rules();
    scoped_checks();
    sources();
    /* Growth's award code carries guest addresses in 32-bit integers. */
    const size_t arena_size = 0x4000;
#ifdef _WIN32
    u8 *arena = VirtualAlloc((void *)(uintptr_t)0x20000000u, arena_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    u8 *arena = mmap((void *)(uintptr_t)0x20000000u, arena_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) arena = NULL;
#endif
    if (!arena || (uintptr_t)arena > 0x7fffffffu - arena_size) {
        puts("game_passive_grant: table, scoped checks and sources passed; Growth skipped (no low arena)");
        return 77;
    }
    growth(arena);
    puts("game_passive_grant: table, scoped checks, sources and Growth passed; not native gameplay qualification");
    return 0;
}
