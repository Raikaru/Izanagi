/* Side ailments (docs/modding/tables.md "Side ailments"): per-battle state of
 * the <status-ailment> leaves, outside DatUnit.bad. Main-thread battle code. */
#include "p4pc/game_side_ailment.h"
#include "p4pc/game_side_ailment_native.h"
#include "p4pc/gmem.h"
#include "p4pc/package_boot.h"
#include "game_tables_runtime_internal.h"
#include <stdio.h>
#include <string.h>

#define UNIT_SLOTS 32u          /* party 4, enemies 5 and summons, with room */
#define DAT_UNIT_BAD 0xCu       /* DatUnit.bad */
#define BAD_DEAD 0x80000u
#define BAD_AILMENTS 0xFFFFFu   /* 002397D0: (bad << 44) >> 44, Dizzy and dead included, Down not */

typedef struct UnitState {
    uint32_t unit;                          /* guest DatUnit; 0 is a free slot */
    const P4SideAilmentDef *held, *landed;  /* landed: this action's mark */
    uint16_t landings[P4_SIDE_AILMENT_LIMIT]; /* by definition index */
} UnitState;
static const P4GameTables *prepared, *published;
static UnitState units[UNIT_SLOTS];
static int in_battle;
static P4SideAilmentObserver observer;
static unsigned delivering;

