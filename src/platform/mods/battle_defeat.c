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
 *   Script battles (btlScriptStartEncounter 0x001932f0, a story boss started by
 *   a field script: sequence 6, Start callback 0x00102e70) never reach state 12.
 *   The script that started the battle polls btlScriptResolvePartyDeath
 *   (0x00193450) each frame: while the battle context (gp-0x4c54, 0x0076449c)
 *   exists it answers "not yet"; once it is gone it starts the Game Over
 *   procedure when the protagonist's record is dead, else revives the fallen
 *   allies. That call is the seam: the same handler answers just before it
 *   reads the dead protagonist:
 *     game over  the call runs untouched, so the script starts the Game Over;
 *     hold       the battle context is shown as present for that one call, so
 *                the script keeps waiting;
 *     retry      the snapshot is copied back, fresh party and enemy groups are
 *                built from the recorded setup exactly as 001932f0 builds them,
 *                btlTaskCreate (0x00192e90) starts the battle again, so the
 *                script never saw the loss.
 *   The enemy groups of a script battle are freed with the battle, so they are
 *   re-created, never copied back.
 *
 * The persistent inventory, Persona stock and selection, party slots, flags
 * and money are also restored, and so are the two native sidecars that hold
 * battle-relevant state outside guest memory: the expanded item table's
 * quantities, availability and equipment references, and the enemy-knowledge
 * observations (known flags and the retail status/side bits). Both are
 * captured with the battle start, compared before a retry is allowed and copied
 * back exactly; a sidecar that cannot be captured or is no longer the captured
 * one refuses the retry with its own reason. In particular, retry refunds consumed items:
 * the loss is treated as though the battle never happened. */
#include "p4pc/mod_defeat.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_failure.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mod_memory.h"
#include "p4pc/game_items.h"
#include "p4pc/game_items_native.h"
#include "game_knowledge_runtime_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void *datUnitCreatePc(uint16_t id);
extern void *datUnitCreateEncounterGroup(uint16_t encounter);
extern void hFadeTaskStart(int16_t mode);
extern int32_t btlTaskCreate(int32_t setup);
extern void btlSetFreeUnitsOnTeardown(void);

#define BATTLE_CREATE 0x00192e90u   /* btlTaskCreate(setup) */
#define SEQ_BATTLE_START 0x00102e70u   /* the BATTLE sequence's Start callback (sequence 6) */
#define SCRIPT_RESOLVE 0x00193450u     /* btlScriptResolvePartyDeath, the script's post-battle poll */
#define BATTLE_CONTEXT 0x0076449Cu     /* gp-0x4c54: the battle context pointer, 0 once released */
#define SETUP_SIZE 0x3Cu
#define SETUP_FORMATION 0x20u       /* u16 formation, u16 variant */
#define SCRIPT_START_POLLS 600u     /* polls a retried battle may take to appear */
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
#define INVENTORY 0x00797574u /* protagonist equipment, then item counts */
#define INVENTORY_SIZE 0x606u
#define PERSONA_STOCK 0x00797F88u
#define PERSONA_STOCK_SIZE 0x244u
#define COMMUNITY 0x0079740Cu
#define COMMUNITY_SIZE 0x158u
#define GAME_FLAGS 0x0079B1CCu
#define GAME_FLAGS_SIZE 0x2C0u
#define SCRIPT_VALUES 0x0079B48Cu
#define SCRIPT_VALUES_SIZE 0x200u
#define KNOWLEDGE 0x0079BCD8u
#define KNOWLEDGE_SIZE 0x154u
#define MONEY 0x0079B68Cu
#define PARTY_SIZE (9u * PARTY_SLOT_SIZE)
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
static P4ModHook *begin_hook, *field_hook, *start_before_hook, *start_after_hook, *resolve_before_hook, *resolve_after_hook;
static unsigned depth;
static unsigned script_start; /* inside the BATTLE sequence's Start callback */
static int restarting;        /* the retry's own btlTaskCreate */
static uint32_t shown_context; /* a placeholder battle context shown to one resolve call */

