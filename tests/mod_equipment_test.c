/* Equipment exchange (include/p4pc/mod_equipment.h) over a fake game state:
 * whole validation before queueing, the Camp menu's commit order at the safe
 * point, waiting out a battle, and a stale request refused whole instead of
 * reaching an accessor that would refuse fatally. The game accessors are the
 * stubs below; datAssignEquippedItem's stub enforces the expanded setter's
 * own check, as the real one refuses fatally. */
#include "p4pc/mod_entry.h"
#include "p4pc/mod_equipment.h"
#include "game_items_internal.h"
#include "guest_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
uint32_t p4_mod_safe_point;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_equipment: FAIL line%d: %s\n", __LINE__, #x); failures++; } } while (0)

/* Fake game. Retail: 1, 2 Yu swords; 3 a Yosuke sword; 0x150 Yu armor; 0x400 a
 * consumable. Expanded: 0x800 a forged Yu sword, 0x801 a forged Yosuke sword,
 * 0x802 a row that is not usable now; 0x803 has no row. Yu and Yosuke joined. */
enum { ITEMS = 0x900 };
static int32_t counts[ITEMS];
static int16_t equipped[11][3];
static int battle, joined[11];
static uint8_t rows[ITEMS][68];
static int permission[ITEMS][11];
static int usable[ITEMS];
static int fake_items;
static char calls[256];

_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{
    fprintf(stderr, "mod_equipment: unexpected failure owner %zu: %s\n", owner, reason);
    exit(1);
}
static void trace(const char *format, int a, int b)
{
    size_t used = strlen(calls);
    snprintf(calls + used, sizeof calls - used, format, a, b);
}
int32_t datItemCount(int16_t item)
{
    if (item <= 0 || item >= ITEMS || !rows[item][0]) { fprintf(stderr, "count of unknown %d\n", item); exit(1); }
    return counts[item];
}
void datItemSetCount(int16_t item, int32_t count)
{
    if (item <= 0 || item >= ITEMS || !rows[item][0]) { fprintf(stderr, "set count of unknown %d\n", item); exit(1); }
    counts[item] = count > 99 ? 99 : count;
    trace("count %d=%d;", item, counts[item]);
}
int32_t datIdBand(int16_t item) { return item >= 0x800 ? rows[item][1] : item < 0x100 ? 0 : item < 0x200 ? 1 : 2; }
int32_t datEntryHasFlag(int16_t item, int16_t actor) { return permission[item][actor]; }
int32_t datMemberIsUnlocked(int64_t member)
{
    if (member <= 0 || member >= 11) { fprintf(stderr, "member assert %d\n", (int)member); exit(1); }
    return joined[member];
}
int16_t datGetEquippedItemByUnit(int16_t id, int16_t slot) { return equipped[id][slot]; }
void datAssignEquippedItem(int16_t id, int16_t slot, int16_t item)
{
    if (item >= 0x600 && (!usable[item] || rows[item][1] != slot || !permission[item][id])) {
        fprintf(stderr, "mod_equipment: the expanded setter would refuse item %d\n", item);
        exit(1);
    }
    equipped[id][slot] = item;
    trace("assign %d<-%d;", id, item);
}
void *kwlnTaskGetTaskByName(const void *name) { (void)name; return battle ? (void *)&battle : NULL; }
const uint8_t *p4_game_tables_items_native_metadata(uint32_t id)
{
    if (id >= ITEMS || !rows[id][0]) { fprintf(stderr, "native metadata refusal %u\n", id); exit(1); }
    return rows[id];
}
P4GameItems *p4_game_tables_items_active(void) { return (P4GameItems *)&fake_items; }
const P4ItemRow *p4_game_tables_items_metadata(const P4GameItems *items, uint32_t id)
{
    return items && id < ITEMS && rows[id][0] ? (const P4ItemRow *)rows[id] : NULL;
}
int p4_game_tables_items_runtime_use(const P4GameItems *items, uint32_t id)
{
    return items && id < ITEMS && usable[id] ? P4_TABLE_OK : P4_TABLE_NOT_READY;
}
int p4_game_tables_items_equipment_check(const P4GameItems *items, uint32_t unit, uint32_t slot, uint32_t id)
{
    if (!items || id < 0x600 || id >= ITEMS || !rows[id][0]) return P4_TABLE_INVALID;
    return rows[id][1] == slot && permission[id][unit] ? P4_TABLE_OK : P4_TABLE_INVALID;
}

