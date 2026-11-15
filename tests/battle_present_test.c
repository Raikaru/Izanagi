/* Mod banners and message lines in battle (src/platform/mods/battle_present.c)
 * over synthetic guest battle records: queueing and its refusals, which unit
 * and moment take an item, the EVENT poll's waits on retail's own banner box,
 * boss talk and message window, the calls it makes and the MSG1 table it
 * builds, ordering, timeouts and owner removal. The panel and window natives
 * are recording stubs; the in-game witness qualifies them. */
#include "guest_test.h"
#include "p4pc/battle_present.h"
#include "p4pc/mod_entry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t battle, actions[3], units[3], ui_task, hud;
static uint32_t enemy_names, party_name;
static int boss_talk;
static struct { int starts, marks; int32_t task, text, countdown; char shown[64]; } panel;
static struct { int opens, shows, bound, steps, releases, resets, open_state, busy; uint32_t table; int32_t flag_handle, flags;
                char slot[2][64]; } window;

void btlPanelStartEntry(int32_t task, int32_t text)
{
    ++panel.starts;
    panel.task = task;
    panel.text = text;
    strcpy(panel.shown, P4_GPTR(char, (uint32_t)text));
    P4_G16(hud + 0x860) = 1;
    P4_G16(hud + 0x85E) = 0;
    P4_G32(hud + 0x870) = (uint32_t)text;
}
static int releases;
void btlPanelReleaseText(int32_t task) { (void)task; ++releases; P4_G16(hud + 0x860) = 0; }
uint32_t p4_mod_safe_point;
static int (*present_pump)(uint32_t frame);
void p4_mod_events_set_present(int (*pump)(uint32_t frame)) { present_pump = pump; }
void btlPanelSetCountdown(int32_t task, int32_t duration) { (void)task; panel.countdown = duration; }
void btlPanelMarkEntryStarted(int32_t task) { (void)task; ++panel.marks; }
int32_t btlTaskSlotIsSet(void) { return boss_talk; }
static uint32_t set_action, set_state;
void btlActionSetState(void *action, uint16_t state)
{
    set_action = P4_H2G(action);
    set_state = state;
    P4_G16(set_action + 0x10) = P4_G16(set_action + 0x0C);
    P4_G16(set_action + 0x0C) = state;
}
uint32_t datPersonaNamePart1Alt(int16_t unitId) { assert(unitId == 2); return party_name; }
uint8_t nmMesWindowOpenCurrent(uint8_t *message)
{
    if (window.busy) return 0;
    ++window.opens;
    window.table = P4_H2G(message);
    P4_G32(0x00882F60) = 7;
    window.open_state = 1;
    window.bound = 0;
    return 1;
}
void nmMesWindowSetCurrentString(int32_t variable, void *text)
{
    assert(variable == 0 || variable == 1);
    assert(window.opens && !window.bound); /* the slots are set before the record binds */
    strcpy(window.slot[variable], (const char *)text);
}
int32_t nmMesWindowShowCurrentRecord(int32_t recordIndex) { assert(recordIndex == 0); window.bound = 1; return ++window.shows; }
void nmMesWindowStepCurrentRelease(void) { ++window.steps; }
int32_t nmMesWindowCurrentOpenState(void) { return window.open_state; }
uint32_t nmMesWindowReleaseCurrent(void) { return (uint32_t)++window.releases; }
int32_t nmMesWindowResetCurrent(void) { ++window.resets; P4_G32(0x00882F60) = 0xFFFFFFFFu; return 0; }
void itfMesWindowSetFlagBits(int32_t handle, int32_t flags) { window.flag_handle = handle; window.flags = flags; }

static uint32_t text(const char *s)
{
    const uint32_t at = P4_H2G(p4t_galloc(0x20, 16));
    strcpy(P4_GPTR(char, at), s);
    return at;
}

