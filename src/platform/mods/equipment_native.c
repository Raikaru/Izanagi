/* Equipment exchange (include/p4pc/mod_equipment.h). Every check reads the
 * game's own state through its accessors, the ones the Camp equipment menu
 * uses: joined members (datMemberIsUnlocked 0x00353DC0, as the member list
 * 0x00353CE0 builds it), held counts (datItemCount 0x00106600, with
 * PortTables' expanded IDs), the slot band (datIdBand 0x00106C80) and the
 * character permission (datEntryHasFlag 0x00106C30), as cmpEquipBuildItemList
 * 0x00131730 lists candidates. An expanded item must also pass the side-state
 * equipment check and runtime use that datAssignEquippedItem's expanded path
 * refuses fatally, so it cannot fail after validation.
 *
 * The commit is the menu's own (0x0013D5D0): the replaced item's count rises
 * by one, the new item is assigned (datAssignEquippedItem 0x00106D40), then
 * its count drops by one. Where the menu silently drops a replaced item
 * already held 99 times, this refuses instead, so nothing the player owns is
 * lost. Validation leaves no failure point after it. */
#include "p4pc/mod_equipment.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_failure.h"
#include "p4pc/game_items.h"
#include "p4pc/game_items_native.h"
#include "p4pc/guest.h"
#include "game_items_internal.h"
#include <stdarg.h>
#include <stdio.h>

extern int32_t datItemCount(int16_t item);
extern void datItemSetCount(int16_t item, int32_t count);
extern int32_t datIdBand(int16_t item);
extern int32_t datEntryHasFlag(int16_t item, int16_t actor);
extern int32_t datMemberIsUnlocked(int64_t member);
extern int16_t datGetEquippedItemByUnit(int16_t id, int16_t slot);
extern void datAssignEquippedItem(int16_t id, int16_t slot, int16_t item);
extern void *kwlnTaskGetTaskByName(const void *name);

/* btlTaskCreate's root task name (gp 0xa088): the task exists while a battle runs. */
#define BATTLE_TASK_NAME 0x00763178u
/* Retail equipment IDs: weapons 0x000, armor 0x100, accessories 0x200 (cmpEquipBuildItemList's range). */
#define RETAIL_EQUIPMENT_END 0x300u

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
static int in_battle(void)
{
    return kwlnTaskGetTaskByName(P4_G2H(BATTLE_TASK_NAME)) != NULL;
}
static int combatant(int character)
{
    return character >= 1 && character <= 8 && character != 5;
}
/* A row every accessor below reads without refusing: a retail row, or an
 * expanded row that is registered and usable now. */
static int known(uint32_t item)
{
    if (item == 0 || item > P4_ITEM_MAX_ID) return 0;
    if (item < P4_ITEM_RETAIL_COUNT) return p4_game_tables_items_native_metadata(item) != NULL;
    const P4GameItems *items = p4_game_tables_items_active();
    return items && p4_game_tables_items_metadata(items, item) && p4_game_tables_items_runtime_use(items, item) == P4_TABLE_OK;
}
/* Equipment of this slot the character may wear; the item is known. */
static int fits(uint32_t item, int character, int slot)
{
    if (item < P4_ITEM_RETAIL_COUNT) {
        if (item >= RETAIL_EQUIPMENT_END) return 0;
    } else if (p4_game_tables_items_equipment_check(p4_game_tables_items_active(), (uint32_t)character,
                   (uint32_t)slot, item) != P4_TABLE_OK) {
        return 0;
    }
    return datIdBand((int16_t)item) == slot && datEntryHasFlag((int16_t)item, (int16_t)character) != 0;
}

int p4_mod_equipment_get(int character, int slot, int32_t *item, char *reason, size_t capacity)
{
    if (!combatant(character)) return fail(reason, capacity, "Character %d has no equipment", character);
    if (slot < 0 || slot > 2) return fail(reason, capacity, "Slot %d is not 0 (weapon), 1 (armor) or 2 (accessory)", slot);
    if (item) *item = datGetEquippedItemByUnit((int16_t)character, (int16_t)slot);
    return 0;
}