static struct {
    int valid;
    char unsupported[128]; /* a sidecar that could not be captured, else empty */
    P4GameItemsCapture *items_store;
    P4GameKnowledgeCapture *knowledge_store;
    uint32_t setup;
    int script;                   /* started by a script battle's sequence 6 */
    char script_unsupported[96];  /* a script battle that cannot be rebuilt, else empty */
    uint32_t encounter;
    uint16_t party_ids[4];
    int party_count;
    uint8_t setup_bytes[SETUP_SIZE];
    uint8_t records[RECORDS][UNIT_SIZE];
    uint8_t hero_tail[0x18];
    uint8_t party_slots[PARTY_SIZE];
    uint8_t inventory[INVENTORY_SIZE];
    uint8_t personas[PERSONA_STOCK_SIZE];
    uint8_t community[COMMUNITY_SIZE];
    uint8_t flags[GAME_FLAGS_SIZE];
    uint8_t script_values[SCRIPT_VALUES_SIZE];
    uint8_t knowledge[KNOWLEDGE_SIZE];
    uint8_t money[4];
    uint32_t groups[GROUPS];
    uint8_t group_bytes[GROUPS][GROUP_SIZE];
    int members;
    P4ModDefeatMember party[4];
} snapshot;

/* A script battle's life: running once its battle task exists, then the
 * defeat question; a retried battle is awaited for SCRIPT_START_POLLS polls. */
