/* Party Persona records for mods (include/p4pc/mod_party.h). The record is the
 * party slot's DatPersonaRecord (datPersonaGetRecord): level +4, EXP +8, eight
 * skill halfwords +0xC, five stat bytes +0x1C. A character's level is its
 * Persona's (datCalcGetLevel), so its HP/SP maxima follow the written level. */
#include "p4pc/mod_party.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_failure.h"
#include "p4pc/mod_field_host.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern void *datPersonaGetRecord(uint16_t id);
extern uint32_t datPersonaGetExpForLevel(void *persona, uint16_t level);
extern uint32_t datCalcGetHpById(int16_t id);
extern uint32_t datCalcGetSpById(int16_t id);
extern uint16_t datCalcMaxHpById(int16_t id);
extern uint16_t datCalcMaxSpById(int16_t id);
extern void datSetUnitHp(int16_t id, int16_t hp);
extern void datSetUnitSp(int16_t id, int16_t sp);
extern void *kwlnTaskGetTaskByName(const void *name);

enum { LEVEL = 4, EXP = 8, SKILLS = 0xC, STATS = 0x1C, SKILL_SLOTS = 8, STAT_COUNT = 5 };
/* btlTaskCreate's root task name (gp 0xa088): the task exists while a battle runs. */
#define BATTLE_TASK_NAME 0x00763178u

static int fail(char *reason, size_t capacity, const char *format, ...)
{
    if (reason && capacity) {
        va_list args;
        va_start(args, format);
        vsnprintf(reason, capacity, format, args);
        va_end(args);
    }
    return -1;
}
static uint8_t *record_of(int id)
{
    if (id < 2 || id > 10) return NULL;
    uint8_t *record = (uint8_t *)datPersonaGetRecord((uint16_t)id);
    if (!record) return NULL;
    uint16_t persona;
    memcpy(&persona, record + 2, 2);
    return persona ? record : NULL;
}

int p4_mod_party_read(int id, P4ModPartyRecord *out)
{
    uint8_t *record = record_of(id);
    if (!record || !out) return -1;
    memcpy(&out->persona, record + 2, 2);
    out->level = record[LEVEL];
    memcpy(&out->exp, record + EXP, 4);
    memcpy(out->stats, record + STATS, STAT_COUNT);
    memcpy(out->skills, record + SKILLS, sizeof out->skills);
    return 0;
}

static int in_battle(void)
{
    return kwlnTaskGetTaskByName(P4_G2H(BATTLE_TASK_NAME)) != NULL;
}

/* The whole record is checked against this Persona's own curve and the active
 * (expanded) skill registry before anything is written. */
static int validate(uint8_t *record, int id, const P4ModPartyRecord *in, char *reason, size_t capacity)
{
    if (!record || !in) return fail(reason, capacity, "Party id %d has no Persona record", id);
    uint16_t persona;
    memcpy(&persona, record + 2, 2);
    if (in->persona != persona) return fail(reason, capacity, "A party record keeps its Persona (%u)", persona);
    if (in->level < 1 || in->level > 99) return fail(reason, capacity, "Level %u is outside 1..99", in->level);
    uint32_t low = in->level >= 2 ? datPersonaGetExpForLevel(record, in->level) : 0;
    uint32_t high = in->level < 99 ? datPersonaGetExpForLevel(record, (uint16_t)(in->level + 1)) : UINT32_MAX;
    if (in->exp < low || in->exp >= high)
        return fail(reason, capacity, "EXP %u is outside level %u's band %u..%u", in->exp, in->level, low, high - 1);
    for (int i = 0; i < STAT_COUNT; ++i)
        if (in->stats[i] < 1 || in->stats[i] > 99) return fail(reason, capacity, "Stat %d is outside 1..99", i + 1);
    int ended = 0;
    for (int i = 0; i < SKILL_SLOTS; ++i) {
        uint16_t skill = in->skills[i];
        if (!skill) {
            ended = 1;
            continue;
        }
        if (ended) return fail(reason, capacity, "Skills must fill the first slots");
        if (!p4_game_skill_registered(skill)) return fail(reason, capacity, "Skill %u is not registered", skill);
        for (int j = 0; j < i; ++j)
            if (in->skills[j] == skill) return fail(reason, capacity, "Skill %u appears twice", skill);
    }
    return 0;
}