int p4_mod_equipment_check(const P4ModEquip *e, int16_t *previous, char *reason, size_t capacity)
{
    if (!e) return fail(reason, capacity, "No equipment request");
    if (!combatant(e->character)) return fail(reason, capacity, "Character %u has no equipment", e->character);
    if (!datMemberIsUnlocked(e->character)) return fail(reason, capacity, "Character %u has not joined", e->character);
    if (e->slot > 2) return fail(reason, capacity, "Slot %u is not 0 (weapon), 1 (armor) or 2 (accessory)", e->slot);
    if (!known(e->item)) return fail(reason, capacity, "Item %u is not in the item table", e->item);
    if (!fits(e->item, e->character, e->slot))
        return fail(reason, capacity, "Item %u is not slot %u equipment character %u can equip", e->item, e->slot, e->character);
    const int16_t old = datGetEquippedItemByUnit((int16_t)e->character, (int16_t)e->slot);
    if (old == (int16_t)e->item) return fail(reason, capacity, "Item %u is already equipped", e->item);
    if (datItemCount((int16_t)e->item) < 1) return fail(reason, capacity, "No unequipped copy of item %u", e->item);
    if (old > 0) {
        if (!known((uint32_t)old)) return fail(reason, capacity, "The equipped item %d is not in the item table", old);
        if (datItemCount(old) >= 99)
            return fail(reason, capacity, "The equipped item %d cannot return to the items: 99 are held", old);
    }
    if (previous) *previous = old;
    return 0;
}

typedef struct Slot {
    int queued, outcome;
    size_t owner;
    P4ModEquip equip;
    char reason[192];
} Slot;
/* One request in flight across all mods; outcomes are kept per owner until taken. */
static Slot slot;
enum { OUTCOMES = 8 };
static Slot outcomes[OUTCOMES];

static Slot *outcome_for(size_t owner, int create)
{
    Slot *free_slot = NULL;
    for (int i = 0; i < OUTCOMES; ++i) {
        if (outcomes[i].outcome && outcomes[i].owner == owner) return &outcomes[i];
        if (!outcomes[i].outcome && !free_slot) free_slot = &outcomes[i];
    }
    return create ? free_slot : NULL;
}

int p4_mod_equipment_queue(size_t owner, const P4ModEquip *e, char *reason, size_t capacity)
{
    if (in_battle()) return fail(reason, capacity, "Equipment cannot be changed during a battle");
    if (slot.queued) return fail(reason, capacity, "Another equipment change is still pending");
    if (outcome_for(owner, 0)) return fail(reason, capacity, "Take the previous equipment result first");
    if (!outcome_for(owner, 1)) return fail(reason, capacity, "Too many mods hold equipment results");
    if (p4_mod_equipment_check(e, NULL, reason, capacity)) return -1;
    slot = (Slot){1, 0, owner, *e, ""};
    slot.equip.previous = 0;
    p4_mod_safe_point |= P4_MOD_SAFE_EQUIP;
    return 0;
}

int p4_mod_equipment_apply_pending(void)
{
    if (!slot.queued) return 0;
    if (in_battle()) return 1;
    Slot done = slot;
    slot.queued = 0;
    done.queued = 0;
    Slot *out = outcome_for(done.owner, 1);
    if (!out) p4_mod_failure(done.owner, "Equipment outcome slots exhausted");
    P4ModEquip *e = &done.equip;
    /* The state may have moved since queueing (another mod, a script): refuse whole. */
    if (p4_mod_equipment_check(e, &e->previous, done.reason, sizeof done.reason)) {
        done.outcome = P4_MOD_EQUIP_REFUSED;
        *out = done;
        return 0;
    }
    if (e->previous > 0) datItemSetCount(e->previous, datItemCount(e->previous) + 1);
    datAssignEquippedItem((int16_t)e->character, (int16_t)e->slot, (int16_t)e->item);
    datItemSetCount((int16_t)e->item, datItemCount((int16_t)e->item) - 1);
    fprintf(stderr, "p4pc: mods: equip by owner %zu: character %u slot %u item %u (replaced %d)\n",
        done.owner, e->character, e->slot, e->item, e->previous > 0 ? e->previous : 0);
    done.outcome = P4_MOD_EQUIP_EQUIPPED;
    done.reason[0] = 0;
    *out = done;
    return 0;
}

int p4_mod_equipment_take_result(size_t owner, P4ModEquip *e, char *reason, size_t capacity)
{
    Slot *out = outcome_for(owner, 0);
    if (!out) return P4_MOD_EQUIP_NONE;
    int outcome = out->outcome;
    if (e) *e = out->equip;
    if (reason && capacity) snprintf(reason, capacity, "%s", out->reason);
    *out = (Slot){0};
    return outcome;
}

int p4_mod_equipment_pending(size_t owner)
{
    return slot.queued && slot.owner == owner;
}

void p4_mod_equipment_clear_owner(size_t owner)
{
    if (slot.queued && slot.owner == owner) slot.queued = 0;
    Slot *out = outcome_for(owner, 0);
    if (out) *out = (Slot){0};
}
