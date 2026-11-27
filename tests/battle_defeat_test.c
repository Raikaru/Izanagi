/* p4.battle.defeat's native half (src/platform/mods/battle_defeat.c) over the
 * real hook dispatch and the real "retry" authorizers (authorize.c), with
 * hand-built descriptors for btlTaskCreate (0x00192e90) and
 * FldEvent_FieldToBattle (0x00172e00) and the playable characters' DatUnit
 * records at their retail addresses.
 *
 * The FieldToBattle body here is the subset of the retail states the seam
 * relies on (k_fldEvent.c, FUN_00172E00): 6..9 only advance the state, 10
 * calls btlTaskCreate with the work's setup at +0x50, 11 waits for the battle
 * task, and 12 starts Game Over (state 0x63) when the field leader's group
 * (D_007EF9F8) is defeated, else continues. A test frame is one call, as the
 * field task or the field script's encounter request polls it. */
#include "guest_test.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_defeat.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mod_memory.h"
#include "p4pc/os.h"
#include "p4pc/table_result.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "battle_defeat: FAIL line %d: %s\n", __LINE__, #x); failures++; } } while (0)
static int failures;
/* The two native sidecars, as fakes with the real call contract
 * (game_items.h, game_knowledge_runtime_internal.h): capture stores *out = NULL
 * when none is active, matches and restore refuse a store that is not the
 * captured one, and nothing is touched by a refused restore. */
typedef struct FakeSidecar {
    int active, fail_capture;
    unsigned generation, captures, restores, frees;
    uint32_t data[4];
} FakeSidecar;
static FakeSidecar fake_items, fake_knowledge;
struct P4GameItemsCapture { uint32_t data[4]; unsigned generation; };
struct P4GameKnowledgeCapture { uint32_t data[4]; unsigned generation; };
const char *p4_game_tables_error(int result) { return result == P4_TABLE_NOMEM ? "out of memory" : "error"; }
static int fake_capture(FakeSidecar *side, void **out)
{
    *out = NULL;
    if (!side->active) return P4_TABLE_OK;
    if (side->fail_capture) return P4_TABLE_NOMEM;
    struct P4GameItemsCapture *capture = calloc(1, sizeof *capture);
    memcpy(capture->data, side->data, sizeof capture->data);
    capture->generation = side->generation;
    ++side->captures;
    *out = capture;
    return P4_TABLE_OK;
}
static int fake_matches(const FakeSidecar *side, const struct P4GameItemsCapture *capture)
{
    if (!side->active) return capture ? P4_TABLE_CONFLICT : P4_TABLE_OK;
    return capture && capture->generation == side->generation ? P4_TABLE_OK : P4_TABLE_CONFLICT;
}
static int fake_restore(FakeSidecar *side, const struct P4GameItemsCapture *capture)
{
    int result = fake_matches(side, capture);
    if (result || !side->active) return result;
    memcpy(side->data, capture->data, sizeof side->data);
    ++side->restores;
    return P4_TABLE_OK;
}
int p4_game_tables_items_battle_capture(struct P4GameItemsCapture **out) { return fake_capture(&fake_items, (void **)out); }
int p4_game_tables_items_battle_matches(const struct P4GameItemsCapture *c) { return fake_matches(&fake_items, c); }
int p4_game_tables_items_battle_restore(const struct P4GameItemsCapture *c) { return fake_restore(&fake_items, c); }
void p4_game_tables_items_capture_free(struct P4GameItemsCapture *c) { if (c) ++fake_items.frees; free(c); }
static uint64_t fake_digest(const uint32_t *data) { return ((uint64_t)data[0] << 32) ^ data[1] ^ ((uint64_t)data[3] << 8); }
uint64_t p4_game_tables_items_battle_digest(void) { return fake_items.active ? fake_digest(fake_items.data) : 0; }
uint64_t p4_game_tables_items_capture_digest(const struct P4GameItemsCapture *c) { return c ? fake_digest(c->data) : 0; }
uint64_t p4_game_knowledge_battle_digest(void) { return fake_knowledge.active ? fake_digest(fake_knowledge.data) : 0; }
uint64_t p4_game_knowledge_capture_digest(const struct P4GameKnowledgeCapture *c) { return c ? fake_digest(c->data) : 0; }
int p4_game_knowledge_battle_capture(struct P4GameKnowledgeCapture **out) { return fake_capture(&fake_knowledge, (void **)out); }
int p4_game_knowledge_battle_matches(const struct P4GameKnowledgeCapture *c)
{ return fake_matches(&fake_knowledge, (const struct P4GameItemsCapture *)c); }
int p4_game_knowledge_battle_restore(const struct P4GameKnowledgeCapture *c)
{ return fake_restore(&fake_knowledge, (const struct P4GameItemsCapture *)c); }
void p4_game_knowledge_capture_free(struct P4GameKnowledgeCapture *c) { if (c) ++fake_knowledge.frees; free(c); }

