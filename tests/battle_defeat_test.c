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
#include "p4pc/gmem.h"
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_defeat.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/os.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "battle_defeat: FAIL line %d: %s\n", __LINE__, #x); failures++; } } while (0)
static int failures;

enum { FIELD, CREATE, COUNT };
static void field_to_battle(P4ModValue *args, P4ModValue *result);
static void battle_create(P4ModValue *args, P4ModValue *result);
static P4ModEntry entries[COUNT];
/* Ascending addresses, as the generated table is (p4_mod_function_find bisects). */
static P4ModFunction functions[COUNT] = {
    {0x00172e00u, &entries[FIELD], "ip", field_to_battle, NULL, NULL},  /* FldEvent_FieldToBattle */
    {0x00192e90u, &entries[CREATE], "ii", battle_create, NULL, NULL},   /* btlTaskCreate */
};
P4ModFunction *const p4_mod_function_table[COUNT] = {&functions[FIELD], &functions[CREATE]};
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

#define HERO 0x007973C4u
#define YOSUKE (0x00796E50u + 4u)
#define CHIE (0x00796E50u + 0x88u + 4u)
#define FIELD_PARTY 0x007EF9F8u
static uint32_t task, work, leader, yosuke_group, enemies, other_setup;
static unsigned created, game_overs, wins;
static uint32_t created_setup;

static void battle_create(P4ModValue *args, P4ModValue *result)
{
    if (dispatched(&functions[CREATE], args, result)) return;
    ++created;
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
static int ironman(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    (void)user;
    ++refusals;
    CHECK(context->count == 2 && !strcmp(context->fields[0].name, "encounter") && context->fields[0].value == 811);
    CHECK(!strcmp(context->fields[1].name, "source") && context->fields[1].value == P4_MOD_DEFEAT_FIELD);
    snprintf(reason, capacity, "IRONMAN: a lost battle ends this run");
    return 0;
}

int main(void)
{
    p4_os_init();
    task = p4_gport_alloc(0x40, 16);
    work = p4_gport_alloc(0x160, 16);
    leader = p4_gport_alloc(0x10, 16);
    yosuke_group = p4_gport_alloc(0x10, 16);
    enemies = p4_gport_alloc(0x130, 16);
    other_setup = p4_gport_alloc(0x40, 16);
    if (!task || !work || !leader || !yosuke_group || !enemies || !other_setup) return 77;
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
    CHECK(handler && functions[FIELD].observers && functions[CREATE].observers);
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

    /* A disabled handler leaves the retail path: no seam runs. */
    CHECK(p4_mod_defeat_enable(handler, 0) == 0);
    CHECK(entries[FIELD].enabled == P4_MOD_INACTIVE && entries[CREATE].enabled == P4_MOD_INACTIVE);
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
