/* The battle command panel as the game drives it; see battle_menu.h.
 *
 * Layouts (decomp src/Battle/btlEffect_inferred_00202c60.c): a BtlSelection is
 * 0x34 bytes with first (+2), selected (+4), pageSize (+6) and count (+8); the
 * 0x5B4-byte panel work keeps the acting BtlAction at +0x178. The action's
 * DistanceSelection at +0x98 (btlEffectSelectNearestEntry) holds twelve target
 * action words, count (+0x38) and selected (+0x3A); a BtlAction's unit is at
 * +0x30 and a unit's character ID at +0xA4 (1..8 are the party). */
#include "battle_menu.h"
#include "p4pc/battle_sim.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/platform.h"
#include <string.h>

#define SEL_FIRST 2u
#define SEL_SELECTED 4u
#define SEL_PAGE 6u
#define SEL_COUNT 8u
#define WORK_ACTION 0x178u
#define ACTION_UNIT 0x30u
#define UNIT_CHAR 0xA4u
#define TARGETS 0x98u
#define TARGET_COUNT 0x38u
#define TARGET_SELECTED 0x3Au
#define READ_CONFIRM 2

typedef struct Observer {
    uint32_t address;
    const char *signature;
    int phase, panel;
    P4ModHook *hook;
} Observer;

static struct {
    P4BattleMenu menu;
    uint32_t seen, generation;
    /* 1 once every observer exists (created disabled), -1 when one is
     * unavailable; active once the first use enabled them. */
    int installed, active;
    /* The wait's driver and whether it changed anything the player then confirms. */
    const void *owner;
    int influenced;
    /* A chosen command: written into the cursor before the panel's next input
     * read, which then returns confirm once. */
    uint32_t chooseSelection;
    int chooseCommand;
    uint32_t confirmSelection;
    /* The requested default target of an action's next target selection. */
    uint32_t aimAction, aimUnit;
    void (*use)(const void *owner);
} state = {.chooseCommand = -1};

static int waiting(void)
{
    return state.menu.panel != P4_BATTLE_PANEL_NONE && !p4_battle_sim_active &&
        p4pc_flip_count() - state.seen <= 1u;
}

static void end_wait(void)
{
    state.owner = NULL;
    state.influenced = 0;
    state.chooseSelection = 0;
    state.chooseCommand = -1;
    state.generation++;
}

static void record(int panel, uint32_t selection, uint32_t work)
{
    const uint32_t action = P4_G32(work + WORK_ACTION);
    const uint32_t unit = action ? P4_G32(action + ACTION_UNIT) : 0;
    const uint32_t id = unit ? P4_G16(unit + UNIT_CHAR) : 0;
    if (!unit || id < 1u || id > 8u) return; /* not a party member's own command */
    /* A different panel or action, or a gap, starts a new wait. */
    if (!waiting() || state.menu.panel != panel || state.menu.selection != selection || state.menu.action != action)
        end_wait();
    state.menu = (P4BattleMenu){.panel = panel, .selection = selection, .work = work, .action = action, .unit = unit};
    state.seen = p4pc_flip_count();
}

static int panel_input(P4ModCall *call, void *user)
{
    const Observer *observer = user;
    const uint32_t selection = (uint32_t)call->args[0].u, work = (uint32_t)call->args[1].u;
    if (!selection || !work || p4_battle_sim_active) return 0;
    record(observer->panel, selection, work);
    if (observer->panel == P4_BATTLE_PANEL_COMMANDS && state.chooseSelection == selection && state.chooseCommand >= 0) {
        P4_G16(selection + SEL_SELECTED) = (uint16_t)state.chooseCommand;
        state.confirmSelection = selection;
        state.chooseSelection = 0;
        state.chooseCommand = -1;
    }
    return 0;
}

/* btlSelectionReadInput: READ_CONFIRM is the confirm its callers act on. A
 * confirm of the waiting panel ends a wait; one a driver influenced records
 * that driver's use. */
static int read_input(P4ModCall *call, void *user)
{
    (void)user;
    const uint32_t selection = (uint32_t)call->args[0].u;
    if (state.confirmSelection && selection == state.confirmSelection) {
        call->result->i = READ_CONFIRM;
        state.confirmSelection = 0;
    }
    if (call->result->i == READ_CONFIRM && selection == state.menu.selection && state.owner) {
        if (state.influenced && state.use) state.use(state.owner);
        state.influenced = 0;
    }
    return 0;
}

/* After the game picks the default target of a selection, move it to the
 * aimed unit when that unit is one of the candidates. */
static int default_target(P4ModCall *call, void *user)
{
    (void)user;
    const uint32_t action = (uint32_t)call->args[0].u, targets = (uint32_t)call->args[1].u;
    if (!state.aimUnit || action != state.aimAction || targets != action + TARGETS) return 0;
    const uint32_t count = P4_G16(targets + TARGET_COUNT);
    for (uint32_t i = 0; i < count && i < 12u; ++i) {
        const uint32_t entry = P4_G32(targets + i * 4u);
        if (entry && P4_G32(entry + ACTION_UNIT) == state.aimUnit) {
            P4_G16(targets + TARGET_SELECTED) = (uint16_t)i;
            break;
        }
    }
    /* Both default pickers run for most skills; the second keeps the aim too. */
    if (call->function->address == 0x001d8e50u) state.aimAction = state.aimUnit = 0;
    return 0;
}

