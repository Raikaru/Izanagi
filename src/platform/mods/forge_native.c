/* Weapon fusion transaction (include/p4pc/mod_forge.h). Every check reads the
 * game's own state through its accessors: the roster (p4_mod_persona_stock),
 * recruited members (datMemberIsUnlocked 0x00353DC0), item counts
 * (datItemCount 0x00106600, with PortTables' expanded IDs), the equipment
 * band (datIdBand 0x00106C80: 0 is a weapon), the item's character
 * permission bit (categoryMask +0x30, bit characterId + 4, as datEntryHasFlag
 * 0x00106C30 tests) and money (datGetMoney 0x00105ED0). The commit removes the
 * Persona the way fusion removes a non-lead material (fclCombineConsumeMaterials
 * 0x0030F4F0: datPersonaRemoveRosterMemberById 0x0010AD80), then the vessel,
 * the fee and adds the result; validation leaves no failure point after it. */
#include "p4pc/mod_forge.h"
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_failure.h"
#include "p4pc/mod_persona.h"
#include "p4pc/game_items_native.h"
#include "p4pc/guest.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern int32_t datItemCount(int16_t item);
extern void datItemSetCount(int16_t item, int32_t count);
extern int32_t datIdBand(int16_t item);
extern uint32_t datGetMoney(void);
extern void datSetMoney(uint32_t amount);
extern int32_t datPersonaRemoveRosterMemberById(int32_t persona);
extern int32_t datMemberIsUnlocked(int64_t member);
extern void *kwlnTaskGetTaskByName(const void *name);

/* btlTaskCreate's root task name (gp 0xa088): the task exists while a battle runs. */
#define BATTLE_TASK_NAME 0x00763178u
#define CATEGORY_MASK 0x30u
#define MONEY_CAP 9999999u

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
/* A weapon the character may equip: a known row, weapon band, permission bit. */
static int weapon_for(uint32_t item, int character)
{
    if (item == 0 || item > P4_ITEM_MAX_ID) return 0;
    const uint8_t *row = p4_game_tables_items_native_metadata(item);
    if (!row || datIdBand((int16_t)item) != 0) return 0;
    uint32_t mask;
    memcpy(&mask, row + CATEGORY_MASK, 4);
    return (mask >> (character + 4)) & 1u;
}

uint32_t p4_mod_forge_vessel_count(int character, uint32_t item)
{
    if (!combatant(character) || !datMemberIsUnlocked(character) || !weapon_for(item, character)) return 0;
    const int32_t count = datItemCount((int16_t)item);
    return count > 0 ? (uint32_t)count : 0;
}

static int validate(const P4ModForge *f, char *reason, size_t capacity)
{
    if (!f) return fail(reason, capacity, "No forge request");
    if (!combatant(f->character)) return fail(reason, capacity, "Character %u does not fight", f->character);
    if (!datMemberIsUnlocked(f->character)) return fail(reason, capacity, "Character %u has not joined", f->character);
    P4ModStockEntry stock[P4_MOD_STOCK_MAX];
    int count = p4_mod_persona_stock(stock, P4_MOD_STOCK_MAX), found = -1;
    for (int i = 0; i < count; ++i)
        if (stock[i].persona == f->persona) found = i;
    if (found < 0) return fail(reason, capacity, "Persona %u is not carried", f->persona);
    if (stock[found].lead) return fail(reason, capacity, "The equipped Persona cannot be consumed");
    if (count < 2) return fail(reason, capacity, "The last Persona cannot be consumed");
    if (!weapon_for(f->vessel, f->character))
        return fail(reason, capacity, "Item %u is not a weapon character %u can equip", f->vessel, f->character);
    if (datItemCount((int16_t)f->vessel) < 1) return fail(reason, capacity, "No unequipped copy of item %u", f->vessel);
    if (!weapon_for(f->result, f->character))
        return fail(reason, capacity, "Item %u is not a weapon character %u can equip", f->result, f->character);
    if (f->result == f->vessel) return fail(reason, capacity, "The result must differ from the vessel");
    if (datItemCount((int16_t)f->result) >= 99) return fail(reason, capacity, "Item %u is already at 99", f->result);
    if (f->fee > MONEY_CAP || f->fee > datGetMoney()) return fail(reason, capacity, "Not enough money for the fee");
    return 0;
}