static struct {
    int running, restart_polls;
} script_battle;
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
    else if (user == &start_before_hook) ++script_start;
    else if (user == &start_after_hook) { if (script_start) --script_start; }
    else if (user == &resolve_before_hook) p4_mod_defeat_script();
    else if (user == &resolve_after_hook) {
        if (shown_context) put_word(BATTLE_CONTEXT, 0);
        shown_context = 0;
    }
    else p4_mod_defeat_field((uint32_t)call->args[0].u);
    return 0;
}
static P4ModHook *seam_add(uint32_t address, const char *signature, int phase, void *user)
{
    P4ModFunction *function = p4_mod_function_find(address, signature);
    P4ModHook *hook = function ? p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, phase, seam_callback, user) : NULL;
    if (hook) p4_mod_hook_enable(hook, 0);
    return hook;
}
static int seams(void)
{
    if (!begin_hook) begin_hook = seam_add(BATTLE_CREATE, "ii", P4_MOD_BEFORE, &begin_hook);
    if (!field_hook) field_hook = seam_add(FIELD_TO_BATTLE, "ip", P4_MOD_BEFORE, &field_hook);
    if (!start_before_hook) start_before_hook = seam_add(SEQ_BATTLE_START, "vi", P4_MOD_BEFORE, &start_before_hook);
    if (!start_after_hook) start_after_hook = seam_add(SEQ_BATTLE_START, "vi", P4_MOD_AFTER, &start_after_hook);
    if (!resolve_before_hook) resolve_before_hook = seam_add(SCRIPT_RESOLVE, "i", P4_MOD_BEFORE, &resolve_before_hook);
    if (!resolve_after_hook) resolve_after_hook = seam_add(SCRIPT_RESOLVE, "i", P4_MOD_AFTER, &resolve_after_hook);
    if (begin_hook && field_hook && start_before_hook && start_after_hook && resolve_before_hook && resolve_after_hook)
        return 0;
    fprintf(stderr, "p4pc: mods: battle defeat seams unavailable: %08x ii %s, %08x ip %s, %08x vi %s, %08x i %s\n",
            BATTLE_CREATE, begin_hook ? "ok" : "missing", FIELD_TO_BATTLE, field_hook ? "ok" : "missing",
            SEQ_BATTLE_START, start_before_hook && start_after_hook ? "ok" : "missing",
            SCRIPT_RESOLVE, resolve_before_hook && resolve_after_hook ? "ok" : "missing");
    return -1;
}
static void sidecars_free(void)
{
    p4_game_tables_items_capture_free(snapshot.items_store);
    p4_game_knowledge_capture_free(snapshot.knowledge_store);
    snapshot.items_store = NULL;
    snapshot.knowledge_store = NULL;
}
static void seams_update(void)
{
    int on = active();
    if (begin_hook) p4_mod_hook_enable(begin_hook, on);
    if (field_hook) p4_mod_hook_enable(field_hook, on);
    if (start_before_hook) p4_mod_hook_enable(start_before_hook, on);
    if (start_after_hook) p4_mod_hook_enable(start_after_hook, on);
    if (resolve_before_hook) p4_mod_hook_enable(resolve_before_hook, on);
    if (resolve_after_hook) p4_mod_hook_enable(resolve_after_hook, on);
    if (!on) {
        snapshot.valid = 0;
        sidecars_free();
        pending.active = 0;
        script_battle.running = script_battle.restart_polls = 0;
        script_start = 0;
        if (shown_context) put_word(BATTLE_CONTEXT, 0);
        shown_context = 0;
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
    sidecars_free();
    snapshot.unsupported[0] = 0;
    int result = p4_game_tables_items_battle_capture(&snapshot.items_store);
    if (result) snprintf(snapshot.unsupported, sizeof snapshot.unsupported,
                         "the expanded item state could not be captured (%s)", p4_game_tables_error(result));
    else if ((result = p4_game_knowledge_battle_capture(&snapshot.knowledge_store)))
        snprintf(snapshot.unsupported, sizeof snapshot.unsupported,
                 "the enemy knowledge state could not be captured (%s)", p4_game_tables_error(result));
    if (!snapshot.unsupported[0] && (snapshot.items_store || snapshot.knowledge_store))
        fprintf(stderr, "p4pc: mods: battle defeat: battle start sidecar state, items %016llx, knowledge %016llx\n",
                (unsigned long long)p4_game_tables_items_capture_digest(snapshot.items_store),
                (unsigned long long)p4_game_knowledge_capture_digest(snapshot.knowledge_store));
    void *party = guest(PARTY_SLOTS, PARTY_SIZE);
    void *inventory = guest(INVENTORY, INVENTORY_SIZE);
    void *personas = guest(PERSONA_STOCK, PERSONA_STOCK_SIZE);
    void *community = guest(COMMUNITY, COMMUNITY_SIZE);
    void *flags = guest(GAME_FLAGS, GAME_FLAGS_SIZE);
    void *script_values = guest(SCRIPT_VALUES, SCRIPT_VALUES_SIZE);
    void *knowledge = guest(KNOWLEDGE, KNOWLEDGE_SIZE);
    void *money = guest(MONEY, 4);
    if (!party || !inventory || !personas || !community || !flags || !script_values || !knowledge || !money) return;
    memcpy(snapshot.party_slots, party, PARTY_SIZE);
    memcpy(snapshot.inventory, inventory, INVENTORY_SIZE);
    memcpy(snapshot.personas, personas, PERSONA_STOCK_SIZE);
    memcpy(snapshot.community, community, COMMUNITY_SIZE);
    memcpy(snapshot.flags, flags, GAME_FLAGS_SIZE);
    memcpy(snapshot.script_values, script_values, SCRIPT_VALUES_SIZE);
    memcpy(snapshot.knowledge, knowledge, KNOWLEDGE_SIZE);
    memcpy(snapshot.money, money, 4);
    void *hero_tail = guest(HERO_RECORD + UNIT_SIZE, sizeof snapshot.hero_tail);
    if (!hero_tail) return;
    memcpy(snapshot.hero_tail, hero_tail, sizeof snapshot.hero_tail);
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
    snapshot.script = script_start != 0 || restarting;
    snapshot.script_unsupported[0] = 0;
    snapshot.encounter = snapshot.groups[0] ? half(snapshot.groups[0] + 8u) : 0xFFFFu;
    snapshot.party_count = 0;
    memcpy(snapshot.setup_bytes, guest(setup, SETUP_SIZE), SETUP_SIZE);
    for (int i = 0; i < 4; ++i) {
        uint32_t group = word(setup + SETUP_PARTY + 4u * (uint32_t)i);
        if (!group) continue;
        uint32_t unit = group && half(group) == GENUS_PC ? word(group + 4u) : 0;
        uint16_t id = unit ? half(unit + 2u) : 0;
        if (id < 1 || id > RECORDS || unit != record_address(id))
            snprintf(snapshot.script_unsupported, sizeof snapshot.script_unsupported,
                     "a party member of the battle is not a playable record");
        else snapshot.party_ids[snapshot.party_count++] = id;
    }
    if (!snapshot.groups[0] || snapshot.groups[1] || snapshot.groups[2])
        snprintf(snapshot.script_unsupported, sizeof snapshot.script_unsupported,
                 "the battle's enemy groups are not one retail encounter group");
    snapshot.setup = setup;
    snapshot.valid = 1;
    if (snapshot.script) script_battle.running = 0;
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
/* The protagonist's record: a script battle's loss is a fallen protagonist,
 * which the script reads back through the record, not a field group. */
static int hero_dead(void)
{
    if (!guest(HERO_RECORD, UNIT_SIZE)) return 0;
    return (word(HERO_RECORD + UNIT_BAD) & UNIT_KO) != 0 || half(HERO_RECORD + UNIT_HP) == 0;
}
/* enemy_groups: a field battle's groups belong to the field and are copied
 * back; a script battle's are freed with it and re-created by the caller. */
static int restore(int enemy_groups)
{
    /* Both sidecars were matched when the retry was allowed and nothing runs in
     * between, so these cannot fail; if one does, nothing else is copied. */
    if (p4_game_tables_items_battle_matches(snapshot.items_store) || p4_game_knowledge_battle_matches(snapshot.knowledge_store) ||
        p4_game_tables_items_battle_restore(snapshot.items_store))
        return -1;
    if (p4_game_knowledge_battle_restore(snapshot.knowledge_store)) return -1;
    memcpy(guest(PARTY_SLOTS, PARTY_SIZE), snapshot.party_slots, PARTY_SIZE);
    memcpy(guest(INVENTORY, INVENTORY_SIZE), snapshot.inventory, INVENTORY_SIZE);
    memcpy(guest(PERSONA_STOCK, PERSONA_STOCK_SIZE), snapshot.personas, PERSONA_STOCK_SIZE);
    memcpy(guest(COMMUNITY, COMMUNITY_SIZE), snapshot.community, COMMUNITY_SIZE);
    memcpy(guest(GAME_FLAGS, GAME_FLAGS_SIZE), snapshot.flags, GAME_FLAGS_SIZE);
    memcpy(guest(SCRIPT_VALUES, SCRIPT_VALUES_SIZE), snapshot.script_values, SCRIPT_VALUES_SIZE);
    memcpy(guest(KNOWLEDGE, KNOWLEDGE_SIZE), snapshot.knowledge, KNOWLEDGE_SIZE);
    memcpy(guest(MONEY, 4), snapshot.money, 4);
    memcpy(guest(HERO_RECORD + UNIT_SIZE, sizeof snapshot.hero_tail), snapshot.hero_tail, sizeof snapshot.hero_tail);
    for (int id = 1; id <= RECORDS; ++id) {
        void *host = guest(record_address(id), UNIT_SIZE);
        if (host) memcpy(host, snapshot.records[id - 1], UNIT_SIZE);
    }
    for (int i = 0; enemy_groups && i < GROUPS; ++i) {
        void *host = snapshot.groups[i] ? guest(snapshot.groups[i], GROUP_SIZE) : NULL;
        if (host) memcpy(host, snapshot.group_bytes[i], GROUP_SIZE);
    }
    return 0;
}
static const char *source_name(int source)
{
    return source == P4_MOD_DEFEAT_FIELD ? "field" : source == P4_MOD_DEFEAT_SCRIPT ? "script" : "collision";
}
static void game_over_note(const char *why)
{
    fprintf(stderr, "p4pc: mods: battle defeat: encounter %u game over (%s)\n", (unsigned)pending.encounter, why);
    pending.active = 0;
}
static int game_over(uint32_t work, const char *why)
{
    game_over_note(why);
    put_word(work + WORK_STATE, STATE_RESULT);
    return P4_MOD_DEFEAT_GAME_OVER;
}
/* The first question of one defeat: whether a retry is possible at all (a
 * recorded battle start, capturable and unchanged sidecars, the authorizers),
 * logged once. */
static void defeat_begin(uint32_t key, uint32_t setup, uint32_t encounter, int source)
{
    pending.active = 1;
    pending.work = key;
    pending.encounter = encounter;
    pending.polls = 0;
    pending.source = source;
    pending.allowed = 1;
    pending.reason[0] = 0;
    if (!snapshot.valid || snapshot.setup != setup) {
        pending.allowed = 0;
        snprintf(pending.reason, sizeof pending.reason, "the battle start was not recorded");
    } else if (source == P4_MOD_DEFEAT_SCRIPT && snapshot.script_unsupported[0]) {
        pending.allowed = 0;
        snprintf(pending.reason, sizeof pending.reason, "%s", snapshot.script_unsupported);
    } else if (snapshot.unsupported[0]) {
        pending.allowed = 0;
        snprintf(pending.reason, sizeof pending.reason, "%s", snapshot.unsupported);
    } else if (p4_game_tables_items_battle_matches(snapshot.items_store)) {
        pending.allowed = 0;
        snprintf(pending.reason, sizeof pending.reason,
                 "the expanded item table is no longer the one captured at the battle start");
    } else if (p4_game_knowledge_battle_matches(snapshot.knowledge_store)) {
        pending.allowed = 0;
        snprintf(pending.reason, sizeof pending.reason,
                 "the enemy knowledge store is no longer the one captured at the battle start");
    } else if (p4_mod_authorize_mask & P4_MOD_AUTH_RETRY_BIT) {
        P4ModEvent context = {"retry", 2, {{"encounter", pending.encounter}, {"source", pending.source}}};
        pending.allowed = p4_mod_authorize(P4_MOD_AUTH_RETRY, &context, pending.reason, sizeof pending.reason);
        if (!pending.allowed && !pending.reason[0]) snprintf(pending.reason, sizeof pending.reason, "a retry authorizer refused it");
    }
    fprintf(stderr, "p4pc: mods: battle defeat: encounter %u (%s) answered by owner %zu, retry %s%s%s\n",
            (unsigned)pending.encounter, source_name(pending.source), handler->owner,
            pending.allowed ? "allowed" : "refused", pending.allowed ? "" : ": ", pending.reason);
}
/* Asks the handler: its answer, game over for a failure or an unknown word. */
static int defeat_ask(uint32_t setup)
{
    P4ModDefeat defeat = {pending.encounter, pending.source, pending.polls, pending.allowed, pending.reason, 0, {{0}}};
    if (snapshot.valid && snapshot.setup == setup) {
        defeat.members = snapshot.members;
        memcpy(defeat.party, snapshot.party, sizeof defeat.party);
    }
    P4ModDefeatHandler *h = handler;
    ++depth;
    int answer = h->callback(&defeat, h->user);
    --depth;
    if (answer < 0) p4_mod_failure(h->owner, "Native battle defeat handler failed");
    return answer;
}
static void restored_note(void)
{
    fprintf(stderr, "p4pc: mods: battle defeat: encounter %u retried from its battle start (%d records, %d enemy groups)\n",
            (unsigned)pending.encounter, RECORDS, (snapshot.groups[0] != 0) + (snapshot.groups[1] != 0) + (snapshot.groups[2] != 0));
    if (snapshot.items_store || snapshot.knowledge_store)
        fprintf(stderr, "p4pc: mods: battle defeat: sidecar state restored, items %016llx, knowledge %016llx\n",
                (unsigned long long)p4_game_tables_items_battle_digest(),
                (unsigned long long)p4_game_knowledge_battle_digest());
}

int p4_mod_defeat_field(uint32_t task)
{
    if (!active()) return -1;
    uint32_t work = word(task + TASK_WORK);
    if (!guest(work, WORK_RETURN_MODE + 4u)) return -1;
    uint32_t state = word(work + WORK_STATE);
    uint32_t setup = work + WORK_SETUP;
    if (pending.active && pending.work == work && state == STATE_BATTLE) {
        /* held: the poll after a hold */
    } else if (state == STATE_RESULT && leader_dead()) {
        uint32_t group = word(setup + SETUP_ENEMIES);
        defeat_begin(work, setup, group ? half(group + 8u) : 0xFFFFu,
                     word(work + WORK_RETURN_MODE) == 1 ? P4_MOD_DEFEAT_FIELD : P4_MOD_DEFEAT_COLLISION);
    } else {
        if (pending.active && pending.work == work) pending.active = 0;
        return -1;
    }

    int answer = defeat_ask(setup);
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
        if (restore(1)) return game_over(work, "retry refused: a sidecar state could not be restored");
        restored_note();
        pending.active = 0;
        put_word(work + WORK_STATE, STATE_BATTLE);
        return P4_MOD_DEFEAT_RETRY;
    }
    return game_over(work, "the handler let it run");
}

/* ---- the lost script battle */

/* The retail 001932f0 body, from the recorded setup: fresh groups for the
 * same party and encounter, the same formation, then the battle itself. */
static int script_restart(void)
{
    /* One port-owned packet for every retry; mods may read it (the battle_start
     * event hands them its address) but never write it. */
    static uint32_t packet;
    if (!packet) {
        packet = p4_gport_alloc(SETUP_SIZE, 16);
        if (packet) p4_mod_memory_declare(packet, SETUP_SIZE, P4_MOD_MEMORY_READ);
    }
    uint8_t *bytes = packet ? guest(packet, SETUP_SIZE) : NULL;
    if (!bytes) return -1;
    memcpy(bytes, snapshot.setup_bytes, SETUP_SIZE);
    memset(bytes + SETUP_PARTY, 0, 16);
    memset(bytes + SETUP_ENEMIES, 0, 12);
    hFadeTaskStart(1);
    for (int i = 0; i < snapshot.party_count; ++i) {
        uint32_t group = P4_H2G(datUnitCreatePc(snapshot.party_ids[i]));
        memcpy(bytes + SETUP_PARTY + 4 * i, &group, 4);
    }
    uint32_t enemies = P4_H2G(datUnitCreateEncounterGroup((uint16_t)snapshot.encounter));
    memcpy(bytes + SETUP_ENEMIES, &enemies, 4);
    restarting = 1;
    btlTaskCreate((int32_t)packet);
    btlSetFreeUnitsOnTeardown();
    restarting = 0;
    return snapshot.valid && snapshot.script ? 0 : -1;
}

/* Shows the script a battle that is still there for this one poll: the
 * resolve call then answers "not yet" and the script keeps waiting. */
static void show_running(void)
{
    if (!shown_context) shown_context = p4_gport_alloc(16, 16);
    if (shown_context) put_word(BATTLE_CONTEXT, shown_context);
}

void p4_mod_defeat_script(void)
{
    if (!active() || !snapshot.valid || !snapshot.script) return;
    if (word(BATTLE_CONTEXT) != 0) {
        /* A real battle exists. */
        script_battle.running = 1;
        script_battle.restart_polls = 0;
        return;
    }
    if (!script_battle.running) {
        /* Not started yet (the sequence's delay frames), or a retried battle
         * that has not published its context: the wait goes on for the latter. */
        if (script_battle.restart_polls > 0 && script_battle.restart_polls++ < (int)SCRIPT_START_POLLS) show_running();
        return;
    }
    script_battle.running = 0;
    if (!hero_dead()) {
        snapshot.valid = 0; /* won, or no loss: the script goes on */
        pending.active = 0;
        return;
    }
    if (!(pending.active && pending.work == snapshot.setup))
        defeat_begin(snapshot.setup, snapshot.setup, snapshot.encounter, P4_MOD_DEFEAT_SCRIPT);
    int answer = defeat_ask(snapshot.setup);
    if (answer == P4_MOD_DEFEAT_HOLD) {
        if (++pending.polls >= P4_MOD_DEFEAT_HOLD_LIMIT) {
            game_over_note("held too long");
            snapshot.valid = 0;
            return;
        }
        script_battle.running = 1; /* still asking: the next poll is the same defeat */
        show_running();
        return;
    }
    if (answer == P4_MOD_DEFEAT_RETRY) {
        if (!pending.allowed) {
            char why[160];
            snprintf(why, sizeof why, "retry refused: %s", pending.reason);
            game_over_note(why);
            snapshot.valid = 0;
            return;
        }
        if (restore(0)) {
            game_over_note("retry refused: a sidecar state could not be restored");
            snapshot.valid = 0;
            return;
        }
        restored_note();
        pending.active = 0;
        if (script_restart()) {
            game_over_note("retry refused: the battle could not be started again");
            snapshot.valid = 0;
            return;
        }
        script_battle.running = 0;
        script_battle.restart_polls = 1;
        if (word(BATTLE_CONTEXT) == 0) show_running();
        return;
    }
    game_over_note("the handler let it run");
    snapshot.valid = 0;
}