static void write_record(uint8_t *record, int id, const P4ModPartyRecord *in)
{
    record[LEVEL] = in->level;
    memcpy(record + EXP, &in->exp, 4);
    memcpy(record + STATS, in->stats, STAT_COUNT);
    memcpy(record + SKILLS, in->skills, sizeof in->skills);
    /* Current HP/SP never exceed the maxima of the written level. */
    uint16_t hp = datCalcMaxHpById((int16_t)id), sp = datCalcMaxSpById((int16_t)id);
    if (datCalcGetHpById((int16_t)id) > hp) datSetUnitHp((int16_t)id, (int16_t)hp);
    if (datCalcGetSpById((int16_t)id) > sp) datSetUnitSp((int16_t)id, (int16_t)sp);
}

/* Changes to one Persona record apply in the order they were asked for, across
 * p4.party.set here and the party actions queued by game_party.c: each queued
 * change takes the next ticket. */
static uint64_t last_ticket;
uint64_t p4_mod_party_ticket(void)
{
    return ++last_ticket;
}
static uint64_t (*first_action)(uint32_t id);
void p4_mod_party_set_actions(uint64_t (*first)(uint32_t id))
{
    first_action = first;
}

/* One queued write per party id, applied at the next frame safe point outside
 * battle; a second mod's write to an id with a pending write is refused. */
typedef struct Pending {
    int queued;
    size_t owner;
    uint64_t ticket;
    P4ModPartyRecord record;
} Pending;
static Pending pending[11];

uint64_t p4_mod_party_write_ticket(uint32_t id, P4ModPartyRecord *record)
{
    if (id < 2 || id > 10 || !pending[id].queued) return 0;
    if (record) *record = pending[id].record;
    return pending[id].ticket;
}

int p4_mod_party_queue(size_t owner, int id, const P4ModPartyRecord *in, char *reason, size_t capacity)
{
    if (in_battle()) return fail(reason, capacity, "Party records cannot change during a battle");
    if (validate(record_of(id), id, in, reason, capacity)) return -1;
    if (pending[id].queued && pending[id].owner != owner)
        return fail(reason, capacity, "Another mod's change to party id %d is still pending", id);
    /* The owner's newer write replaces its pending one and takes its place in the
     * order: party actions asked for in between apply before it. */
    pending[id] = (Pending){1, owner, p4_mod_party_ticket(), *in};
    p4_mod_safe_point |= P4_MOD_SAFE_PARTY;
    return 0;
}

int p4_mod_party_apply_pending(void)
{
    int waiting = 0;
    for (int id = 2; id <= 10; ++id) {
        if (!pending[id].queued) continue;
        /* A write waits, without holding other ids: for a battle to end; while the
         * skill-choice panel shows this Persona (its skill list is the panel's copy,
         * which the native replace task writes back into the record); and behind a
         * party action to this Persona asked for before it (that action applies at
         * the services step of a frame boundary, this write at a later one). */
        const uint64_t action = first_action ? first_action((uint32_t)id) : 0;
        if (in_battle() || p4_mod_field_host_member() == (uint32_t)id || (action && action < pending[id].ticket)) {
            waiting = 1;
            continue;
        }
        char reason[160];
        uint8_t *record = record_of(id);
        size_t owner = pending[id].owner;
        pending[id].queued = 0;
        /* The record may have changed since the call: a write that no longer
         * fits it is the caller's failure, never a partial or clamped write. */
        if (validate(record, id, &pending[id].record, reason, sizeof reason))
            p4_mod_failure(owner, reason);
        write_record(record, id, &pending[id].record);
        fprintf(stderr, "p4pc: mods: party record %d set by owner %zu: level %u\n", id, owner, pending[id].record.level);
    }
    return waiting;
}

void p4_mod_party_clear_owner(size_t owner)
{
    for (int id = 2; id <= 10; ++id)
        if (pending[id].queued && pending[id].owner == owner) pending[id].queued = 0;
}