static void build(void)
{
    battle = P4_H2G(p4t_galloc(0xE00, 16));
    P4_G32(0x0076449C) = battle;
    P4_G32(battle + 0x0C) = 0x200;
    ui_task = P4_H2G(p4t_galloc(0x40, 16));
    hud = P4_H2G(p4t_galloc(0x900, 16));
    P4_G32(ui_task + 0x38) = hud;
    P4_G32(battle + 0xDD4) = ui_task;
    enemy_names = P4_H2G(p4t_galloc(0x15 * 0x300, 16));
    strcpy(P4_GPTR(char, enemy_names + 0x2B0 * 0x15), "Shadow Yukiko");
    P4_GDATA(uint32_t, P4_GPADDR(0xB444)) = enemy_names;
    party_name = text("Yosuke");
    for (unsigned i = 0; i < 3; ++i) {
        actions[i] = P4_H2G(p4t_galloc(0x458, 16));
        units[i] = P4_H2G(p4t_galloc(0xa70, 16));
        P4_G32(actions[i] + 0x30) = units[i];
        P4_G8(units[i] + 0xa2) = i == 2 ? 1 : 0;
        P4_G16(units[i] + 0xa4) = i == 2 ? 0x2B0 : (uint16_t)(i + 1);
        P4_G32(units[i] + 0xa8) = 100 + i;
        if (i) P4_G32(actions[i - 1] + 0x450) = actions[i];
    }
    P4_G32(battle + 0x174) = actions[0];
    P4_G32(0x00882F60) = 0xFFFFFFFFu;
}

static int queue(uint32_t owner, uint32_t uid, int kind, int moment, const char *line, const char *speaker, int frames)
{
    const P4PresentRequest request = {.uid = uid, .kind = kind, .moment = moment, .text = line, .speaker = speaker,
                                      .frames = frames};
    return p4_battle_present_queue(owner, "test.mod", &request);
}
static int queue_hold(uint32_t owner, uint32_t uid, int moment, const char *line, int frames)
{
    const P4PresentRequest request = {.uid = uid, .kind = P4_PRESENT_BANNER, .moment = moment, .text = line,
                                      .frames = frames, .hold = 1};
    return p4_battle_present_queue(owner, "test.mod", &request);
}
static int run_until_done(int limit)
{
    int frames = 0;
    while (p4_battle_present_poll()) {
        if (++frames > limit) return -1;
        if (panel.starts && P4_G16(hud + 0x860)) P4_G16(hud + 0x860) = 0; /* the box finishes on its own */
    }
    return frames;
}

static void refusals(void)
{
    assert(queue(1, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "", NULL, 60) == P4_PRESENT_BAD_TEXT);
    assert(queue(1, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "0123456789012345678901234567890123456789X", NULL, 60) ==
           P4_PRESENT_BAD_TEXT);
    assert(queue(1, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "two\nlines", NULL, 60) == P4_PRESENT_BAD_TEXT);
    assert(queue(1, 102, P4_PRESENT_SAY, P4_PRESENT_TURN_END, "line", "\x01", 0) == P4_PRESENT_BAD_TEXT);
    assert(queue(1, 999, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "who", NULL, 60) == P4_PRESENT_NO_UNIT);
    P4_G32(0x0076449C) = 0;
    assert(queue(1, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "no battle", NULL, 60) == P4_PRESENT_NO_BATTLE);
    P4_G32(0x0076449C) = battle;
    for (int i = 0; i < P4_PRESENT_PER_TURN; ++i)
        assert(queue(1, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "fill", NULL, 60) == P4_PRESENT_OK);
    assert(queue(1, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "one more", NULL, 60) == P4_PRESENT_FULL);
    assert(p4_battle_present_pending == P4_PRESENT_PER_TURN);
    p4_battle_present_remove_owner(1);
    assert(p4_battle_present_pending == 0);
    assert(!strcmp(p4_battle_present_reason(P4_PRESENT_NO_UNIT), "no unit of this battle has that uid"));
}

