/* Passive grants: the native (scope, key, skill) table the game's passive
 * checks ask after a unit's own slots and accessory (include/p4pc/
 * game_passive_grant.h, docs/modding/tables.md "Passive grants"). The tuples
 * stay sorted by (scope, key, skill, load order), so a query is a binary
 * search and the listing never depends on the order of grants. Main-thread
 * game state; queries never allocate, call into Lua or use RNG. */
#include "p4pc/game_passive_grant.h"
#include <string.h>

/* The same native accessors datCalcSkillList 0x0023E140 and the affinity
 * sources use: a party member's current Persona record ID (0x00109360) and
 * its equipped items, weapon 0, armor 1 and accessory 2 (-1 when empty). */
extern uint16_t datPersonaGetRecordId(uint16_t party_id);
extern int16_t datGetEquippedItemByUnit(int16_t party_id, int16_t slot);
/* The loaded tables' skill classes (game_skills_native.c): the retail split
 * at any time, an added skill once its tables are published. Only a new
 * grant asks; a skill that is not a registered passive is refused. */
extern int p4_game_skill_registered(uint32_t id);
extern int p4_game_skill_active(uint32_t id);
#define EQUIPMENT_SLOTS 3
#define DATUNIT_ENEMY 4u      /* DatUnit flags bit 2 */
#define DATUNIT_PARTY_LIMIT 0xBu /* datCalcSkillList's party bound */
#define PERSONA_SKILLS 0x0Cu  /* DatPersonaRecord.skills; the Persona ID is at +2 */

typedef struct Entry {
    uint32_t load_order;
    uint16_t key, skill;
    uint8_t scope;
    char owner[P4_PASSIVE_OWNER_SIZE];
} Entry;
static Entry entries[P4_PASSIVE_GRANT_MAX];
static size_t entry_count;
static uint16_t skill_refs[P4_PASSIVE_SKILL_MAX + 1];
/* Tuples per scope, and per scope and skill: a check looks up a party
 * member's Persona only while a Persona tuple of that skill exists, and its
 * equipment only while an item tuple of that skill does. */
static uint16_t scope_refs[P4_PASSIVE_SCOPES];
static uint16_t scope_skill_refs[P4_PASSIVE_SCOPES][P4_PASSIVE_SKILL_MAX + 1];
uint32_t p4_game_passive_live;

static int valid(uint32_t scope, uint32_t key, uint32_t skill)
{
    if (!skill || skill > P4_PASSIVE_SKILL_MAX) return 0;
    switch (scope) {
    case P4_PASSIVE_PARTY: return key >= 1u && key <= P4_PASSIVE_PARTY_MAX;
    case P4_PASSIVE_ITEM: return key <= P4_PASSIVE_ITEM_MAX;
    case P4_PASSIVE_PERSONA: case P4_PASSIVE_ENEMY: return key <= P4_PASSIVE_KEY_MAX;
    default: return 0;
    }
}
static int compare(const Entry *entry, uint32_t scope, uint32_t key, uint32_t skill, uint32_t load_order)
{
    if (entry->scope != scope) return entry->scope < scope ? -1 : 1;
    if (entry->key != key) return entry->key < key ? -1 : 1;
    if (entry->skill != skill) return entry->skill < skill ? -1 : 1;
    if (entry->load_order != load_order) return entry->load_order < load_order ? -1 : 1;
    return 0;
}
/* The first tuple not ordered before (scope, key, skill, load_order). */
static size_t lower_bound(uint32_t scope, uint32_t key, uint32_t skill, uint32_t load_order)
{
    size_t low = 0, high = entry_count;
    while (low < high) {
        const size_t middle = low + (high - low) / 2;
        if (compare(&entries[middle], scope, key, skill, load_order) < 0) low = middle + 1;
        else high = middle;
    }
    return low;
}
static int held(uint32_t scope, uint32_t key, uint32_t skill)
{
    const size_t at = lower_bound(scope, key, skill, 0);
    return at < entry_count && entries[at].scope == scope && entries[at].key == key && entries[at].skill == skill;
}
static void count(const Entry *entry, int delta)
{
    skill_refs[entry->skill] = (uint16_t)(skill_refs[entry->skill] + delta);
    scope_refs[entry->scope] = (uint16_t)(scope_refs[entry->scope] + delta);
    scope_skill_refs[entry->scope][entry->skill] = (uint16_t)(scope_skill_refs[entry->scope][entry->skill] + delta);
}
static void remove_at(size_t at)
{
    count(&entries[at], -1);
    memmove(&entries[at], &entries[at + 1], (entry_count - at - 1) * sizeof entries[0]);
    --entry_count;
    p4_game_passive_live = (uint32_t)entry_count;
}