size_t p4_side_ailment_count(void) { return published ? p4_game_tables_side_ailment_count(published) : 0; }
const P4SideAilmentDef *p4_side_ailment_at(size_t index)
{ return published ? p4_game_tables_side_ailment_at(published, index) : NULL; }
const P4SideAilmentDef *p4_side_ailment_find_leaf(uint32_t leaf)
{ return published ? p4_game_tables_find_side_ailment(published, leaf) : NULL; }
const P4SideAilmentDef *p4_side_ailment_find_key(const char *key)
{
    const size_t count = p4_side_ailment_count();
    for (size_t i = 0; key && i < count; ++i) {
        const P4SideAilmentDef *def = p4_game_tables_side_ailment_at(published, i);
        if (!strcmp(def->key, key)) return def;
    }
    return NULL;
}
static size_t def_index(const P4SideAilmentDef *def)
{
    size_t i = 0;
    while (p4_game_tables_side_ailment_at(published, i) != def) ++i;
    return i;
}
/* An authored row authorizes its use before any field of it is read. */
static void use(const P4SideAilmentDef *def)
{
    const int result = p4_game_tables_runtime_use_row(published, P4_TABLE_SOURCE_SIDE_AILMENT, def);
    if (!result) return;
    char reason[192];
    snprintf(reason, sizeof reason, "Side ailment %s refused (%d): %s.", def->key, result, p4_game_tables_error(result));
    p4_package_boot_runtime_failed(reason);
}
static void notify(uint32_t unit, const P4SideAilmentDef *def, uint32_t event, uint32_t reason)
{
    if (!observer) return;
    ++delivering;
    observer(unit, def->leaf, event, reason);
    --delivering;
}
static UnitState *slot_find(uint32_t unit)
{
    for (size_t i = 0; unit && i < UNIT_SLOTS; ++i)
        if (units[i].unit == unit) return &units[i];
    return NULL;
}
static UnitState *slot_get(uint32_t unit)
{
    UnitState *slot = slot_find(unit);
    for (size_t i = 0; !slot && i < UNIT_SLOTS; ++i)
        if (!units[i].unit) { slot = &units[i]; slot->unit = unit; }
    return slot;
}
static void release(UnitState *slot, uint32_t reason)
{
    const P4SideAilmentDef *held = slot->held;
    const uint32_t unit = slot->unit;
    memset(slot, 0, sizeof *slot);
    if (held) notify(unit, held, P4_SIDE_EVENT_CLEARED, reason);
}
static void release_all(uint32_t reason)
{
    for (size_t i = 0; i < UNIT_SLOTS; ++i) if (units[i].unit) release(&units[i], reason);
}
/* The refusals set and mark_landed share; a matching mark admits its set. */
static int admit(uint32_t unit, uint32_t leaf, const P4SideAilmentDef **out, int mark)
{
    if (!published || !in_battle || delivering) return P4_TABLE_NOT_READY;
    const P4SideAilmentDef *def = p4_game_tables_find_side_ailment(published, leaf);
    if (!def) return P4_TABLE_NOT_FOUND;
    if (!unit) return P4_TABLE_INVALID;
    use(def);
    const uint32_t bad = P4_G32(unit + DAT_UNIT_BAD);
    if (bad & BAD_DEAD) return P4_TABLE_INVALID;
    const UnitState *slot = slot_find(unit);
    if (slot && (slot->held || (slot->landed && (mark || slot->landed != def)))) return P4_TABLE_CONFLICT;
    if ((def->flags & P4_SIDE_SINGLE_SLOT) && (bad & BAD_AILMENTS)) return P4_TABLE_CONFLICT;
    *out = def;
    return P4_TABLE_OK;
}
int p4_side_ailment_query(uint32_t unit, const P4SideAilmentDef **def)
{
    const UnitState *slot = published && in_battle ? slot_find(unit) : NULL;
    if (!slot || !slot->held) return 0;
    use(slot->held);
    if (def) *def = slot->held;
    return 1;
}
int p4_side_ailment_set(uint32_t unit, uint32_t leaf, uint32_t reason)
{
    if (reason != P4_SIDE_REASON_RIDER && reason != P4_SIDE_REASON_MOD) return P4_TABLE_INVALID;
    const P4SideAilmentDef *def;
    int result = admit(unit, leaf, &def, 0);
    if (result) return result;
    UnitState *slot = slot_get(unit);
    if (!slot) return P4_TABLE_NOMEM;
    slot->held = def;
    slot->landed = NULL;
    uint16_t *landings = &slot->landings[def_index(def)];
    if (*landings < UINT16_MAX) ++*landings;
    notify(unit, def, P4_SIDE_EVENT_LANDED, reason);
    return P4_TABLE_OK;
}
uint32_t p4_side_ailment_clear(uint32_t unit, uint32_t reason)
{
    if (reason < P4_SIDE_REASON_CURE || reason > P4_SIDE_REASON_RESET || delivering) return P4_SIDE_DEFAULT_NONE;
    UnitState *slot = published ? slot_find(unit) : NULL;
    if (!slot || !slot->held) return P4_SIDE_DEFAULT_NONE;
    const P4SideAilmentDef *held = slot->held;
    use(held);
    slot->held = NULL;
    notify(unit, held, P4_SIDE_EVENT_CLEARED, reason);
    return held->leaf;
}
uint32_t p4_side_ailment_landings(uint32_t unit, uint32_t leaf)
{
    const P4SideAilmentDef *def = p4_side_ailment_find_leaf(leaf);
    const UnitState *slot = def ? slot_find(unit) : NULL;
    return slot ? slot->landings[def_index(def)] : 0;
}
int p4_side_ailment_turn_lost(uint32_t unit)
{
    const UnitState *slot = published && in_battle ? slot_find(unit) : NULL;
    if (!slot || !slot->held) return 0;
    use(slot->held);
    if (!(slot->held->flags & P4_SIDE_LOSE_TURN)) return 0;
    notify(unit, slot->held, P4_SIDE_EVENT_TURN_LOST, P4_SIDE_REASON_NONE);
    return 1;
}
int p4_side_ailment_mark_landed(uint32_t unit, uint32_t leaf)
{
    const P4SideAilmentDef *def;
    int result = admit(unit, leaf, &def, 1);
    if (result) return result;
    UnitState *slot = slot_get(unit);
    if (!slot) return P4_TABLE_NOMEM;
    slot->landed = def;
    return P4_TABLE_OK;
}
int p4_side_ailment_landed(uint32_t unit, uint32_t *leaf)
{
    const UnitState *slot = published ? slot_find(unit) : NULL;
    if (!slot || !slot->landed) return 0;
    if (leaf) *leaf = slot->landed->leaf;
    return 1;
}
void p4_side_ailment_action_end(void)
{
    if (!published) return;
    for (size_t i = 0; i < UNIT_SLOTS; ++i) units[i].landed = NULL;
}
void p4_side_ailment_set_observer(P4SideAilmentObserver sink) { observer = sink; }

void p4_game_side_ailment_native_battle_begin(void)
{
    if (!published) return;
    release_all(P4_SIDE_REASON_RESET); /* only a battle that was never released */
    in_battle = 1;
}
void p4_game_side_ailment_native_unit_exit(uint32_t unit)
{
    UnitState *slot = published ? slot_find(unit) : NULL;
    if (slot) release(slot, P4_SIDE_REASON_BATTLE_END);
}
void p4_game_side_ailment_native_battle_end(void)
{
    if (!published) return;
    release_all(P4_SIDE_REASON_BATTLE_END);
    in_battle = 0;
}

int p4_game_side_ailment_prepare(const P4GameTables *tables)
{
    if (!tables) return P4_TABLE_INVALID;
    if (!p4_game_tables_revision(tables)) return P4_TABLE_NOT_READY;
    if (prepared) return prepared == tables ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (p4_game_tables_side_ailment_count(tables)) prepared = tables;
    return P4_TABLE_OK;
}
void p4_game_side_ailment_publish(void) { published = prepared; }
void p4_game_side_ailment_session_end(void)
{
    if (published) release_all(P4_SIDE_REASON_RESET);
    memset(units, 0, sizeof units);
    published = prepared = NULL;
    in_battle = 0;
}