static void banners(void)
{
    /* Only the unit's own moment takes it; the others run retail. */
    assert(queue(1, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "SHADOW YUKIKO IS GATHERING POWER!", NULL, 40) == 0);
    assert(queue(2, 102, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "SECOND", NULL, 20) == 0);
    assert(!p4_battle_present_park(actions[1], P4_PRESENT_TURN_END));
    assert(!p4_battle_present_park(actions[2], P4_PRESENT_TURN_START));
    assert(set_state == 0);
    assert(p4_battle_present_park(actions[2], P4_PRESENT_TURN_END));
    assert(p4_battle_present_pending == 1);
    /* Parked the retail boss-talk way: the poll as EVENT callback, END to resume, EVENT now. */
    assert(set_action == actions[2] && set_state == 0x16 && P4_G16(actions[2] + 0x43C) == 0x21);
    assert(P4_FN_G2H(P4_G32(actions[2] + 0x440)) == (void *)p4_battle_present_poll);
    /* Retail's own banner, a boss talk and a hidden HUD hold it back. */
    P4_G16(hud + 0x860) = 1;
    assert(p4_battle_present_poll() == 1 && panel.starts == 0);
    P4_G16(hud + 0x860) = 0;
    boss_talk = 1;
    assert(p4_battle_present_poll() == 1 && panel.starts == 0);
    boss_talk = 0;
    P4_G32(battle + 0x0C) = 0;
    assert(p4_battle_present_poll() == 1 && panel.starts == 0);
    P4_G32(battle + 0x0C) = 0x200;
    assert(p4_battle_present_poll() == 1);
    assert(panel.starts == 1 && panel.marks == 1 && panel.task == (int32_t)ui_task && panel.countdown == 40);
    assert(!strcmp(panel.shown, "SHADOW YUKIKO IS GATHERING POWER!"));
    /* Held for its frames plus the fade, then the next one in queue order, then retail resumes. */
    const int frames = run_until_done(1000);
    assert(frames == (40 + 8 - 1) + 1 + 1 + (20 + 8 - 1));
    assert(panel.starts == 2 && !strcmp(panel.shown, "SECOND") && panel.countdown == 20);
    assert(p4_battle_present_pending == 0 && !p4_battle_present_park(actions[2], P4_PRESENT_TURN_END));
}

static void timeout_and_removal(void)
{
    assert(queue(3, 100, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "never shown", NULL, 60) == 0);
    assert(queue(4, 100, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "shown", NULL, 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_START) && P4_G16(actions[0] + 0x43C) == 2);
    P4_G16(hud + 0x860) = 1;
    const int starts = panel.starts;
    for (int i = 0; i < P4_PRESENT_WAIT; ++i) assert(p4_battle_present_poll() == 1); /* the last drops the first */
    assert(panel.starts == starts);
    P4_G16(hud + 0x860) = 0;
    assert(p4_battle_present_poll() == 1 && panel.starts == starts + 1 && !strcmp(panel.shown, "shown"));
    assert(run_until_done(100) > 0);
    /* A mod going away takes its waiting items, not one already on screen. */
    assert(queue(5, 101, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "kept", NULL, 16) == 0);
    assert(queue(6, 101, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "removed", NULL, 16) == 0);
    p4_battle_present_remove_owner(6);
    assert(p4_battle_present_park(actions[1], P4_PRESENT_TURN_START));
    assert(p4_battle_present_poll() == 1 && !strcmp(panel.shown, "kept"));
    p4_battle_present_remove_owner(5);
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 0);
    /* No battle HUD task: nothing can be drawn and the actor goes on. */
    assert(queue(1, 100, P4_PRESENT_BANNER, P4_PRESENT_TURN_END, "unseen", NULL, 16) == 0);
    P4_G32(battle + 0xDD4) = 0;
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_END) && p4_battle_present_poll() == 0);
    P4_G32(battle + 0xDD4) = ui_task;
}

