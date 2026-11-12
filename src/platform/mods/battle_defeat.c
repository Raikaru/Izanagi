/* Battle defeat handler (include/p4pc/mod_defeat.h, p4.battle.defeat).
 *
 * Two native observers, created disabled with the first handler:
 *
 *   btlTaskCreate (0x00192e90) before: the battle-start snapshot. Every
 *   playable character's DatUnit record (0x30 bytes: HP, SP, the ailment word,
 *   buffs, tactics; the protagonist at 0x007973C4, party IDs 2..10 at
 *   0x00796E50 + (ID - 2) * 0x88 + 4, datUnitGetRecord 0x00105510) and each
 *   enemy group of the setup (a self-contained 0x12C-byte DatUnitEc,
 *   datUnitCreateEncounterGroup 0x00231630; summons are added into it and
 *   removed from it in place). A field battle's groups belong to the field
 *   (FldUnit_ReleaseSlot frees them after state 12), so they are still live
 *   when the battle is lost.
 *
 *   FldEvent_FieldToBattle (0x00172e00) before: state 12 is the first state
 *   after the battle task ended. With the field party's leader dead
 *   (datUnitGroupIsDefeated(D_007EF9F8): KO bit 0x80000 or HP 0) it starts
 *   the Game Over procedure. Before it runs, the handler answers:
 *     game over  the state stays 12 (or returns to 12 after a hold);
 *     hold       the state becomes 9: cases 6..9 only advance it, so the
 *                next poll is at state 10 and asks again;
 *     retry      the snapshot is copied back and the state becomes 10: the
 *                body calls btlTaskCreate with the same setup again.
 *   Nothing else in the work changes between state 10 and state 12, so the
 *   retried battle starts exactly as the lost one did, at the same place.
 *
 * Items used, Analyze knowledge, the random generator and everything outside
 * those records are not part of the snapshot. */
#include "p4pc/mod_defeat.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_failure.h"
#include "p4pc/mod_hooks.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BATTLE_CREATE 0x00192e90u   /* btlTaskCreate(setup) */
#define FIELD_TO_BATTLE 0x00172e00u /* FldEvent_FieldToBattle(task) */
#define TASK_WORK 0x38u
#define WORK_STATE 0x04u
#define WORK_SETUP 0x50u            /* FldEventBattle, the 0x3C-byte btlTaskCreate setup */
#define WORK_RETURN_MODE 0x10cu
#define SETUP_PARTY 0x04u           /* four PC group pointers */
#define SETUP_ENEMIES 0x14u         /* three EC group pointers */
#define FIELD_PARTY 0x007EF9F8u     /* the field leader's PC group */
#define HERO_RECORD 0x007973C4u
#define PARTY_SLOTS 0x00796E50u
#define PARTY_SLOT_SIZE 0x88u
#define UNIT_SIZE 0x30u
#define GROUP_SIZE 0x12Cu
#define GENUS_PC 0
#define GENUS_EC 1
#define UNIT_HP 0x08u
#define UNIT_SP 0x0Au
#define UNIT_BAD 0x0Cu
#define UNIT_KO 0x80000u
enum { STATE_HOLD = 9, STATE_BATTLE = 10, STATE_RESULT = 12 };
enum { RECORDS = 10, GROUPS = 3 };

struct P4ModDefeatHandler {
    size_t owner;
    int enabled, removed;
    P4ModDefeatFn callback;
    void *user;
};
static P4ModDefeatHandler *handler;
static P4ModHook *begin_hook, *field_hook;
static unsigned depth;

static struct {
    int valid;
    uint32_t setup;
    uint8_t records[RECORDS][UNIT_SIZE];
    uint32_t groups[GROUPS];
    uint8_t group_bytes[GROUPS][GROUP_SIZE];
    int members;
    P4ModDefeatMember party[4];
} snapshot;

static struct {
    int active;
    uint32_t work, encounter, polls;
    int source, allowed;
    char reason[128];
} pending;

static void *guest(uint32_t address, uint32_t size)
{
#ifdef P4_GUEST64
    if (address < 0x00100000u || address > P4_GUEST_SIZE - size) return NULL;
#else
    if (address < 0x00100000u || address > UINT32_MAX - size) return NULL;
#endif
    return P4_G2H(address);
}
static uint32_t word(uint32_t address)
{
    uint32_t value = 0;
    void *host = guest(address, 4);
    if (host) memcpy(&value, host, 4);
    return value;
}
static uint16_t half(uint32_t address)
{
    uint16_t value = 0;
    void *host = guest(address, 2);
    if (host) memcpy(&value, host, 2);
    return value;
}
static void put_word(uint32_t address, uint32_t value)
{
    void *host = guest(address, 4);
    if (host) memcpy(host, &value, 4);
}
static uint32_t record_address(int id)
{
    return id == 1 ? HERO_RECORD : PARTY_SLOTS + (uint32_t)(id - 2) * PARTY_SLOT_SIZE + 4u;
}

/* ---- seams */