enum { START, FIELD, CREATE, RESOLVE, COUNT };
static void field_to_battle(P4ModValue *args, P4ModValue *result);
static void battle_create(P4ModValue *args, P4ModValue *result);
static void sequence_start(P4ModValue *args, P4ModValue *result);
static void script_resolve(P4ModValue *args, P4ModValue *result);
static P4ModEntry entries[COUNT];
/* Ascending addresses, as the generated table is (p4_mod_function_find bisects). */
static P4ModFunction functions[COUNT] = {
    {0x00102e70u, &entries[START], "vi", sequence_start, NULL, NULL},     /* the BATTLE sequence's Start */
    {0x00172e00u, &entries[FIELD], "ip", field_to_battle, NULL, NULL},  /* FldEvent_FieldToBattle */
    {0x00192e90u, &entries[CREATE], "ii", battle_create, NULL, NULL},   /* btlTaskCreate */
    {0x00193450u, &entries[RESOLVE], "i", script_resolve, NULL, NULL},  /* btlScriptResolvePartyDeath */
};
P4ModFunction *const p4_mod_function_table[COUNT] = {&functions[START], &functions[FIELD], &functions[CREATE],
                                                     &functions[RESOLVE]};
const size_t p4_mod_function_count = COUNT;

static int dispatched(P4ModFunction *function, P4ModValue *args, P4ModValue *result)
{
    P4ModEntry *entry = function->entry;
    if (!entry->enabled) return 0;
    if (entry->enabled == P4_MOD_BYPASS) {
        entry->enabled = entry->restore;
        return 0;
    }
    p4_mod_dispatch(function, args, result);
    return 1;
}

/* A script battle (btlScriptStartEncounter, sequence 6): the Start callback
 * creates the battle, btlScriptResolvePartyDeath polls it, and the retry rebuilds
 * the groups through the retail constructors, stubbed here. */
static uint32_t script_setup, group_pool, pool_used;
#define BATTLE_CTX 0x0076449Cu
static unsigned script_game_overs, script_revives, script_starts, pcs_created, enemy_groups_created, fades;
static int hero_is_dead(void);
static uint16_t created_pc_ids[8], created_encounter;
static uint32_t last_pc_group, last_enemy_group;
static void *new_group(uint16_t genus)
{
    uint32_t group = group_pool + 0x130u * pool_used++;
    P4_G16(group) = genus;
    return P4_G2H(group);
}
void *datUnitCreatePc(uint16_t id)
{
    created_pc_ids[pcs_created++ & 7u] = id;
    uint8_t *host = new_group(0);
    last_pc_group = P4_H2G(host);
    P4_G32(last_pc_group + 4u) = id == 1 ? 0x007973C4u : 0x00796E50u + (uint32_t)(id - 2) * 0x88u + 4u;
    return host;
}
void *datUnitCreateEncounterGroup(uint16_t encounter)
{
    ++enemy_groups_created;
    created_encounter = encounter;
    uint8_t *host = new_group(1);
    last_enemy_group = P4_H2G(host);
    P4_G16(last_enemy_group + 8u) = encounter;
    return host;
}
void hFadeTaskStart(int16_t mode) { (void)mode; ++fades; }
static unsigned declared_reads;
P4ModMemoryRegion *p4_mod_memory_declare(uint32_t address, size_t size, unsigned flags)
{
    CHECK(size == 0x3C && flags == P4_MOD_MEMORY_READ);
    ++declared_reads;
    return (P4ModMemoryRegion *)(uintptr_t)address;
}
void btlSetFreeUnitsOnTeardown(void) {}
static void battle_create(P4ModValue *args, P4ModValue *result);
int32_t btlTaskCreate(int32_t setup)
{
    P4ModValue args[1] = {{0}}, result = {0};
    args[0].u = (uint32_t)setup;
    battle_create(args, &result);
    return (int32_t)result.i;
}

#define HERO 0x007973C4u
#define YOSUKE (0x00796E50u + 4u)
#define CHIE (0x00796E50u + 0x88u + 4u)
#define ITEMS 0x0079757Au
#define PERSONAS 0x00797F88u
#define COMMUNITY 0x0079740Cu
#define FLAGS 0x0079B1CCu
#define SCRIPT_VALUES 0x0079B48Cu
#define KNOWLEDGE 0x0079BCD8u
#define MONEY 0x0079B68Cu
#define FIELD_PARTY 0x007EF9F8u
static uint32_t task, work, leader, yosuke_group, enemies, other_setup;
static unsigned created, game_overs, wins;
static uint32_t created_setup;

static void sequence_start(P4ModValue *args, P4ModValue *result)
{
    if (dispatched(&functions[START], args, result)) return;
    ++script_starts;
    P4ModValue create[1] = {{0}}, task_result = {0};
    create[0].u = script_setup;
    battle_create(create, &task_result);
}
/* btlScriptResolvePartyDeath (0x193450): "not yet" while the battle context exists, then the
 * Game Over when the protagonist is dead, else the allies are revived. */