static void lines(void)
{
    /* The enemy speaks under its own name and waits for the confirm button. */
    assert(queue(1, 102, P4_PRESENT_SAY, P4_PRESENT_TURN_START, "You'll burn!", NULL, 0) == 0);
    assert(p4_battle_present_park(actions[2], P4_PRESENT_TURN_START));
    P4_G32(0x00882F60) = 3; /* the window is someone else's */
    assert(p4_battle_present_poll() == 1 && window.opens == 0);
    P4_G32(0x00882F60) = 0xFFFFFFFFu;
    assert(p4_battle_present_poll() == 1 && window.opens == 1 && window.shows == 1 && window.flags == 0);
    assert(!strcmp(window.slot[0], "Shadow Yukiko") && !strcmp(window.slot[1], "You'll burn!"));
    const uint32_t t = window.table;
    assert(P4_G32(t) == 7 && !memcmp(P4_GPTR(char, t + 8), "MSG1", 4) && P4_G32(t + 0x18) == 1);
    assert(P4_G16(t + 0x1C) == 1 && P4_G16(t + 0x1E) == 2 && P4_G32(t + 0x24) == t + 0x34);
    assert(P4_G16(t + 0x4C) == 1 && P4_G16(t + 0x4E) == 0x8000 && P4_G32(t + 0x50) == t + 0x58 && P4_G32(t + 0x54) == 14);
    static const uint8_t page[14] = {0xF2, 0x05, 0xFF, 0xFF, 0xF1, 0x41, 0xF2, 0x44, 0x02, 0x01, 0x0A, 0xF1, 0x21, 0x00};
    assert(!memcmp(P4_GPTR(uint8_t, t + 0x58), page, sizeof page) && P4_G32(t + 4) == 0x58 + 14);
    for (int i = 0; i < 30; ++i) assert(p4_battle_present_poll() == 1);
    assert(window.steps == 30 && window.releases == 0);
    window.open_state = 0; /* the player pressed confirm */
    assert(p4_battle_present_poll() == 0 && window.releases == 1 && window.resets == 1);
    /* A party member under a given name, timed: input is ignored and the poll closes it. */
    assert(queue(1, 101, P4_PRESENT_SAY, P4_PRESENT_TURN_END, "Partner!", "Yosuke?", 16) == 0);
    assert(queue(1, 101, P4_PRESENT_SAY, P4_PRESENT_TURN_END, "Own name", NULL, 16) == 0);
    assert(p4_battle_present_park(actions[1], P4_PRESENT_TURN_END));
    assert(p4_battle_present_poll() == 1 && !strcmp(window.slot[0], "Yosuke?"));
    assert(window.flag_handle == 7 && window.flags == 0x4000000);
    int frames = 0;
    while (p4_battle_present_poll()) ++frames;
    assert(window.opens == 3 && !strcmp(window.slot[0], "Yosuke") && !strcmp(window.slot[1], "Own name"));
    assert(window.resets == 3 && frames == 15 + 1 + 1 + 15);
}

static void holding(void)
{
    assert(p4_battle_present_pending == 0 && !present_pump);
    P4_G16(hud + 0x860) = 0;
    assert(queue_hold(1, 102, P4_PRESENT_TURN_END, "GATHERING POWER", 20) == 0);
    assert(p4_battle_present_park(actions[2], P4_PRESENT_TURN_END));
    assert(p4_battle_present_poll() == 1 && !strcmp(panel.shown, "GATHERING POWER") && panel.countdown == 20);
    assert(present_pump && (p4_mod_safe_point & P4_MOD_SAFE_PRESENT) && p4_battle_present_pending == 1);
    /* While the turn-end banner itself shows, the pump leaves the box alone. */
    const int starts = panel.starts;
    assert(present_pump(1) == 1 && panel.starts == starts);
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 1);
    /* Idle box: raised and held (no countdown) until something replaces or ends it. */
    assert(present_pump(2) == 1 && panel.starts == starts + 1 && panel.countdown == -1);
    assert(!strcmp(panel.shown, "GATHERING POWER"));
    /* The game's own line replaces it; once that line is gone the banner returns. */
    P4_G32(hud + 0x870) = 0x00123400u;
    assert(present_pump(3) == 1 && panel.starts == starts + 1);
    P4_G16(hud + 0x860) = 0;
    assert(present_pump(4) == 1 && panel.starts == starts + 2);
    /* Another unit's queued banner takes the box over it (the held one counts as idle). */
    assert(queue(2, 101, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "OTHER", NULL, 16) == 0);
    assert(p4_battle_present_park(actions[1], P4_PRESENT_TURN_START));
    assert(p4_battle_present_poll() == 1 && !strcmp(panel.shown, "OTHER"));
    assert(run_until_done(100) > 0);
    assert(present_pump(5) == 1 && !strcmp(panel.shown, "GATHERING POWER"));
    /* The unit's next turn start ends it and fades the box; nothing else waits, so the turn goes on. */
    const int released = releases;
    assert(!p4_battle_present_park(actions[2], P4_PRESENT_TURN_START));
    assert(releases == released + 1 && p4_battle_present_pending == 0 && present_pump(6) == 0);
    /* announce_clear: only the owner's, for that unit. */
    assert(queue_hold(1, 100, P4_PRESENT_TURN_END, "HELD", 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_END) && p4_battle_present_poll() == 1);
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 1);
    assert(p4_battle_present_clear(9, 0) == 0 && p4_battle_present_clear(1, 101) == 0);
    assert(p4_battle_present_clear(1, 100) == 1 && p4_battle_present_pending == 0);
    /* A held banner of a unit that left the battle ends at the next frame. */
    assert(queue_hold(1, 100, P4_PRESENT_TURN_END, "GONE", 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_END) && p4_battle_present_poll() == 1);
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 1);
    P4_G32(units[0] + 0xa8) = 999;
    assert(present_pump(7) == 0 && p4_battle_present_pending == 0);
    P4_G32(units[0] + 0xa8) = 100;
}
static void enter_start(uint32_t action, uint16_t previous)
{
    P4_G16(action + 0x10) = previous;
    P4_G16(action + 0x0C) = 2;
}