int p4_game_passive_grant(const char *owner, uint32_t load_order, uint32_t scope, uint32_t key, uint32_t skill)
{
    if (!owner || !valid(scope, key, skill)) return P4_PASSIVE_INVALID;
    if (!p4_game_skill_registered(skill) || p4_game_skill_active(skill)) return P4_PASSIVE_INVALID;
    const size_t length = strlen(owner);
    if (!length || length >= P4_PASSIVE_OWNER_SIZE) return P4_PASSIVE_INVALID;
    const size_t at = lower_bound(scope, key, skill, load_order);
    if (at < entry_count && !compare(&entries[at], scope, key, skill, load_order)) return 0;
    if (entry_count == P4_PASSIVE_GRANT_MAX) return P4_PASSIVE_FULL;
    memmove(&entries[at + 1], &entries[at], (entry_count - at) * sizeof entries[0]);
    Entry *entry = &entries[at];
    *entry = (Entry){.load_order = load_order, .key = (uint16_t)key, .skill = (uint16_t)skill, .scope = (uint8_t)scope};
    memcpy(entry->owner, owner, length + 1);
    count(entry, 1);
    ++entry_count;
    p4_game_passive_live = (uint32_t)entry_count;
    return 1;
}
int p4_game_passive_revoke(uint32_t load_order, uint32_t scope, uint32_t key, uint32_t skill)
{
    if (!valid(scope, key, skill)) return P4_PASSIVE_INVALID;
    const size_t at = lower_bound(scope, key, skill, load_order);
    if (at == entry_count || compare(&entries[at], scope, key, skill, load_order)) return 0;
    remove_at(at);
    return 1;
}
size_t p4_game_passive_revoke_owner(uint32_t load_order)
{
    size_t removed = 0;
    for (size_t at = entry_count; at-- > 0;)
        if (entries[at].load_order == load_order) { remove_at(at); ++removed; }
    return removed;
}
int p4_game_passive_held(uint32_t scope, uint32_t key, uint32_t skill)
{
    return valid(scope, key, skill) && skill_refs[skill] && held(scope, key, skill);
}
size_t p4_game_passive_count(void) { return entry_count; }
int p4_game_passive_at(size_t index, P4PassiveGrant *out)
{
    if (!out || index >= entry_count) return P4_PASSIVE_INVALID;
    const Entry *entry = &entries[index];
    *out = (P4PassiveGrant){entry->scope, entry->key, entry->skill, entry->load_order, entry->owner};
    return P4_PASSIVE_OK;
}

/* A party member's view. The Persona is looked up only while a Persona tuple
 * of this skill exists, the equipment only while an item tuple of it does. */
enum { PERSONA_LOOKUP, PERSONA_KNOWN, PERSONA_NONE };
static int member_has(uint32_t member, int persona_mode, uint32_t persona, uint32_t skill)
{
    if (held(P4_PASSIVE_PARTY, member, skill)) return 1;
    if (persona_mode != PERSONA_NONE && scope_skill_refs[P4_PASSIVE_PERSONA][skill]) {
        if (persona_mode == PERSONA_LOOKUP) persona = datPersonaGetRecordId((uint16_t)member);
        if (held(P4_PASSIVE_PERSONA, persona, skill)) return 1;
    }
    if (scope_skill_refs[P4_PASSIVE_ITEM][skill]) {
        for (int16_t slot = 0; slot < EQUIPMENT_SLOTS; ++slot) {
            const int16_t item = datGetEquippedItemByUnit((int16_t)member, slot);
            if (item >= 0 && held(P4_PASSIVE_ITEM, (uint32_t)item, skill)) return 1;
        }
    }
    return 0;
}
int p4_game_passive_party_has(uint32_t member, uint32_t persona, uint32_t skill)
{
    if (skill > P4_PASSIVE_SKILL_MAX || !skill_refs[skill] || member >= DATUNIT_PARTY_LIMIT) return 0;
    return member_has(member, PERSONA_KNOWN, persona, skill);
}
int p4_game_passive_enemy_has(uint32_t enemy, uint32_t skill)
{
    if (skill > P4_PASSIVE_SKILL_MAX || !skill_refs[skill]) return 0;
    return held(P4_PASSIVE_ENEMY, enemy, skill);
}