static void script_resolve(P4ModValue *args, P4ModValue *result)
{
    if (dispatched(&functions[RESOLVE], args, result)) return;
    result->i = 0;
    if (P4_G32(BATTLE_CTX) != 0) return;
    if (hero_is_dead()) ++script_game_overs;
    else { ++script_revives; result->i = 1; }
}
static void battle_create(P4ModValue *args, P4ModValue *result)
{
    if (dispatched(&functions[CREATE], args, result)) return;
    ++created;
    P4_G32(BATTLE_CTX) = 0x1000;
    created_setup = (uint32_t)args[0].u;
    result->i = 0x01234560;
}
static int leader_defeated(void)
{
    uint32_t unit = P4_G32(P4_G32(FIELD_PARTY) + 4u);
    return (P4_G32(unit + 0xCu) & 0x80000u) != 0 || P4_G16(unit + 8u) == 0;
}
static void field_to_battle(P4ModValue *args, P4ModValue *result)
{
    if (dispatched(&functions[FIELD], args, result)) return;
    uint32_t w = P4_G32((uint32_t)args[0].u + 0x38u);
    uint32_t state = P4_G32(w + 4u);
    result->i = 1;
    switch (state) {
    case 6: case 7: case 8: case 9: P4_G32(w + 4u) = state + 1; break;
    case 10: {
        P4ModValue create[1] = {{0}}, task_result = {0};
        create[0].u = w + 0x50u;
        battle_create(create, &task_result);
        P4_G32(w + 4u) = 11;
        break;
    }
    case 11: P4_G32(w + 4u) = 12; break;
    case 12:
        if (leader_defeated()) { ++game_overs; P4_G32(w + 4u) = 0x63; }
        else { ++wins; P4_G32(w + 4u) = 13; }
        break;
    default: break;
    }
}
static void script_start_battle(void)
{
    P4ModValue args[1] = {{0}}, result = {0};
    sequence_start(args, &result);
}
static void poll_script(void)
{
    P4ModValue args[1] = {{0}}, result = {0};
    script_resolve(args, &result);
}
static void battle_ends(void) { P4_G32(BATTLE_CTX) = 0; }
static int hero_is_dead(void)
{
    return (P4_G32(0x007973C4u + 0xCu) & 0x80000u) != 0 || P4_G16(0x007973C4u + 8u) == 0;
}
static void poll(void)
{
    P4ModValue args[1] = {{0}}, result = {0};
    args[0].u = task;
    field_to_battle(args, &result);
    CHECK(result.i == 1);
}

static void set_unit(uint32_t unit, uint16_t id, uint16_t hp, uint16_t sp, uint32_t bad)
{
    P4_G16(unit + 2u) = id;
    P4_G16(unit + 8u) = hp;
    P4_G16(unit + 0xAu) = sp;
    P4_G32(unit + 0xCu) = bad;
}
/* A field battle at the start of state 10 with full vitals: Yu and Yosuke
 * against encounter 811 (enemy 310). */
static void fresh_battle(uint32_t return_mode)
{
    set_unit(HERO, 1, 80, 40, 0);
    set_unit(YOSUKE, 2, 60, 30, 0);
    set_unit(CHIE, 3, 70, 20, 0);
    P4_G8(HERO + 0x11u) = 0x21;   /* a buff byte */
    memset(P4_G2H(enemies), 0, 0x12Cu);
    P4_G16(enemies) = 1;          /* EC genus */
    P4_G16(enemies + 2u) = 1;
    P4_G32(enemies + 4u) = enemies + 0xCu;
    P4_G16(enemies + 8u) = 811;
    set_unit(enemies + 0xCu, 310, 1000, 200, 0);
    P4_G32(work + 4u) = 10;
    P4_G32(work + 0x10Cu) = return_mode;
    created = game_overs = wins = 0;
}
/* The battle is lost: Yu falls, Yosuke spent SP and is afraid, the Knight is
 * hurt and has summoned a second unit into its group. */
static void lose(void)
{
    set_unit(HERO, 1, 0, 12, 0x80000u);
    set_unit(YOSUKE, 2, 9, 3, 0x4u);
    P4_G8(HERO + 0x11u) = 0;
    P4_G16(enemies + 0xCu + 8u) = 340;
    P4_G16(enemies + 2u) = 2;
    set_unit(enemies + 0xCu + 0x30u, 269, 500, 50, 0);
}

static struct {
    int calls, answers[8], count, always;
    P4ModDefeat seen[8];
    char reasons[8][128];
} script;
static int answer(const P4ModDefeat *defeat, void *user)
{
    (void)user;
    int i = script.calls++;
    if (i < 8) {
        script.seen[i] = *defeat;
        snprintf(script.reasons[i], sizeof script.reasons[i], "%s", defeat->reason);
    }
    if (script.always >= 0 && i >= script.count) return script.always;
    return i < script.count ? script.answers[i] : P4_MOD_DEFEAT_GAME_OVER;
}
static void plan(int always, int count, const int *answers)
{
    memset(&script, 0, sizeof script);
    script.always = always;
    script.count = count;
    for (int i = 0; i < count; ++i) script.answers[i] = answers[i];
}