static int active(void)
{
    return handler && handler->enabled && !handler->removed;
}
static int seam_callback(P4ModCall *call, void *user)
{
    if (user == &begin_hook) p4_mod_defeat_battle_begin((uint32_t)call->args[0].u);
    else p4_mod_defeat_field((uint32_t)call->args[0].u);
    return 0;
}
static int seams(void)
{
    if (begin_hook && field_hook) return 0;
    if (!begin_hook) {
        P4ModFunction *function = p4_mod_function_find(BATTLE_CREATE, "ii");
        begin_hook = function ? p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, P4_MOD_BEFORE, seam_callback, &begin_hook) : NULL;
        if (begin_hook) p4_mod_hook_enable(begin_hook, 0);
    }
    if (!field_hook) {
        P4ModFunction *function = p4_mod_function_find(FIELD_TO_BATTLE, "ip");
        field_hook = function ? p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, P4_MOD_BEFORE, seam_callback, &field_hook) : NULL;
        if (field_hook) p4_mod_hook_enable(field_hook, 0);
    }
    if (begin_hook && field_hook) return 0;
    fprintf(stderr, "p4pc: mods: battle defeat seams unavailable: %08x ii %s, %08x ip %s\n", BATTLE_CREATE,
            begin_hook ? "ok" : "missing", FIELD_TO_BATTLE, field_hook ? "ok" : "missing");
    return -1;
}
static void seams_update(void)
{
    int on = active();
    if (begin_hook) p4_mod_hook_enable(begin_hook, on);
    if (field_hook) p4_mod_hook_enable(field_hook, on);
    if (!on) {
        snapshot.valid = 0;
        pending.active = 0;
    }
}

/* ---- handler registry: one owner at a time, never last-writer-wins */

P4ModDefeatHandler *p4_mod_defeat_add(size_t owner, P4ModDefeatFn callback, void *user, const char **reason)
{
    const char *why = NULL;
    if (!callback || depth) why = "a defeat handler cannot be registered while one answers a defeat";
    else if (active() || (handler && !handler->removed)) why = "another mod already answers battle defeats";
    else if (seams()) why = "the battle defeat seams are unavailable in this build";
    if (why) {
        if (reason) *reason = why;
        return NULL;
    }
    P4ModDefeatHandler *h = calloc(1, sizeof *h);
    if (!h) {
        if (reason) *reason = "out of memory";
        return NULL;
    }
    *h = (P4ModDefeatHandler){owner, 1, 0, callback, user};
    /* A removed handler stays a tombstone: its owner may still hold it. */
    handler = h;
    seams_update();
    return h;
}
int p4_mod_defeat_enable(P4ModDefeatHandler *h, int enabled)
{
    if (!h || h != handler || h->removed || depth) return -1;
    h->enabled = !!enabled;
    seams_update();
    return 0;
}
int p4_mod_defeat_remove(P4ModDefeatHandler *h)
{
    if (!h || depth) return -1;
    h->removed = 1;
    h->enabled = 0;
    if (h == handler) seams_update();
    return 0;
}
void p4_mod_defeat_remove_owner(size_t owner)
{
    if (depth) {
        fprintf(stderr, "p4pc: mods: battle defeat teardown requires idle dispatch\n");
        fflush(stderr);
        _Exit(EXIT_FAILURE);
    }
    if (handler && handler->owner == owner) p4_mod_defeat_remove(handler);
}

/* ---- battle start */

void p4_mod_defeat_battle_begin(uint32_t setup)
{
    pending.active = 0;
    snapshot.valid = 0;
    if (!active() || !guest(setup, 0x3Cu)) return;
    for (int id = 1; id <= RECORDS; ++id) {
        void *host = guest(record_address(id), UNIT_SIZE);
        if (!host) return;
        memcpy(snapshot.records[id - 1], host, UNIT_SIZE);
    }
    for (int i = 0; i < GROUPS; ++i) {
        uint32_t group = word(setup + SETUP_ENEMIES + 4u * (uint32_t)i);
        void *host = group ? guest(group, GROUP_SIZE) : NULL;
        snapshot.groups[i] = host && half(group) == GENUS_EC ? group : 0;
        if (snapshot.groups[i]) memcpy(snapshot.group_bytes[i], host, GROUP_SIZE);
    }
    snapshot.members = 0;
    for (int i = 0; i < 4; ++i) {
        uint32_t group = word(setup + SETUP_PARTY + 4u * (uint32_t)i);
        if (!group || half(group) != GENUS_PC) continue;
        uint32_t unit = word(group + 4u);
        uint16_t id = half(unit + 2u);
        if (id < 1 || id > RECORDS || unit != record_address(id)) continue;
        const uint8_t *r = snapshot.records[id - 1];
        P4ModDefeatMember *m = &snapshot.party[snapshot.members++];
        m->id = id;
        memcpy(&m->hp, r + UNIT_HP, 2);
        memcpy(&m->sp, r + UNIT_SP, 2);
        memcpy(&m->ailments, r + UNIT_BAD, 4);
    }
    snapshot.setup = setup;
    snapshot.valid = 1;
}

