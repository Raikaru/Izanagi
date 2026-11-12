/* p4.battle's panel observers (src/platform/mods/battle_menu.c) over the real
 * hook dispatch, with hand-built descriptors at the observed addresses.
 *
 * Once a mod calls p4.clock.updates, the kwlnRunFrame observers route every
 * game frame through the hook dispatch, and the frame boundary that runs
 * frame_start (the present inside the task frame) is inside it. A mod's first
 * p4.battle.menu() there must still see the commands panel on later frames. */
#include "../src/platform/mods/battle_menu.h"
#include "p4pc/battle_sim.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include <stdint.h>
#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "battle_menu: FAIL line %d: %s\n", __LINE__, #x); failures++; } } while (0)
static int failures;

int p4_battle_sim_active;
static uint32_t flips;
uint32_t p4pc_flip_count(void) { return flips; }

enum { FRAME, NEAREST, BY_STATUS, READ_INPUT, COMMANDS, SKILLS, ITEMS, SUB_LIST, SWAP, COUNT };
static void frame(P4ModValue *args, P4ModValue *result);
static void commands(P4ModValue *args, P4ModValue *result);
static void never(P4ModValue *args, P4ModValue *result);
static P4ModEntry entries[COUNT];
/* Ascending addresses, as the generated table is (p4_mod_function_find bisects). */
static P4ModFunction functions[COUNT] = {
    {0x00101270u, &entries[FRAME], "i", frame, NULL, NULL},          /* kwlnRunFrame */
    {0x001d8cb0u, &entries[NEAREST], "ipp", never, NULL, NULL},      /* btlEffectSelectNearestEntry */
    {0x001d8e50u, &entries[BY_STATUS], "vpp", never, NULL, NULL},    /* btlEffectSelectTargetByStatus */
    {0x00202e70u, &entries[READ_INPUT], "ip", never, NULL, NULL},    /* btlSelectionReadInput */
    {0x00207320u, &entries[COMMANDS], "ipip", commands, NULL, NULL}, /* btlSelectionHandleInput */
    {0x00208b00u, &entries[SKILLS], "Hpp", never, NULL, NULL},       /* btlSelectionHandleSkillInput */
    {0x002091f0u, &entries[ITEMS], "Hpp", never, NULL, NULL},        /* btlSelectionHandleItemInput */
    {0x00209870u, &entries[SUB_LIST], "Hpip", never, NULL, NULL},    /* btlSelectionOpenSubList */
    {0x0020a640u, &entries[SWAP], "Hpp", never, NULL, NULL},         /* btlSelectionHandleSwapInput */
};
P4ModFunction *const p4_mod_function_table[COUNT] = {
    &functions[FRAME], &functions[NEAREST], &functions[BY_STATUS], &functions[READ_INPUT], &functions[COMMANDS],
    &functions[SKILLS], &functions[ITEMS], &functions[SUB_LIST], &functions[SWAP],
};
const size_t p4_mod_function_count = COUNT;

/* The guard tools/mod_codegen.py puts at a hookable function's start: an active
 * entry sends the call through the hook dispatch, whose original body reaches
 * the function once more (BYPASS). 1 when the dispatch handled the call. */
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

static int unexpected;
static void never(P4ModValue *args, P4ModValue *result)
{
    (void)args; (void)result;
    ++unexpected;
}

static void commands(P4ModValue *args, P4ModValue *result)
{
    if (dispatched(&functions[COMMANDS], args, result)) return;
    result->i = 0; /* no input this frame */
}

/* One game frame: the task frame (the protagonist's commands panel reads its
 * input when one is open), then the present's frame boundary, where frame_start
 * runs: here a mod reading the panel, as the hard-mode telegraph does. */
static uint32_t panel_selection, panel_work;
static int read_result;
static P4BattleMenu read_menu;
static void frame(P4ModValue *args, P4ModValue *result)
{
    if (dispatched(&functions[FRAME], args, result)) return;
    if (panel_selection) {
        P4ModValue input[3] = {{0}}, none = {0};
        input[0].u = panel_selection;
        input[1].u = panel_work;
        commands(input, &none);
    }
    ++flips;
    CHECK(!p4_mod_hooks_quiescent()); /* the kwlnRunFrame dispatch encloses the frame boundary */
    read_result = p4_battle_menu(&read_menu);
    result->i = 1;
}
static void run_frame(void)
{
    P4ModValue none[1] = {{0}}, result = {0};
    frame(none, &result);
    CHECK(result.i == 1);
}

/* p4.clock's kwlnRunFrame observer (src/platform/mods/game_services.c). */
static unsigned updates;
static int count_update(P4ModCall *call, void *user)
{
    (void)user;
    if (call->result->i == 1) ++updates;
    return 0;
}

int main(void)
{
    p4_os_init();
    const uint32_t selection = p4_gport_alloc(0x34, 16), work = p4_gport_alloc(0x5b4, 16);
    const uint32_t action = p4_gport_alloc(0x440, 16), unit = p4_gport_alloc(0xa70, 16);
    if (!selection || !work || !action || !unit) return 77;
    P4_G32(work + 0x178) = action;  /* the panel work's acting BtlAction */
    P4_G32(action + 0x30) = unit;   /* its BtlUnit */
    P4_G16(unit + 0xa4) = 1;        /* the protagonist */
    P4_G16(selection + 4) = 3;      /* the cursor on Attack */
    P4_G16(selection + 8) = 8;      /* eight commands */

    /* The first mod VM's initialization creates the services' kwlnRunFrame
     * observer and the panel observers, all disabled; the other VMs' change
     * nothing. */
    P4ModHook *clock = p4_mod_hook_add(P4_MOD_NATIVE_OWNER, &functions[FRAME], P4_MOD_AFTER, count_update, NULL);
    CHECK(clock && p4_mod_hook_enable(clock, 0) == 0);
    p4_battle_menu_prepare();
    p4_battle_menu_prepare();
    for (int i = FRAME; i < COUNT; ++i) CHECK(functions[i].observers && entries[i].enabled == P4_MOD_INACTIVE);
    /* A mod's p4.clock.updates enables the kwlnRunFrame observer. */
    CHECK(p4_mod_hook_enable(clock, 1) == 0 && entries[FRAME].enabled == P4_MOD_ACTIVE);

    /* p4.battle's first use comes from frame_start inside that dispatch: no
     * panel waits yet, and the observers are on from now on. */
    run_frame();
    CHECK(updates == 1 && read_result == -1);
    for (int i = NEAREST; i < COUNT; ++i) CHECK(entries[i].enabled == P4_MOD_ACTIVE);

    /* The commands panel reads its input; that frame's frame_start sees it. */
    panel_selection = selection;
    panel_work = work;
    run_frame();
    CHECK(updates == 2 && read_result == 0);
    CHECK(read_menu.panel == P4_BATTLE_PANEL_COMMANDS && read_menu.action == action && read_menu.unit == unit);
    CHECK(read_menu.selection == selection && read_menu.cursor == 3 && read_menu.count == 8);
    run_frame();
    CHECK(read_result == 0 && read_menu.action == action);

    /* The panel closed: nothing waits the next frame. */
    panel_selection = 0;
    run_frame();
    CHECK(updates == 4 && read_result == -1);
    CHECK(unexpected == 0 && p4_mod_hooks_quiescent());
    printf("battle_menu: %s (observers created disabled, enabled by a first use inside a dispatch)\n",
           failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