static void row(int id, int band, int character)
{
    rows[id][0] = 1;
    rows[id][1] = (uint8_t)band;
    if (character) permission[id][character] = 1;
    usable[id] = 1;
}
static void reset(void)
{
    memset(counts, 0, sizeof counts);
    memset(equipped, 0, sizeof equipped);
    memset(rows, 0, sizeof rows);
    memset(permission, 0, sizeof permission);
    memset(usable, 0, sizeof usable);
    memset(joined, 0, sizeof joined);
    joined[1] = joined[2] = 1;
    row(1, 0, 1); row(2, 0, 1); row(3, 0, 2); row(0x150, 1, 1); row(0x400, 2, 1);
    row(0x800, 0, 1); row(0x801, 0, 2); row(0x802, 0, 1);
    usable[0x802] = 0;
    equipped[1][0] = 1;
    equipped[2][0] = 3;
    counts[1] = 3;
    counts[0x800] = 1;
    counts[0x801] = 1;
    counts[0x802] = 1;
    counts[0x150] = 1;
    counts[0x400] = 1;
    calls[0] = 0;
    battle = 0;
}
static int refused(P4ModEquip e, const char *expect)
{
    char reason[192] = "";
    int32_t before[ITEMS];
    int16_t worn[11][3];
    memcpy(before, counts, sizeof before);
    memcpy(worn, equipped, sizeof worn);
    int ok = p4_mod_equipment_queue(9, &e, reason, sizeof reason) && strstr(reason, expect) &&
        !p4_mod_equipment_pending(9) && !memcmp(before, counts, sizeof before) && !memcmp(worn, equipped, sizeof worn);
    if (!ok) fprintf(stderr, "mod_equipment: expected \"%s\", got \"%s\"\n", expect, reason);
    return ok;
}