static unsigned refusals;
static uint32_t refused_source;
static int ironman(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    (void)user;
    ++refusals;
    CHECK(context->count == 2 && !strcmp(context->fields[0].name, "encounter") && (context->fields[0].value == 811 || context->fields[0].value == 514));
    CHECK(!strcmp(context->fields[1].name, "source"));
    refused_source = (uint32_t)context->fields[1].value;
    snprintf(reason, capacity, "IRONMAN: a lost battle ends this run");
    return 0;
}

int main(void)
{
    p4_os_init();
    /* These retail globals are not mapped by the host test's normal startup. */
    if (p4t_guest_map(0x00796000u, 0x6000u) || p4t_guest_map(0x007ef000u, 0x1000u) || p4t_guest_map(0x00764000u, 0x1000u)) return 77;
    task = p4_gport_alloc(0x40, 16);
    work = p4_gport_alloc(0x160, 16);
    leader = p4_gport_alloc(0x10, 16);
    yosuke_group = p4_gport_alloc(0x10, 16);
    enemies = p4_gport_alloc(0x130, 16);
    other_setup = p4_gport_alloc(0x40, 16);
    script_setup = p4_gport_alloc(0x40, 16);
    group_pool = p4_gport_alloc(0x130u * 64u, 16);
    if (!script_setup || !group_pool || !task || !work || !leader || !yosuke_group || !enemies || !other_setup) return 77;
    P4_G32(task + 0x38u) = work;
    P4_G16(leader) = 0; P4_G16(leader + 2u) = 1; P4_G32(leader + 4u) = HERO;
    P4_G16(yosuke_group) = 0; P4_G16(yosuke_group + 2u) = 1; P4_G32(yosuke_group + 4u) = YOSUKE;
    P4_G32(FIELD_PARTY) = leader;
    P4_G32(work + 0x54u) = leader;       /* setup party[0] */
    P4_G32(work + 0x58u) = yosuke_group; /* setup party[1] */
    P4_G32(work + 0x64u) = enemies;      /* setup enemies[0] */

    /* One handler; the seams exist from the first one, enabled while it is. */
    const char *why = NULL;
    P4ModDefeatHandler *handler = p4_mod_defeat_add(3, answer, NULL, &why);
    CHECK(handler && functions[FIELD].observers && functions[CREATE].observers && functions[START].observers &&
          functions[RESOLVE].observers);
    CHECK(entries[FIELD].enabled == P4_MOD_ACTIVE && entries[CREATE].enabled == P4_MOD_ACTIVE);
    CHECK(!p4_mod_defeat_add(4, answer, NULL, &why) && why && strstr(why, "another mod"));

    /* A won battle: state 12 with the leader standing never asks. */
    plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
    fresh_battle(1);
    poll(); poll(); poll();
    CHECK(created == 1 && created_setup == work + 0x50u && wins == 1 && game_overs == 0 && script.calls == 0);

    /* Declined: a field encounter request's loss is offered with the battle
     * start values; the handler lets the Game Over run (retail). */
    plan(P4_MOD_DEFEAT_GAME_OVER, 0, NULL);
    fresh_battle(1);
    poll(); lose(); poll(); poll();
    CHECK(script.calls == 1 && game_overs == 1 && created == 1 && P4_G32(work + 4u) == 0x63);
    CHECK(script.seen[0].encounter == 811 && script.seen[0].source == P4_MOD_DEFEAT_FIELD && script.seen[0].polls == 0);
    CHECK(script.seen[0].allowed == 1 && !strcmp(script.reasons[0], ""));
    CHECK(script.seen[0].members == 2 && script.seen[0].party[0].id == 1 && script.seen[0].party[0].hp == 80 &&
          script.seen[0].party[0].sp == 40 && script.seen[0].party[1].id == 2 && script.seen[0].party[1].hp == 60 &&
          script.seen[0].party[1].sp == 30 && script.seen[0].party[1].ailments == 0);
    CHECK(P4_G16(HERO + 8u) == 0 && P4_G16(YOSUKE + 0xAu) == 3); /* nothing restored */

    /* Held while the player decides (state 9, then asked again at 10), then
     * declined: the Game Over still runs from state 12. */
    static const int hold_then_decline[] = {P4_MOD_DEFEAT_HOLD, P4_MOD_DEFEAT_HOLD, P4_MOD_DEFEAT_HOLD};
    plan(P4_MOD_DEFEAT_GAME_OVER, 3, hold_then_decline);
    fresh_battle(1);
    poll(); lose(); poll();
    poll(); CHECK(P4_G32(work + 4u) == 10 && script.calls == 1 && created == 1);
    poll(); poll(); poll();
    CHECK(script.calls == 4 && script.seen[3].polls == 3 && game_overs == 1 && created == 1);

    /* Accepted: the battle-start records and enemy group are copied back and
     * the same setup creates the battle again; this time it is won. */
    uint8_t hero_start[0x30], yosuke_start[0x30], chie_start[0x30], group_start[0x12C];
    static const int hold_then_retry[] = {P4_MOD_DEFEAT_HOLD, P4_MOD_DEFEAT_RETRY};
    plan(P4_MOD_DEFEAT_GAME_OVER, 2, hold_then_retry);
    fresh_battle(1);
    memcpy(hero_start, P4_G2H(HERO), 0x30);
    memcpy(yosuke_start, P4_G2H(YOSUKE), 0x30);
    memcpy(chie_start, P4_G2H(CHIE), 0x30);
    memcpy(group_start, P4_G2H(enemies), 0x12C);
    poll(); lose(); poll(); poll(); poll();
    CHECK(script.calls == 2 && game_overs == 0 && created == 2 && created_setup == work + 0x50u);
    CHECK(!memcmp(P4_G2H(HERO), hero_start, 0x30) && !memcmp(P4_G2H(YOSUKE), yosuke_start, 0x30));
    CHECK(!memcmp(P4_G2H(CHIE), chie_start, 0x30) && !memcmp(P4_G2H(enemies), group_start, 0x12C));
    CHECK(P4_G32(work + 4u) == 11);
    poll(); poll();
    CHECK(wins == 1 && game_overs == 0 && script.calls == 2);
    /* Lost again: the retried battle's own start is the restore point. */
    plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
    P4_G32(work + 4u) = 10;
    created = wins = 0;
    poll(); lose(); poll(); poll();
    CHECK(script.calls == 1 && created == 2 && !memcmp(P4_G2H(HERO), hero_start, 0x30));

    /* Fixture for the 0x214/0x215 roster replacement, combat money delta,
     * item consumption and battle flags. A second retry must restore exactly
     * the original bytes, not the first loss's mutated state. */
    for (int encounter = 0x214; encounter <= 0x215; ++encounter) {
        uint8_t stock[0x244], items[0x600], flags[0x2c0], party[9 * 0x88];
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        P4_G16(enemies + 8u) = (uint16_t)encounter;
        memset(P4_G2H(PERSONAS), 0x35, sizeof stock);
        memset(P4_G2H(ITEMS), 0x12, sizeof items);
        memset(P4_G2H(FLAGS), 0x9a, sizeof flags);
        memset(P4_G2H(KNOWLEDGE), 0x68, 0x154);
        memset(P4_G2H(COMMUNITY), 0x47, 0x158);
        P4_G32(SCRIPT_VALUES + 0x79u * 4u) = 123;
        P4_G32(HERO + 0x3cu) = 98765;
        P4_G16(ITEMS - 6u) = 22;
        P4_G32(MONEY) = 123456;
        memcpy(stock, P4_G2H(PERSONAS), sizeof stock);
        memcpy(items, P4_G2H(ITEMS), sizeof items);
        memcpy(flags, P4_G2H(FLAGS), sizeof flags);
        memcpy(party, P4_G2H(0x00796E50u), sizeof party);
        poll();
        for (int attempt = 0; attempt < 2; ++attempt) {
            memset(P4_G2H(PERSONAS + 4u), 0, 0x240);
            P4_G16(PERSONAS) = 0;
            P4_G16(PERSONAS + 4u) = 1;
            P4_G16(PERSONAS + 6u) = (uint16_t)(encounter == 0x214 ? 0xe0 : 0xe1);
            P4_G32(MONEY) = 5;
            P4_G8(ITEMS + 0x301u) = 0;
            P4_G32(HERO + 0x3cu) = 0;
            P4_G16(ITEMS - 6u) = 4;
            P4_G8(FLAGS + 0x100u) ^= 0xff;
            P4_G8(KNOWLEDGE + 0x88u) = 0;
            P4_G32(SCRIPT_VALUES + 0x79u * 4u) = 42;
            P4_G8(COMMUNITY + 0x80u) = 0;
            P4_G8(0x00796E50u + 0x40u) ^= 0xff;
            lose(); poll(); poll();
            CHECK(!memcmp(P4_G2H(PERSONAS), stock, sizeof stock));
            CHECK(!memcmp(P4_G2H(ITEMS), items, sizeof items));
            CHECK(!memcmp(P4_G2H(FLAGS), flags, sizeof flags));
            CHECK(P4_G8(KNOWLEDGE + 0x88u) == 0x68);
            CHECK(P4_G32(SCRIPT_VALUES + 0x79u * 4u) == 123);
            CHECK(P4_G8(COMMUNITY + 0x80u) == 0x47);
            CHECK(P4_G32(HERO + 0x3cu) == 98765);
            CHECK(P4_G16(ITEMS - 6u) == 22);
            CHECK(!memcmp(P4_G2H(0x00796E50u), party, sizeof party));
            CHECK(P4_G32(MONEY) == 123456);
        }
        CHECK(script.calls == 2 && created == 3 && !game_overs);
    }

    /* Sidecar state is captured with the battle start and restored exactly:
     * the expanded items and the enemy knowledge, on every retry. */
    fake_items.active = fake_knowledge.active = 1;
    fake_items.generation = fake_knowledge.generation = 7;
    for (int attempt = 0; attempt < 1; ++attempt) {
        static const uint32_t items_start[4] = {3, 0, 99, 5}, knowledge_start[4] = {0x11, 0, 0x80, 1};
        memcpy(fake_items.data, items_start, sizeof items_start);
        memcpy(fake_knowledge.data, knowledge_start, sizeof knowledge_start);
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        poll();
        CHECK(fake_items.captures >= 1 && fake_knowledge.captures >= 1);
        for (int retry = 0; retry < 2; ++retry) {
            fake_items.data[0] = 0; fake_items.data[3] = 77;       /* consumed, found */
            fake_knowledge.data[0] |= 0x400; fake_knowledge.data[1] = 9; /* learned in the lost battle */
            unsigned items_restores = fake_items.restores, knowledge_restores = fake_knowledge.restores;
            lose(); poll(); poll();
            CHECK(script.seen[retry].allowed == 1 && fake_items.restores == items_restores + 1 &&
                  fake_knowledge.restores == knowledge_restores + 1);
            CHECK(!memcmp(fake_items.data, items_start, sizeof items_start));
            CHECK(!memcmp(fake_knowledge.data, knowledge_start, sizeof knowledge_start));
        }
        CHECK(script.calls == 2 && created == 3 && !game_overs);
    }
    /* A win never restores; a battle start frees the previous capture. */
    {
        unsigned restores = fake_items.restores + fake_knowledge.restores, frees = fake_items.frees;
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        poll(); poll(); poll();
        CHECK(wins == 1 && fake_items.restores + fake_knowledge.restores == restores && fake_items.frees > frees);
    }

    /* Fail closed, naming the state: not capturable, or no longer the captured one. */
    for (int sidecar = 0; sidecar < 2; ++sidecar) {
        FakeSidecar *side = sidecar ? &fake_knowledge : &fake_items;
        FakeSidecar *other = sidecar ? &fake_items : &fake_knowledge;
        const char *capture_reason = sidecar ? "the enemy knowledge state could not be captured (out of memory)" :
                                               "the expanded item state could not be captured (out of memory)";
        const char *changed_reason = sidecar ?
            "the enemy knowledge store is no longer the one captured at the battle start" :
            "the expanded item table is no longer the one captured at the battle start";
        side->data[0] = 5; other->data[0] = 6;

        side->fail_capture = 1;
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        poll(); lose(); poll(); poll();
        CHECK(script.calls == 1 && !script.seen[0].allowed && !strcmp(script.reasons[0], capture_reason));
        CHECK(game_overs == 1 && created == 1 && side->data[0] == 5 && other->data[0] == 6);
        side->fail_capture = 0;

        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        poll(); lose();
        ++side->generation; /* the store was replaced under the battle */
        side->data[0] = 8; other->data[0] = 9;
        poll(); poll();
        CHECK(script.calls == 1 && !script.seen[0].allowed && !strcmp(script.reasons[0], changed_reason));
        CHECK(game_overs == 1 && created == 1 && side->data[0] == 8 && other->data[0] == 9); /* nothing restored */
    }
    /* A store that appears (or is lost) after the battle start is no match either. */
    fake_items.active = 0;
    plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
    fresh_battle(1);
    poll(); lose(); fake_items.active = 1;
    poll(); poll();
    CHECK(script.calls == 1 && !script.seen[0].allowed && strstr(script.reasons[0], "expanded item table"));
    fake_items.active = fake_knowledge.active = 0;
    /* No sidecar at all: the retail snapshot alone retries. */
    plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
    fresh_battle(1);
    poll(); lose(); poll(); poll();
    CHECK(script.calls == 1 && script.seen[0].allowed == 1 && created == 2 && !game_overs);

    /* Ironman: the "retry" authorizer refuses once per defeat; the handler
     * sees why, and a retry it answers anyway is not performed. */
    P4ModAuthorizer *rule = p4_mod_authorizer_add(5, "retry", ironman, NULL);
    CHECK(rule != NULL);
    static const int refused_then_retry[] = {P4_MOD_DEFEAT_HOLD, P4_MOD_DEFEAT_HOLD, P4_MOD_DEFEAT_RETRY};
    plan(P4_MOD_DEFEAT_GAME_OVER, 3, refused_then_retry);
    refusals = 0;
    fresh_battle(1);
    poll(); lose(); poll(); poll(); poll(); poll(); poll();
    CHECK(refusals == 1 && script.calls == 3 && script.seen[0].allowed == 0 && script.seen[2].allowed == 0);
    CHECK(refused_source == P4_MOD_DEFEAT_FIELD);
    CHECK(!strcmp(script.reasons[0], "IRONMAN: a lost battle ends this run"));
    CHECK(game_overs == 1 && created == 1 && P4_G16(HERO + 8u) == 0);
    CHECK(p4_mod_authorizer_remove(rule) == 0);

    /* A shadow touched on the field, lost after a battle this seam did not
     * record (another setup started last): no retry is possible. */
    plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
    fresh_battle(0);
    poll(); lose();
    p4_mod_defeat_battle_begin(other_setup);
    poll(); poll();
    CHECK(script.calls == 1 && script.seen[0].source == P4_MOD_DEFEAT_COLLISION && script.seen[0].allowed == 0);
    CHECK(!strcmp(script.reasons[0], "the battle start was not recorded") && script.seen[0].members == 0);
    CHECK(game_overs == 1 && created == 1);

    /* A handler that never lets go: the Game Over runs after the limit. */
    plan(P4_MOD_DEFEAT_HOLD, 0, NULL);
    fresh_battle(1);
    poll(); lose(); poll();
    for (unsigned i = 0; i < 2u * P4_MOD_DEFEAT_HOLD_LIMIT && !game_overs; ++i) poll();
    CHECK(script.calls == (int)P4_MOD_DEFEAT_HOLD_LIMIT && game_overs == 1);

    /* ---- Script battles: Shadow Yukiko (encounter 514) is started by a field
     * script, which polls btlScriptResolvePartyDeath and starts the Game Over
     * itself when it finds the battle gone and the protagonist dead. */
    P4_G32(script_setup + 4u) = leader;
    P4_G32(script_setup + 8u) = yosuke_group;
    P4_G32(script_setup + 0x14u) = enemies;
    P4_G16(script_setup + 0x20u) = 0x41;
    P4_G16(script_setup + 0x22u) = 2;
    {
        uint8_t hero_start[0x30], yosuke_start[0x30];
        /* Won: the script goes on (the retail call revives and answers 1), nobody is asked. */
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        P4_G16(enemies + 8u) = 514;
        created = 0; script_game_overs = script_revives = 0;
        script_start_battle();
        CHECK(script_starts == 1 && created == 1 && created_setup == script_setup && P4_G32(BATTLE_CTX) != 0);
        poll_script();                         /* the battle runs: "not yet" */
        CHECK(script_game_overs == 0 && script_revives == 0);
        battle_ends();
        poll_script();
        CHECK(script_game_overs == 0 && script_revives == 1 && script.calls == 0);
        CHECK(P4_G32(BATTLE_CTX) == 0);

        /* Lost, Game Over: the call runs untouched and the script starts the Game Over. */
        plan(P4_MOD_DEFEAT_GAME_OVER, 0, NULL);
        fresh_battle(1);
        P4_G16(enemies + 8u) = 514;
        script_game_overs = script_revives = 0;
        script_start_battle();
        poll_script();
        lose();
        battle_ends();
        poll_script();
        CHECK(script.calls == 1 && script.seen[0].allowed == 1 && script_game_overs == 1 && created == 1);
        CHECK(script.seen[0].encounter == 514 && script.seen[0].source == P4_MOD_DEFEAT_SCRIPT && script.seen[0].members == 2);
        poll_script();                         /* the script is gone; nothing asks again */
        CHECK(script.calls == 1 && P4_G16(HERO + 8u) == 0);  /* nothing restored */

        /* Held, then retried: the script keeps waiting (the call sees a battle), the
         * records come back, fresh groups for the recorded party and encounter start
         * the same battle again. */
        static const int hold_then_retry_script[] = {P4_MOD_DEFEAT_HOLD, P4_MOD_DEFEAT_HOLD, P4_MOD_DEFEAT_RETRY};
        plan(P4_MOD_DEFEAT_GAME_OVER, 3, hold_then_retry_script);
        fake_items.active = fake_knowledge.active = 1;
        fake_items.data[0] = 7; fake_knowledge.data[0] = 9;
        fresh_battle(1);
        P4_G16(enemies + 8u) = 514;
        memcpy(hero_start, P4_G2H(HERO), 0x30);
        memcpy(yosuke_start, P4_G2H(YOSUKE), 0x30);
        created = 0; pcs_created = enemy_groups_created = fades = 0; pool_used = 0;
        script_game_overs = script_revives = 0;
        script_start_battle();
        poll_script();
        lose();
        fake_items.data[0] = 1; fake_knowledge.data[0] = 2;
        battle_ends();
        poll_script();
        CHECK(script.calls == 1 && P4_G16(HERO + 8u) == 0 && script_game_overs == 0 && P4_G32(BATTLE_CTX) == 0);  /* held */
        poll_script();
        CHECK(script.calls == 2 && created == 1 && script_game_overs == 0);                                    /* held again */
        poll_script();
        CHECK(script.calls == 3 && script.seen[2].polls == 2 && script_game_overs == 0);                       /* retried */
        CHECK(declared_reads == 1);
        CHECK(created == 2 && created_setup != script_setup && pcs_created == 2 && enemy_groups_created == 1 && fades == 1);
        CHECK(created_pc_ids[0] == 1 && created_pc_ids[1] == 2 && created_encounter == 514);
        CHECK(P4_G32(created_setup + 4u) != leader && P4_G32(created_setup + 0x14u) == last_enemy_group &&
              P4_G32(created_setup + 4u) != 0 && P4_G32(created_setup + 8u) != 0 && P4_G32(created_setup + 0xCu) == 0);
        CHECK(P4_G16(created_setup + 0x20u) == 0x41 && P4_G16(created_setup + 0x22u) == 2);
        CHECK(!memcmp(P4_G2H(HERO), hero_start, 0x30) && !memcmp(P4_G2H(YOSUKE), yosuke_start, 0x30));
        CHECK(fake_items.data[0] == 7 && fake_knowledge.data[0] == 9);
        CHECK(P4_G32(BATTLE_CTX) != 0);        /* the retried battle exists */
        poll_script();
        CHECK(script.calls == 3 && script_game_overs == 0);
        lose();                                /* lost again: the new battle's own start is the restore point */
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        battle_ends();
        poll_script();
        CHECK(script.calls == 1 && created == 3 && !memcmp(P4_G2H(HERO), hero_start, 0x30) && script_game_overs == 0);
        CHECK(declared_reads == 1);  /* the packet is reused and declared once */
        poll_script();                         /* the twice-retried battle runs */
        battle_ends();
        poll_script();                         /* won: hero standing, nobody asked, the script revives */
        CHECK(script.calls == 1 && script_game_overs == 0 && script_revives == 1);

        /* Refusals name their state, and the Game Over runs. */
        fake_items.fail_capture = 1;
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        P4_G16(enemies + 8u) = 514;
        created = 0; script_game_overs = 0;
        script_start_battle();
        poll_script();
        lose();
        battle_ends();
        poll_script();
        CHECK(script.calls == 1 && !script.seen[0].allowed && script_game_overs == 1 && created == 1 &&
              !strcmp(script.reasons[0], "the expanded item state could not be captured (out of memory)"));
        fake_items.fail_capture = 0;
        fake_items.active = fake_knowledge.active = 0;

        P4_G32(script_setup + 8u) = other_setup;    /* a party member that is no playable record */
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        fresh_battle(1);
        P4_G16(enemies + 8u) = 514;
        created = 0; script_game_overs = 0;
        script_start_battle();
        poll_script();
        lose();
        battle_ends();
        poll_script();
        CHECK(script.calls == 1 && !script.seen[0].allowed && script_game_overs == 1 && created == 1 &&
              !strcmp(script.reasons[0], "a party member of the battle is not a playable record"));
        P4_G32(script_setup + 8u) = yosuke_group;

        P4ModAuthorizer *script_rule = p4_mod_authorizer_add(6, "retry", ironman, NULL);
        CHECK(script_rule != NULL);
        plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
        refusals = 0;
        fresh_battle(1);
        P4_G16(enemies + 8u) = 514;
        created = 0; script_game_overs = 0;
        script_start_battle();
        poll_script();
        lose();
        battle_ends();
        poll_script();
        CHECK(script_game_overs == 1 && refusals == 1 && refused_source == P4_MOD_DEFEAT_SCRIPT && created == 1 &&
              !strcmp(script.reasons[0], "IRONMAN: a lost battle ends this run"));
        CHECK(p4_mod_authorizer_remove(script_rule) == 0);

        /* A field battle after a script battle is judged by its own path. */
        plan(P4_MOD_DEFEAT_GAME_OVER, 0, NULL);
        fresh_battle(1);
        poll(); lose(); poll(); poll();
        CHECK(script.calls == 1 && script.seen[0].source == P4_MOD_DEFEAT_FIELD && game_overs == 1);
        battle_ends();
        script_game_overs = 0;
        poll_script();
        CHECK(script.calls == 1 && script_game_overs == 1);   /* not a script battle: the script's own answer */
    }

    /* A disabled handler leaves the retail path: no seam runs. */
    CHECK(p4_mod_defeat_enable(handler, 0) == 0);
    CHECK(entries[FIELD].enabled == P4_MOD_INACTIVE && entries[CREATE].enabled == P4_MOD_INACTIVE &&
          entries[START].enabled == P4_MOD_INACTIVE && entries[RESOLVE].enabled == P4_MOD_INACTIVE);
    plan(P4_MOD_DEFEAT_RETRY, 0, NULL);
    fresh_battle(1);
    poll(); lose(); poll(); poll();
    CHECK(script.calls == 0 && game_overs == 1);
    /* Removal frees the slot for another owner; the old handle stays dead. */
    CHECK(p4_mod_defeat_remove(handler) == 0 && p4_mod_defeat_enable(handler, 1) == -1);
    P4ModDefeatHandler *next = p4_mod_defeat_add(4, answer, NULL, &why);
    CHECK(next && entries[FIELD].enabled == P4_MOD_ACTIVE);
    p4_mod_defeat_remove_owner(4);
    CHECK(entries[FIELD].enabled == P4_MOD_INACTIVE && p4_mod_hooks_quiescent());

    printf("battle_defeat: %s (declined, held, retried from the battle start, Ironman refusal, unrecorded start, hold limit)\n",
           failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