typedef struct Slot {
    int queued, outcome;
    size_t owner;
    P4ModForge forge;
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

int p4_mod_forge_queue(size_t owner, const P4ModForge *f, char *reason, size_t capacity)
{
    if (in_battle()) return fail(reason, capacity, "Weapons cannot be forged during a battle");
    if (slot.queued) return fail(reason, capacity, "Another forge is still pending");
    if (outcome_for(owner, 0)) return fail(reason, capacity, "Take the previous forge result first");
    if (!outcome_for(owner, 1)) return fail(reason, capacity, "Too many mods hold forge results");
    if (validate(f, reason, capacity)) return -1;
    slot = (Slot){1, 0, owner, *f, ""};
    p4_mod_safe_point |= P4_MOD_SAFE_FORGE;
    return 0;
}

int p4_mod_forge_apply_pending(void)
{
    if (!slot.queued) return 0;
    if (in_battle()) return 1;
    Slot done = slot;
    slot.queued = 0;
    const P4ModForge *f = &done.forge;
    Slot *out = outcome_for(done.owner, 1);
    if (!out) p4_mod_failure(done.owner, "Forge outcome slots exhausted");
    done.queued = 0;
    /* The challenge rule's seam: asked before safe-point revalidation or spending. */
    if (!p4_mod_authorize_weapon_fusion(P4_MOD_WEAPON_FUSION_COMMIT, f->character, f->vessel, f->persona,
            f->signature, done.reason, sizeof done.reason)) {
        done.outcome = P4_MOD_FORGE_REFUSED;
        *out = done;
        return 0;
    }
    /* Recruitment can change while a request is queued. Do not consume a
     * vessel or Persona for a fighter who can no longer receive the result. */
    if (!datMemberIsUnlocked(f->character)) {
        fail(done.reason, sizeof done.reason, "Character %u has not joined", f->character);
        done.outcome = P4_MOD_FORGE_REFUSED;
        *out = done;
        return 0;
    }
    char reason[192];
    if (validate(f, reason, sizeof reason)) p4_mod_failure(done.owner, reason);
    datPersonaRemoveRosterMemberById(f->persona);
    datItemSetCount((int16_t)f->vessel, datItemCount((int16_t)f->vessel) - 1);
    datSetMoney(datGetMoney() - f->fee);
    datItemSetCount((int16_t)f->result, datItemCount((int16_t)f->result) + 1);
    fprintf(stderr, "p4pc: mods: forge by owner %zu: character %u persona %u vessel %u -> item %u (fee %u)\n",
        done.owner, f->character, f->persona, f->vessel, f->result, f->fee);
    done.outcome = P4_MOD_FORGE_FORGED;
    done.reason[0] = 0;
    *out = done;
    return 0;
}

int p4_mod_forge_take_result(size_t owner, P4ModForge *f, char *reason, size_t capacity)
{
    Slot *out = outcome_for(owner, 0);
    if (!out) return P4_MOD_FORGE_NONE;
    int outcome = out->outcome;
    if (f) *f = out->forge;
    if (reason && capacity) snprintf(reason, capacity, "%s", out->reason);
    *out = (Slot){0};
    return outcome;
}

int p4_mod_forge_pending(size_t owner)
{
    return slot.queued && slot.owner == owner;
}

void p4_mod_forge_clear_owner(size_t owner)
{
    if (slot.queued && slot.owner == owner) slot.queued = 0;
    Slot *out = outcome_for(owner, 0);
    if (out) *out = (Slot){0};
}