int main(void)
{
    if (p4t_guest_init()) return 1;
    char reason[192];
    const P4ModEquip forged = {.character = 1, .slot = 0, .item = 0x800};
    P4ModEquip seen;
    int16_t previous = -7;
    int32_t item = 0;
    reset();

    /* Reads. */
    CHECK(!p4_mod_equipment_get(1, 0, &item, reason, sizeof reason) && item == 1);
    CHECK(p4_mod_equipment_get(5, 0, &item, reason, sizeof reason) && strstr(reason, "no equipment"));
    CHECK(p4_mod_equipment_get(1, 3, &item, reason, sizeof reason) && strstr(reason, "Slot"));
    CHECK(!p4_mod_equipment_check(&forged, &previous, reason, sizeof reason) && previous == 1);

    /* Whole validation: every bad field refuses and writes or queues nothing. */
    P4ModEquip bad = forged; bad.character = 5;
    CHECK(refused(bad, "no equipment"));
    bad = forged; bad.character = 3;
    CHECK(refused(bad, "not joined"));
    bad = forged; bad.slot = 3;
    CHECK(refused(bad, "Slot"));
    bad = forged; bad.item = 0x803;
    CHECK(refused(bad, "not in the item table"));
    bad = forged; bad.item = 0x802;
    CHECK(refused(bad, "not in the item table"));
    /* The stale-result case: another fighter's forged weapon, or a misfit slot. */
    bad = forged; bad.item = 0x801;
    CHECK(refused(bad, "can equip"));
    bad = forged; bad.character = 2;
    CHECK(refused(bad, "can equip"));
    bad = forged; bad.item = 0x150;
    CHECK(refused(bad, "can equip"));
    bad = forged; bad.item = 0x400; bad.slot = 2;
    CHECK(refused(bad, "can equip"));
    bad = forged; bad.item = 1;
    CHECK(refused(bad, "already equipped"));
    bad = forged; bad.item = 2;
    CHECK(refused(bad, "No unequipped copy"));
    counts[1] = 99;
    CHECK(refused(forged, "99 are held"));
    counts[1] = 3;
    battle = 1;
    CHECK(refused(forged, "battle"));
    battle = 0;
    CHECK(!(p4_mod_safe_point & P4_MOD_SAFE_EQUIP));

    /* Allowed: the menu's order, after waiting out a battle. */
    CHECK(!p4_mod_equipment_queue(9, &forged, reason, sizeof reason));
    CHECK(p4_mod_equipment_pending(9) && (p4_mod_safe_point & P4_MOD_SAFE_EQUIP));
    CHECK(p4_mod_equipment_queue(10, &forged, reason, sizeof reason) && strstr(reason, "pending"));
    battle = 1;
    CHECK(p4_mod_equipment_apply_pending() == 1);
    CHECK(equipped[1][0] == 1 && counts[0x800] == 1 && counts[1] == 3 && !calls[0]);
    battle = 0;
    CHECK(p4_mod_equipment_apply_pending() == 0);
    CHECK(!strcmp(calls, "count 1=4;assign 1<-2048;count 2048=0;"));
    CHECK(equipped[1][0] == 0x800 && counts[0x800] == 0 && counts[1] == 4);
    CHECK(p4_mod_equipment_queue(9, &forged, reason, sizeof reason) && strstr(reason, "previous equipment result"));
    CHECK(p4_mod_equipment_take_result(9, &seen, reason, sizeof reason) == P4_MOD_EQUIP_EQUIPPED && reason[0] == 0);
    CHECK(seen.character == 1 && seen.slot == 0 && seen.item == 0x800 && seen.previous == 1);
    CHECK(p4_mod_equipment_take_result(9, NULL, NULL, 0) == P4_MOD_EQUIP_NONE);

    /* State that changed after queueing refuses whole: nothing is written and
     * no accessor that refuses fatally is reached. */
    reset();
    CHECK(!p4_mod_equipment_queue(9, &forged, reason, sizeof reason));
    counts[0x800] = 0;
    CHECK(p4_mod_equipment_apply_pending() == 0);
    CHECK(!calls[0] && equipped[1][0] == 1 && counts[1] == 3);
    CHECK(p4_mod_equipment_take_result(9, &seen, reason, sizeof reason) == P4_MOD_EQUIP_REFUSED);
    CHECK(strstr(reason, "No unequipped copy") && seen.item == 0x800);
    reset();
    CHECK(!p4_mod_equipment_queue(9, &forged, reason, sizeof reason));
    usable[0x800] = 0;
    CHECK(p4_mod_equipment_apply_pending() == 0 && !calls[0]);
    CHECK(p4_mod_equipment_take_result(9, NULL, reason, sizeof reason) == P4_MOD_EQUIP_REFUSED);
    CHECK(strstr(reason, "not in the item table"));
    reset();
    CHECK(!p4_mod_equipment_queue(9, &forged, reason, sizeof reason));
    counts[1] = 99;
    CHECK(p4_mod_equipment_apply_pending() == 0 && !calls[0] && counts[0x800] == 1);
    CHECK(p4_mod_equipment_take_result(9, NULL, reason, sizeof reason) == P4_MOD_EQUIP_REFUSED);

    /* An empty slot returns nothing. */
    reset();
    equipped[1][2] = 0;
    row(0x201, 2, 1);
    counts[0x201] = 1;
    P4ModEquip ring = {.character = 1, .slot = 2, .item = 0x201};
    CHECK(!p4_mod_equipment_queue(9, &ring, reason, sizeof reason));
    CHECK(p4_mod_equipment_apply_pending() == 0);
    CHECK(!strcmp(calls, "assign 1<-513;count 513=0;") && equipped[1][2] == 0x201);
    CHECK(p4_mod_equipment_take_result(9, &seen, NULL, 0) == P4_MOD_EQUIP_EQUIPPED && seen.previous == 0);

    /* The owner's teardown drops a queued request. */
    reset();
    CHECK(!p4_mod_equipment_queue(9, &forged, reason, sizeof reason));
    p4_mod_equipment_clear_owner(9);
    CHECK(!p4_mod_equipment_pending(9) && p4_mod_equipment_apply_pending() == 0);
    CHECK(!calls[0] && equipped[1][0] == 1);

    if (failures) return 1;
    puts("mod_equipment: ok");
    return 0;
}