int p4_game_passive_native_unit(const void *unit, uint32_t skill)
{
    if (!unit || skill > P4_PASSIVE_SKILL_MAX || !skill_refs[skill]) return 0;
    const uint16_t *words = unit;
    if (words[0] & DATUNIT_ENEMY) return held(P4_PASSIVE_ENEMY, words[1], skill);
    if (words[1] >= DATUNIT_PARTY_LIMIT) return 0;
    return member_has(words[1], PERSONA_LOOKUP, 0, skill);
}
int p4_game_passive_native_roster(const void *skills, uint32_t skill)
{
    if (!skills || skill > P4_PASSIVE_SKILL_MAX || !skill_refs[skill]) return 0;
    const uint16_t persona = *(const uint16_t *)((const uint8_t *)skills - PERSONA_SKILLS + 2u);
    return member_has(1u, PERSONA_KNOWN, persona, skill);
}
int p4_game_passive_native_leader(uint32_t skill)
{
    if (skill > P4_PASSIVE_SKILL_MAX || !skill_refs[skill]) return 0;
    return member_has(1u, PERSONA_LOOKUP, 0, skill);
}
int p4_game_passive_native_member(uint32_t member, uint32_t skill)
{
    if (skill > P4_PASSIVE_SKILL_MAX || !skill_refs[skill] || member >= DATUNIT_PARTY_LIMIT) return 0;
    return member_has(member, PERSONA_NONE, 0, skill);
}

static uint32_t le16(const uint8_t *bytes) { return (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8; }
int p4_game_passive_unit_sources(uint32_t enemy, uint32_t unit_id, const uint8_t *skills, uint32_t skill_count,
    uint32_t accessory_skill, P4PassiveVisitFn visit, void *user)
{
    if (!visit || (skill_count && !skills)) return P4_PASSIVE_INVALID;
    if (!entry_count) return P4_PASSIVE_OK;
    if (!enemy && unit_id >= DATUNIT_PARTY_LIMIT) return P4_PASSIVE_OK;
    uint32_t persona = 0;
    int16_t items[EQUIPMENT_SLOTS] = {-1, -1, -1};
    if (!enemy) {
        if (scope_refs[P4_PASSIVE_PERSONA]) persona = datPersonaGetRecordId((uint16_t)unit_id);
        if (scope_refs[P4_PASSIVE_ITEM])
            for (int16_t slot = 0; slot < EQUIPMENT_SLOTS; ++slot)
                items[slot] = datGetEquippedItemByUnit((int16_t)unit_id, slot);
    }
    uint16_t seen[P4_PASSIVE_GRANT_MAX];
    size_t seen_count = 0;
    for (size_t at = 0; at < entry_count; ++at) {
        const Entry *entry = &entries[at];
        int visible;
        if (enemy) visible = entry->scope == P4_PASSIVE_ENEMY && entry->key == unit_id;
        else if (entry->scope == P4_PASSIVE_PARTY) visible = entry->key == unit_id;
        else if (entry->scope == P4_PASSIVE_PERSONA) visible = entry->key == persona;
        else if (entry->scope == P4_PASSIVE_ITEM)
            visible = entry->key == (uint32_t)items[0] || entry->key == (uint32_t)items[1] ||
                entry->key == (uint32_t)items[2];
        else visible = 0;
        if (!visible || entry->skill == accessory_skill) continue;
        uint32_t slot = 0;
        while (slot < skill_count && le16(skills + slot * 2u) != entry->skill) ++slot;
        if (slot < skill_count) continue;
        size_t place = seen_count;
        while (place > 0 && seen[place - 1] > entry->skill) --place;
        if (place > 0 && seen[place - 1] == entry->skill) continue;
        memmove(&seen[place + 1], &seen[place], (seen_count - place) * sizeof seen[0]);
        seen[place] = entry->skill;
        ++seen_count;
    }
    for (size_t at = 0; at < seen_count; ++at) {
        const int result = visit(user, seen[at]);
        if (result) return result;
    }
    return P4_PASSIVE_OK;
}