/* ---- the lost field battle */

static int leader_dead(void)
{
    uint32_t group = word(FIELD_PARTY);
    if (!group || half(group) != GENUS_PC) return 0;
    uint32_t unit = word(group + 4u);
    if (!guest(unit, UNIT_SIZE)) return 0;
    return (word(unit + UNIT_BAD) & UNIT_KO) != 0 || half(unit + UNIT_HP) == 0;
}
static void restore(void)
{
    for (int id = 1; id <= RECORDS; ++id) {
        void *host = guest(record_address(id), UNIT_SIZE);
        if (host) memcpy(host, snapshot.records[id - 1], UNIT_SIZE);
    }
    for (int i = 0; i < GROUPS; ++i) {
        void *host = snapshot.groups[i] ? guest(snapshot.groups[i], GROUP_SIZE) : NULL;
        if (host) memcpy(host, snapshot.group_bytes[i], GROUP_SIZE);
    }
}
static int game_over(uint32_t work, const char *why)
{
    fprintf(stderr, "p4pc: mods: battle defeat: encounter %u game over (%s)\n", (unsigned)pending.encounter, why);
    pending.active = 0;
    put_word(work + WORK_STATE, STATE_RESULT);
    return P4_MOD_DEFEAT_GAME_OVER;
}

int p4_mod_defeat_field(uint32_t task)
{
    if (!active()) return -1;
    uint32_t work = word(task + TASK_WORK);
    if (!guest(work, WORK_RETURN_MODE + 4u)) return -1;
    uint32_t state = word(work + WORK_STATE);
    if (pending.active && pending.work == work && state == STATE_BATTLE) {
        /* held: the poll after a hold */
    } else if (state == STATE_RESULT && leader_dead()) {
        uint32_t setup = work + WORK_SETUP;
        uint32_t group = word(setup + SETUP_ENEMIES);
        pending.active = 1;
        pending.work = work;
        pending.encounter = group ? half(group + 8u) : 0xFFFFu;
        pending.polls = 0;
        pending.source = word(work + WORK_RETURN_MODE) == 1 ? P4_MOD_DEFEAT_FIELD : P4_MOD_DEFEAT_COLLISION;
        pending.allowed = 1;
        pending.reason[0] = 0;
        if (!snapshot.valid || snapshot.setup != setup) {
            pending.allowed = 0;
            snprintf(pending.reason, sizeof pending.reason, "the battle start was not recorded");
        } else if (p4_mod_authorize_mask & P4_MOD_AUTH_RETRY_BIT) {
            P4ModEvent context = {"retry", 2, {{"encounter", pending.encounter}, {"source", pending.source}}};
            pending.allowed = p4_mod_authorize(P4_MOD_AUTH_RETRY, &context, pending.reason, sizeof pending.reason);
            if (!pending.allowed && !pending.reason[0]) snprintf(pending.reason, sizeof pending.reason, "a retry authorizer refused it");
        }
        fprintf(stderr, "p4pc: mods: battle defeat: encounter %u (%s) answered by owner %zu, retry %s%s%s\n",
                (unsigned)pending.encounter, pending.source == P4_MOD_DEFEAT_FIELD ? "field" : "collision", handler->owner,
                pending.allowed ? "allowed" : "refused", pending.allowed ? "" : ": ", pending.reason);
    } else {
        if (pending.active && pending.work == work) pending.active = 0;
        return -1;
    }

    P4ModDefeat defeat = {pending.encounter, pending.source, pending.polls, pending.allowed, pending.reason, 0, {{0}}};
    if (snapshot.valid && snapshot.setup == work + WORK_SETUP) {
        defeat.members = snapshot.members;
        memcpy(defeat.party, snapshot.party, sizeof defeat.party);
    }
    P4ModDefeatHandler *h = handler;
    ++depth;
    int answer = h->callback(&defeat, h->user);
    --depth;
    if (answer < 0) p4_mod_failure(h->owner, "Native battle defeat handler failed");
    if (answer == P4_MOD_DEFEAT_HOLD) {
        if (++pending.polls >= P4_MOD_DEFEAT_HOLD_LIMIT) return game_over(work, "held too long");
        put_word(work + WORK_STATE, STATE_HOLD);
        return P4_MOD_DEFEAT_HOLD;
    }
    if (answer == P4_MOD_DEFEAT_RETRY) {
        if (!pending.allowed) {
            char why[160];
            snprintf(why, sizeof why, "retry refused: %s", pending.reason);
            return game_over(work, why);
        }
        restore();
        fprintf(stderr, "p4pc: mods: battle defeat: encounter %u retried from its battle start (%d records, %d enemy groups)\n",
                (unsigned)pending.encounter, RECORDS, (snapshot.groups[0] != 0) + (snapshot.groups[1] != 0) + (snapshot.groups[2] != 0));
        pending.active = 0;
        put_word(work + WORK_STATE, STATE_BATTLE);
        return P4_MOD_DEFEAT_RETRY;
    }
    return game_over(work, "the handler let it run");
}