static Observer observers[] = {
    {0x00207320u, "ipip", P4_MOD_BEFORE, P4_BATTLE_PANEL_COMMANDS, NULL}, /* btlSelectionHandleInput */
    {0x00208b00u, "Hpp", P4_MOD_BEFORE, P4_BATTLE_PANEL_SKILLS, NULL},   /* btlSelectionHandleSkillInput */
    {0x002091f0u, "Hpp", P4_MOD_BEFORE, P4_BATTLE_PANEL_ITEMS, NULL},    /* btlSelectionHandleItemInput */
    {0x00209870u, "Hpip", P4_MOD_BEFORE, P4_BATTLE_PANEL_TACTICS, NULL}, /* btlSelectionOpenSubList */
    {0x0020a640u, "Hpp", P4_MOD_BEFORE, P4_BATTLE_PANEL_PERSONA, NULL},  /* btlSelectionHandleSwapInput */
    {0x00202e70u, "ip", P4_MOD_AFTER, 0, NULL},                          /* btlSelectionReadInput */
    {0x001d8cb0u, "ipp", P4_MOD_AFTER, -1, NULL},                         /* btlEffectSelectNearestEntry */
    {0x001d8e50u, "vpp", P4_MOD_AFTER, -1, NULL},                         /* btlEffectSelectTargetByStatus */
};

/* No hook can be added while a hook dispatch is active, and a first use may
 * come from inside one: a hook callback, or a frame_start subscriber once
 * p4.clock.updates observes kwlnRunFrame, whose dispatch then encloses every
 * frame boundary. So the observers are created disabled while the first mod VM
 * initializes, and the first use only enables them. */
void p4_battle_menu_prepare(void)
{
    if (state.installed) return;
    state.installed = -1;
    for (size_t i = 0; i < sizeof observers / sizeof observers[0]; ++i) {
        Observer *o = &observers[i];
        P4ModFunction *function = p4_mod_function_find(o->address, o->signature);
        P4ModHookFn callback = o->panel > 0 ? panel_input : o->panel == 0 ? read_input : default_target;
        o->hook = function ? p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, o->phase, callback, o) : NULL;
        if (!o->hook) return;
        p4_mod_hook_enable(o->hook, 0);
    }
    state.installed = 1;
}

static int install(void)
{
    if (state.installed <= 0) return -1;
    if (!state.active) {
        for (size_t i = 0; i < sizeof observers / sizeof observers[0]; ++i) p4_mod_hook_enable(observers[i].hook, 1);
        state.active = 1;
    }
    return 0;
}

int p4_battle_menu(P4BattleMenu *menu)
{
    if (install() || !waiting()) return -1;
    *menu = state.menu;
    const uint32_t s = state.menu.selection;
    menu->first = (int16_t)P4_G16(s + SEL_FIRST);
    menu->cursor = (int16_t)P4_G16(s + SEL_SELECTED);
    menu->page = (int16_t)P4_G16(s + SEL_PAGE);
    menu->count = (int16_t)P4_G16(s + SEL_COUNT);
    return 0;
}

/* The first accepted request makes owner this wait's driver. */
static int drive(const void *owner, const void **driver)
{
    if (install() || !waiting() || !owner) return -1;
    if (state.owner && state.owner != owner) {
        if (driver) *driver = state.owner;
        return -2;
    }
    return 0;
}

int p4_battle_menu_choose(const void *owner, int command, const void **driver)
{
    int result = drive(owner, driver);
    if (result) return result;
    if (state.menu.panel != P4_BATTLE_PANEL_COMMANDS || command < 0 || command >= P4_BATTLE_COMMANDS ||
        state.confirmSelection)
        return -1;
    state.owner = owner;
    state.influenced = 1;
    state.chooseSelection = state.menu.selection;
    state.chooseCommand = command;
    return 0;
}

int p4_battle_menu_point(const void *owner, int row, const void **driver)
{
    int result = drive(owner, driver);
    if (result) return result;
    if (state.menu.panel == P4_BATTLE_PANEL_COMMANDS) return -1;
    const uint32_t s = state.menu.selection;
    const int count = (int16_t)P4_G16(s + SEL_COUNT), page = (int16_t)P4_G16(s + SEL_PAGE);
    int first = (int16_t)P4_G16(s + SEL_FIRST);
    if (row < 0 || row >= count || page <= 0) return -1;
    state.owner = owner;
    if ((int16_t)P4_G16(s + SEL_SELECTED) == row) return 0; /* nothing changes, nothing is influenced */
    if (row < first) first = row;
    else if (row >= first + page) first = row - page + 1;
    P4_G16(s + SEL_FIRST) = (uint16_t)first;
    P4_G16(s + SEL_SELECTED) = (uint16_t)row;
    state.influenced = 1;
    return 0;
}

int p4_battle_menu_aim(const void *owner, uint32_t unit, const void **driver)
{
    int result = drive(owner, driver);
    if (result) return result;
    if (!state.menu.action || !unit) return -1;
    state.owner = owner;
    state.influenced = 1;
    state.aimAction = state.menu.action;
    state.aimUnit = unit;
    return 0;
}

void p4_battle_menu_on_use(void (*use)(const void *owner))
{
    state.use = use;
}

void p4_battle_menu_release(const void *owner)
{
    if (state.owner == owner) {
        end_wait();
        state.confirmSelection = 0;
        state.aimAction = state.aimUnit = 0;
    }
}