static void start_hold_reentry(void)
{
    assert(p4_battle_present_pending == 0);
    P4_G16(hud + 0x860) = 0;
    /* The normal EVENT -> START path must retain a newly created hold. */
    assert(queue_hold(9, 100, P4_PRESENT_TURN_START, "START HOLD", 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(P4_G16(actions[0] + 0x43C) == 2);
    assert(p4_battle_present_poll() == 1 && !strcmp(panel.shown, "START HOLD"));
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 1);
    enter_start(actions[0], 0x16); /* EventUpdate calls SetState(START) synchronously. */
    assert(!p4_battle_present_park(actions[0], P4_PRESENT_TURN_START)); /* EVENT -> START */
    assert(p4_battle_present_pending == 1);
    assert(present_pump(8) == 1 && !strcmp(panel.shown, "START HOLD") && panel.countdown == -1);
    assert(!p4_battle_present_park(actions[2], P4_PRESENT_TURN_START));
    assert(p4_battle_present_pending == 1);
    enter_start(actions[0], 0x21);
    assert(!p4_battle_present_park(actions[0], P4_PRESENT_TURN_START)); /* next real turn */
    assert(p4_battle_present_pending == 0 && present_pump(9) == 0);
    /* An older turn-end hold expires at its owner's next real START, not
     * another actor's, while the two new START holds survive EVENT resumption. */
    assert(queue_hold(10, 101, P4_PRESENT_TURN_END, "OTHER HOLD", 16) == 0);
    assert(p4_battle_present_park(actions[1], P4_PRESENT_TURN_END));
    assert(run_until_done(100) > 0);
    assert(queue_hold(11, 100, P4_PRESENT_TURN_END, "OLD HOLD", 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_END));
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 2);
    assert(queue_hold(12, 100, P4_PRESENT_TURN_START, "FIRST START HOLD", 16) == 0);
    assert(queue_hold(13, 100, P4_PRESENT_TURN_START, "SECOND START HOLD", 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(set_action == actions[0] && set_state == 0x16 && P4_G16(actions[0] + 0x43C) == 2);
    assert(p4_battle_present_clear(11, 100) == 0); /* the old hold ended */
    assert(p4_battle_present_poll() == 1 && !strcmp(panel.shown, "FIRST START HOLD"));
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 3);
    assert(present_pump(8) == 1 && !strcmp(panel.shown, "SECOND START HOLD") && panel.countdown == -1);

    /* A late item makes the resumed START park a second time. Neither
     * re-entry may mistake the current turn for the next real turn. */
    assert(queue(14, 100, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "LATE", NULL, 16) == 0);
    enter_start(actions[0], 0x16);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(p4_battle_present_pending == 3);
    assert(p4_battle_present_poll() == 1 && !strcmp(panel.shown, "LATE"));
    assert(run_until_done(100) > 0);
    enter_start(actions[0], 0x16);
    assert(!p4_battle_present_park(actions[0], P4_PRESENT_TURN_START)); /* EVENT -> START */
    assert(p4_battle_present_pending == 3);
    assert(!p4_battle_present_park(actions[2], P4_PRESENT_TURN_START));
    assert(present_pump(9) == 1 && !strcmp(panel.shown, "SECOND START HOLD"));
    assert(p4_battle_present_clear(12, 100) == 1 && p4_battle_present_pending == 2);
    assert(present_pump(10) == 1 && !strcmp(panel.shown, "SECOND START HOLD"));
    /* The next true START ends the remaining own hold, not the other actor's. */
    enter_start(actions[0], 0x21);
    assert(!p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(p4_battle_present_pending == 1);
    assert(present_pump(11) == 1 && !strcmp(panel.shown, "OTHER HOLD"));
    assert(p4_battle_present_clear(10, 101) == 1 && p4_battle_present_pending == 0);
    assert(present_pump(12) == 0);
}
static void interrupted_start_hold(void)
{
    P4_G16(hud + 0x860) = 0;
    enter_start(actions[0], 0x21);
    assert(queue_hold(20, 100, P4_PRESENT_TURN_START, "INTERRUPTED", 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(p4_battle_present_poll() == 1 && p4_battle_present_pending == 1);
    /* The event is redirected before its poll completes; it cannot resume START. */
    btlActionSetState(P4_G2H(actions[0]), 0x18);
    assert(present_pump(13) == 1);
    enter_start(actions[0], 0x18);
    assert(!p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(p4_battle_present_pending == 0 && p4_battle_present_poll() == 0);

    /* The poll can complete but retail's START init can take an early exit
     * before reaching our hook. A later START is not the original re-entry. */
    P4_G16(hud + 0x860) = 0;
    assert(queue_hold(21, 100, P4_PRESENT_TURN_START, "EARLY EXIT", 16) == 0);
    assert(p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(p4_battle_present_poll() == 1);
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 1);
    enter_start(actions[0], 0x16);
    btlActionSetState(P4_G2H(actions[0]), 0x18); /* START exited before park */
    enter_start(actions[0], 0x18);
    assert(!p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    assert(p4_battle_present_pending == 0 && present_pump(14) == 0);
}


/* Exercise the actual turn-start owner with the real presentation queue.
 * Its host-only model lives after the guest action record, so the presenter
 * reads its ordinary 0x30/0x43c/0x440/0x450 guest fields while the included
 * native owner uses typed fixture fields without assuming host pointer size. */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef struct BtlUnit { u32 flags3; u8 genus; u16 charId; void *datUnit; } BtlUnit;
typedef struct BtlAction {
    u8 guest[0x458];
    BtlUnit *unit;
    struct BtlAction *next;
    u32 flags;
    u16 unk_1a;
    s32 unk_28, unk_29, unk_430, unk_43c, unk_41c;
    void *unk_440;
    int targetBlock;
    u16 state;
} BtlAction;
typedef struct BtlBattleState {
    u8 *actions, *selectedAction, *uiTask;
    s32 unkA70;
    u32 unkA72, flags, battleFlags, protagonistTurns;
} BtlBattleState;
static BtlBattleState turn_battle;
u8 *D_0076449C = (u8 *)&turn_battle;
u8 *iGpffffb3ac = (u8 *)&turn_battle;
static BtlAction *turn_actor;
static int turn_boss_window;
u32 datCalcIsDead(void *unit, s32 hp) { (void)unit; (void)hp; return 0; }
void btlUnitSelectRecordTable(u8 *action) { (void)action; }
void btlTargetClearWork(void *work) { (void)work; }
s32 btlStatusWindowShowSlot2(u8 *action) { (void)action; return turn_boss_window; }
s32 btlTaskSlotLinkedSlot2(void) { return 0; }
s32 btlSideCountNotBad(s32 side, s32 status) { (void)side; (void)status; return 1; }
s16 btlCameraSelectFormationIndex(void) { return 0; }
u32 datCalcChkBadStatus(void *unit, u32 status) { (void)unit; (void)status; return 0; }
void btlActionSetStateWithDelay(BtlAction *action, u16 state, u16 delay)
{ (void)delay; btlActionSetState(action, state); }
s32 btlTargetCanControlDamagePhase(void) { return 1; }
void btlTargetEndDamagePhase(void) {}
void btlDamageStart(u8 *task, u8 *action) { (void)task; (void)action; }
u8 *func_001f99c0(u8 *action, u16 code, s32 a, s32 b, s32 c)
{ (void)action; (void)code; (void)a; (void)b; (void)c; return NULL; }
s64 btlPacketEnqueue(u8 *packet, u32 flags) { (void)packet; (void)flags; return 0; }
s32 datCalcGetSkillMask(u8 *unit) { (void)unit; return 0; }
s32 datCalcIsGuarding(u16 *unit) { (void)unit; return 0; }
s64 datCalcFindStrongBuff(u8 *unit) { (void)unit; return -1; }
s32 btlSoundRollStatusCure(u8 *action) { (void)action; return 0; }
u8 *btlEffectFindUnitByIdAndStatus(s32 id, s32 side, s32 status)
{
    (void)status;
    return side == 1 && turn_actor && id == turn_actor->unit->charId
        && (turn_actor->unk_1a & 9) == 9 ? (u8 *)turn_actor : NULL;
}
#include "../overrides/001a06d0.c"

static void owner_start_presentation(void)
{
    BtlUnit fighter = {0, 0, 2, NULL};
    uint32_t actor;
    int starts;

    assert(p4_battle_present_pending == 0);
    turn_actor = p4t_galloc((uint32_t)sizeof(*turn_actor), 16);
    memset(turn_actor, 0, sizeof(*turn_actor));
    turn_actor->unit = &fighter;
    turn_actor->unk_1a = 9;
    turn_actor->state = 2;
    actor = P4_H2G(turn_actor);
    P4_G32(actor + 0x30) = units[1];
    P4_G32(actor + 0x450) = actions[0];
    P4_G32(battle + 0x174) = actor;
    P4_G16(hud + 0x860) = 0;
    memset(&turn_battle, 0, sizeof(turn_battle));
    turn_battle.actions = (u8 *)turn_actor;
    turn_battle.selectedAction = (u8 *)turn_actor;
    turn_battle.unkA70 = -1;

    /* With no item, or only another actor's item, START is not parked. */
    enter_start(actor, 0x21);
    btlActionStartInit(turn_actor);
    assert(turn_battle.protagonistTurns == 1);
    assert(queue(29, 100, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "OTHER", NULL, 16) == 0);
    enter_start(actor, 0x21);
    btlActionStartInit(turn_actor);
    assert(turn_battle.protagonistTurns == 2 && p4_battle_present_pending == 1);
    p4_battle_present_remove_owner(29);
    assert(p4_battle_present_pending == 0);
    turn_battle.protagonistTurns = 0;

    /* A boss talk takes priority and leaves the queued banner untouched. */
    assert(queue(30, 101, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "OWNER", NULL, 16) == 0);
    enter_start(actor, 0x21);
    turn_boss_window = 1;
    btlActionStartInit(turn_actor);
    assert(P4_G16(actor + 0x0c) == 0x16 && p4_battle_present_pending == 1);
    assert(turn_battle.protagonistTurns == 0);

    /* Retry after boss talk: the real queue parks before the counter. */
    turn_boss_window = 0;
    enter_start(actor, 0x16);
    starts = panel.starts;
    btlActionStartInit(turn_actor);
    assert(P4_G16(actor + 0x0c) == 0x16 && P4_G16(actor + 0x43c) == 2);
    assert(P4_FN_G2H(P4_G32(actor + 0x440)) == (void *)p4_battle_present_poll);
    assert(turn_battle.protagonistTurns == 0);
    assert(p4_battle_present_poll() == 1 && panel.starts == starts + 1);
    assert(run_until_done(100) > 0);

    /* A late queued item parks the same START again, still before counting. */
    assert(queue(31, 101, P4_PRESENT_BANNER, P4_PRESENT_TURN_START, "LATE", NULL, 16) == 0);
    enter_start(actor, 0x16);
    btlActionStartInit(turn_actor);
    assert(P4_G16(actor + 0x0c) == 0x16 && turn_battle.protagonistTurns == 0);
    assert(p4_battle_present_poll() == 1);
    assert(run_until_done(100) > 0 && p4_battle_present_pending == 0);
    enter_start(actor, 0x16);
    btlActionStartInit(turn_actor);
    assert(turn_battle.protagonistTurns == 1);
    P4_G32(battle + 0x174) = actions[0];
    turn_actor = NULL;
}

int main(void)
{
    if (p4t_guest_map(0x00764000u, 0x1000u) || p4t_guest_map(0x00882000u, 0x1000u)) return 77;
    build();
    assert(p4_battle_present_poll() == 0 && !p4_battle_present_park(actions[0], P4_PRESENT_TURN_START));
    refusals();
    banners();
    timeout_and_removal();
    lines();
    holding();
    start_hold_reentry();
    interrupted_start_hold();
    owner_start_presentation();
    puts("battle_present: queue, refusals, turn moments, waits, banners, lines, removal and held banners passed");
    return 0;
}
