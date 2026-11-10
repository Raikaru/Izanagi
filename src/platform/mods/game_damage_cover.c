/* Damage cover: <damage-cover> rows as final-damage stage modifiers
 * (docs/modding/tables.md "Damage cover"). Main-thread battle code. */
#include "p4pc/game_damage_cover.h"
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/gmem.h"
#include "p4pc/package_boot.h"
#include "game_tables_runtime_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The two lists datCalcSkillList 0x0023E140 returns: it reads only the unit's
 * side flag and id, so the stage's side and id select the same list. Enemy
 * rows follow the relocated UNIT0 metadata base up to the native enemy limit. */
extern uint16_t *datPersonaGetSkillsByPcId(uint16_t pc_id);
#define UNIT_SKILLS 8u /* datCalcGetTableLength 0x0023E130 */
#define ENEMY_METADATA 0x007644b4u /* the UNIT0 enemy metadata base, 60-byte rows */
/* Eight skills, three items and the accessory's granted skill. */
#define UNIT_SOURCES 12u

typedef struct CoverOwner {
    char owner[P4_DAMAGE_OWNER_SIZE];
    uint32_t load_order, handle;
} CoverOwner;
static const P4GameTables *cover_tables;
static CoverOwner *owners;
static size_t owner_count, registered;

static const uint8_t *unit_skill_list(uint32_t enemy, uint32_t id)
{
    if (!enemy) return id < 0xBu ? (const uint8_t *)datPersonaGetSkillsByPcId((uint16_t)id) : NULL;
    if (id >= p4_game_enemies_native_limit()) return NULL;
    const uint32_t metadata = P4_G32(ENEMY_METADATA);
    return metadata ? (const uint8_t *)P4_G2H(metadata + id * 0x3Cu + 0xEu) : NULL;
}
typedef struct CoverVisit {
    const P4GameTables *tables;
    const char *owner;
    uint32_t element;
    const P4DamageCoverBinding *best[UNIT_SOURCES]; /* one per family */
    size_t count;
} CoverVisit;
static int cover_visit(void *user, uint32_t kind, uint32_t source)
{
    CoverVisit *visit = user;
    const P4DamageCoverBinding *row = p4_game_tables_find_damage_cover(visit->tables,
        kind, source, visit->element, P4_CHANNEL_DAMAGE);
    if (!row) return P4_TABLE_OK;
    const char *owner = p4_game_tables_damage_cover_row_owner(visit->tables, row);
    if (!owner) return P4_TABLE_INVALID;
    if (strcmp(owner, visit->owner)) return P4_TABLE_OK;
    for (size_t i = 0; i < visit->count; ++i)
        if (!strcmp(visit->best[i]->family, row->family)) {
            if (row->percent > visit->best[i]->percent) visit->best[i] = row;
            return P4_TABLE_OK;
        }
    if (visit->count == UNIT_SOURCES) return P4_TABLE_INVALID;
    visit->best[visit->count++] = row;
    return P4_TABLE_OK;
}
int p4_game_damage_cover_factor(const P4GameTables *tables, const char *owner,
    uint32_t enemy, uint32_t unit_id, uint32_t element_id, int runtime, double *factor)
{
    if (!tables || !owner || !factor) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    const uint8_t *skills = unit_skill_list(enemy, unit_id);
    if (!skills) return P4_TABLE_NOT_FOUND;
    CoverVisit visit = {.tables = tables, .owner = owner, .element = element_id};
    int result = p4_game_elements_unit_sources(skills, UNIT_SKILLS, enemy, unit_id, cover_visit, &visit);
    if (result) return result;
    if (!visit.count) return 0;
    double value = 1.0;
    for (size_t i = 0; i < visit.count; ++i) {
        if (runtime) {
            result = p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_COVER, visit.best[i]);
            if (result) return result;
        }
        value *= (double)(100u - visit.best[i]->percent) / 100.0;
    }
    *factor = value;
    return 1;
}
static int cover_modifier(void *user, const P4GameDamageHit *hit, double *value)
{
    const CoverOwner *owner = user;
    if (!hit->element_valid || !cover_tables) return 0;
    double factor;
    const int result = p4_game_damage_cover_factor(cover_tables, owner->owner,
        hit->target_side == P4_DAMAGE_SIDE_ENEMY, hit->target_id, hit->element_id, 1, &factor);
    if (result < 0) {
        char reason[192];
        snprintf(reason, sizeof reason, "Damage cover of %s refused (%d): %s.",
            owner->owner, result, p4_game_tables_error(result));
        p4_package_boot_runtime_failed(reason);
        return 0;
    }
    if (!result) return 0;
    *value = factor;
    return 1;
}
int p4_game_damage_cover_prepare(const P4GameTables *tables, P4DamageCoverLoadOrderFn load_order)
{
    if (!tables || !load_order) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    if (cover_tables) return cover_tables == tables ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    const size_t rows = p4_game_tables_damage_cover_count(tables);
    if (!rows) return P4_TABLE_OK;
    CoverOwner *plan = calloc(rows, sizeof *plan);
    if (!plan) return P4_TABLE_NOMEM;
    size_t count = 0;
    for (size_t i = 0; i < rows; ++i) {
        const char *owner = p4_game_tables_damage_cover_owner(tables, i);
        if (!owner || strlen(owner) >= sizeof plan->owner) { free(plan); return P4_TABLE_INVALID; }
        size_t known = 0;
        while (known < count && strcmp(plan[known].owner, owner)) ++known;
        if (known < count) continue;
        strcpy(plan[count].owner, owner);
        plan[count].load_order = load_order(owner);
        ++count;
    }
    owners = plan; owner_count = count; cover_tables = tables;
    return P4_TABLE_OK;
}
static void unregister_all(void)
{
    while (registered) p4_game_damage_unregister(owners[--registered].handle);
}
void p4_game_damage_cover_publish(void)
{
    if (!owners || registered) return;
    for (size_t i = 0; i < owner_count; ++i) {
        const P4GameDamageModifierDesc desc = {owners[i].owner, P4_DAMAGE_COVER_PRIORITY,
            owners[i].load_order, P4_DAMAGE_MULTIPLY, cover_modifier, &owners[i]};
        if (p4_game_damage_register(&desc, &owners[i].handle)) {
            unregister_all();
            p4_package_boot_runtime_failed("Damage cover could not register its final-damage modifier.");
            return;
        }
        ++registered;
    }
}
void p4_game_damage_cover_session_end(void)
{
    unregister_all();
    free(owners);
    owners = NULL; owner_count = 0; cover_tables = NULL;
}
