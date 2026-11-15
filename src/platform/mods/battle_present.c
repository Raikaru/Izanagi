/* Mod banners and message lines in battle (include/p4pc/battle_present.h).
 * The queue is port state. The guest effects are the retail calls the
 * skill-name banner and the revival prompt make, from the EVENT poll of the
 * acting unit: btlPanelStartEntry/MarkEntryStarted/SetCountdown for a banner;
 * nmMesWindowOpenCurrent, SetCurrentString, ShowCurrentRecord, the per-frame
 * StepCurrentRelease and ReleaseCurrent/ResetCurrent for a line, over a
 * one-record MSG1 table (api-research §1.7). Text, table and slot strings live
 * in port-owned guest buffers allocated on first use and reused. */
#include "p4pc/battle_present.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_events.h"
#include "p4pc/native_glyphs.h"
#include <stdio.h>
#include <string.h>

extern void btlPanelStartEntry(int32_t task, int32_t text);
extern void btlPanelSetCountdown(int32_t task, int32_t duration);
extern void btlPanelMarkEntryStarted(int32_t task);
extern void btlPanelReleaseText(int32_t task);
extern int32_t btlTaskSlotIsSet(void);
extern void btlActionSetState(void *action, uint16_t state);
extern uint32_t datPersonaNamePart1Alt(int16_t unitId);
extern uint8_t nmMesWindowOpenCurrent(uint8_t *message);
extern void nmMesWindowSetCurrentString(int32_t variable, void *text);
extern int32_t nmMesWindowShowCurrentRecord(int32_t recordIndex);
extern void nmMesWindowStepCurrentRelease(void);
extern int32_t nmMesWindowCurrentOpenState(void);
extern uint32_t nmMesWindowReleaseCurrent(void);
extern int32_t nmMesWindowResetCurrent(void);
extern void itfMesWindowSetFlagBits(int32_t handle, int32_t flags);

/* BtlBattleState (pointer at 0x0076449C), BtlAction and BtlUnit fields. */
#define BATTLE_STATE 0x0076449Cu
#define BATTLE_FLAGS 0x0Cu         /* 0x200: the battle HUD shows names (retail gates its skill banner on it) */
#define BATTLE_ACTIONS 0x174u
#define BATTLE_UI_TASK 0xDD4u
#define ACTION_STATE 0x0Cu
#define ACTION_OLD_STATE 0x10u
#define ACTION_NEXT 0x450u
#define ACTION_UNIT 0x30u
#define ACTION_EVENT_RESUME 0x43Cu    /* u16: the state EVENT resumes */
#define ACTION_EVENT_CALLBACK 0x440u  /* the EVENT poll, s32 (*)(void) */
#define STATE_START 0x02
#define STATE_EVENT 0x16
#define STATE_END 0x21
#define UNIT_GENUS 0xA2u           /* u8: 0 party, else enemy */
#define UNIT_ID 0xA4u
#define UNIT_UID 0xA8u
#define TASK_WORK 0x38u            /* KwlnTask.work: the HUD work of the UI task */
#define HUD_PANEL_STATE 0x860u     /* BtlMessagePanel (HUD +0x84C) .state: 0 = idle */
#define HUD_PANEL_MODE 0x85Eu      /* .mode: 0 = raw text */
#define HUD_PANEL_TEXT 0x870u      /* .text: the raw text's guest address */
#define PANEL_HOLD (-1)            /* btlPanelSetCountdown: hold until released or replaced */
#define FADE_FRAMES 8              /* the panel's fade after its countdown */
#define ENEMY_NAMES 0xB444         /* gp word: the enemy name bank, 0x15 bytes a name */
#define WINDOW_CURRENT 0x00882F60u /* nmMesWindowCurrent: handle (s32) at +0, -1 when free */
#define WINDOW_IGNORE_INPUT 0x4000000

typedef struct Item {
    uint32_t owner, uid, order;
    const char *name;
    uint8_t kind, moment, used, hold;
    int16_t frames;
    char text[P4_PRESENT_TEXT_MAX + 1];
    char speaker[P4_PRESENT_SPEAKER_MAX + 1];
} Item;

int p4_battle_present_pending;
static Item items[P4_PRESENT_QUEUE];
static uint32_t next_order;
/* Held banners: the newest is the one raised. */
static struct {
    uint32_t owner, uid, order, start_action;
    uint8_t used, skip_start_reentry;
    char text[P4_PRESENT_TEXT_MAX + 1];
} held[P4_PRESENT_HELD];
static struct {
    uint32_t text, held, table, speaker, line;
} guest;
static struct {
    int active, showing, left;
    uint32_t uid, unit, action;
    uint8_t moment;
    Item item;
} current;

static uint32_t battle_state(void) { return P4_G32(BATTLE_STATE); }

static uint32_t find_unit(uint32_t battle, uint32_t uid)
{
    for (uint32_t action = P4_G32(battle + BATTLE_ACTIONS), guard = 0; action && guard < 64;
         action = P4_G32(action + ACTION_NEXT), ++guard) {
        const uint32_t unit = P4_G32(action + ACTION_UNIT);
        if (unit && P4_G32(unit + UNIT_UID) == uid) return unit;
    }
    return 0;
}

static void drop(Item *item)
{
    item->used = 0;
    --p4_battle_present_pending;
}

const char *p4_battle_present_reason(int status)
{
    switch (status) {
    case P4_PRESENT_NO_BATTLE: return "no battle is running";
    case P4_PRESENT_NO_UNIT: return "no unit of this battle has that uid";
    case P4_PRESENT_FULL: return "that unit already has 4 items waiting for this moment";
    case P4_PRESENT_BAD_TEXT: return "the text is not one line of game characters of the allowed length";
    default: return "";
    }
}

static int valid(const char *text, size_t limit, P4NativeGlyphFlavor flavor)
{
    const size_t length = text ? strnlen(text, limit + 1) : 0;
    return length && length <= limit && !p4_native_glyphs_validate((const uint8_t *)text, length + 1, flavor, 0);
}

int p4_battle_present_queue(uint32_t owner, const char *name, const P4PresentRequest *request)
{
    const P4NativeGlyphFlavor flavor = request->kind == P4_PRESENT_SAY ? P4_GLYPHS_MESSAGE : P4_GLYPHS_RAW;
    if (!valid(request->text, P4_PRESENT_TEXT_MAX, flavor) ||
        (request->speaker && !valid(request->speaker, P4_PRESENT_SPEAKER_MAX, flavor)))
        return P4_PRESENT_BAD_TEXT;
    const uint32_t battle = battle_state();
    if (!battle) return P4_PRESENT_NO_BATTLE;
    if (!find_unit(battle, request->uid)) return P4_PRESENT_NO_UNIT;
    /* Items for units no longer in this battle (an earlier battle's) give their slots back. */
    int same = 0, free_slot = -1;
    for (int i = 0; i < P4_PRESENT_QUEUE; ++i) {
        if (items[i].used && !find_unit(battle, items[i].uid)) drop(&items[i]);
        if (!items[i].used) {
            if (free_slot < 0) free_slot = i;
        } else if (items[i].uid == request->uid && items[i].moment == request->moment) {
            ++same;
        }
    }
    if (same >= P4_PRESENT_PER_TURN || free_slot < 0) return P4_PRESENT_FULL;
    Item *item = &items[free_slot];
    *item = (Item){owner, request->uid, ++next_order, name, (uint8_t)request->kind, (uint8_t)request->moment, 1,
                   (uint8_t)(request->kind == P4_PRESENT_BANNER && request->hold), (int16_t)request->frames, {0}, {0}};
    strcpy(item->text, request->text);
    if (request->speaker) strcpy(item->speaker, request->speaker);
    ++p4_battle_present_pending;
    return P4_PRESENT_OK;
}

/* The oldest item for this unit and moment, or NULL. */
static Item *first(uint32_t uid, uint8_t moment)
{
    Item *found = NULL;
    for (int i = 0; i < P4_PRESENT_QUEUE; ++i)
        if (items[i].used && items[i].uid == uid && items[i].moment == moment &&
            (!found || items[i].order < found->order))
            found = &items[i];
    return found;
}

/* Moves the next waiting item into `current`; 0 when there is none. */
static int advance(void)
{
    Item *item = first(current.uid, current.moment);
    if (!item) {
        current.active = 0;
        return 0;
    }
    current.item = *item;
    drop(item);
    current.showing = 0;
    current.left = P4_PRESENT_WAIT;
    return 1;
}

static uint32_t allocated(uint32_t *slot, uint32_t size)
{
    if (!*slot) *slot = p4_gport_alloc(size, 16);
    return *slot;
}

/* The battle HUD work of the battle's UI task, or 0. */
static uint32_t hud_of(uint32_t battle, uint32_t *ui)
{
    *ui = P4_G32(battle + BATTLE_UI_TASK);
    return *ui ? P4_G32(*ui + TASK_WORK) : 0;
}

/* The box shows the held banner's buffer. */
static int held_on_screen(uint32_t hud)
{
    return guest.held && P4_G16(hud + HUD_PANEL_STATE) != 0 && P4_G16(hud + HUD_PANEL_MODE) == 0 &&
           P4_G32(hud + HUD_PANEL_TEXT) == guest.held;
}

static void unhold(int i)
{
    held[i].used = 0;
    --p4_battle_present_pending;
    /* Fade out the box when it shows the one ended; the pump raises the next held one once idle. */
    const uint32_t battle = battle_state();
    uint32_t ui;
    const uint32_t hud = battle ? hud_of(battle, &ui) : 0;
    if (hud && held_on_screen(hud)) btlPanelReleaseText((int32_t)ui);
}

static int present_frame(uint32_t frame);

/* A redirected EVENT will never finish our poll. Do not let that abandoned
 * poll block later turns or make its holds look like a completed re-entry. */
static void abandon_interrupted(void)
{
    if (!current.active) return;
    const uint32_t action = current.action;
    if (P4_G16(action + ACTION_STATE) == STATE_EVENT &&
        P4_G32(action + ACTION_EVENT_CALLBACK) == p4_fn_h2g_sig((const void *)p4_battle_present_poll, "i") &&
        P4_G16(action + ACTION_EVENT_RESUME) ==
            (current.moment == P4_PRESENT_TURN_START ? STATE_START : STATE_END))
        return;
    current.active = 0;
    for (int i = 0; i < P4_PRESENT_HELD; ++i)
        if (held[i].used && held[i].start_action == action) held[i].skip_start_reentry = 0;
}

static void hold(const Item *item)
{
    int slot = 0;
    for (int i = 0; i < P4_PRESENT_HELD; ++i) {
        if (!held[i].used) { slot = i; break; }
        if (held[i].order < held[slot].order) slot = i; /* full: the oldest gives way */
    }
    if (held[slot].used) unhold(slot);
    held[slot].owner = item->owner;
    held[slot].uid = item->uid;
    held[slot].order = item->order;
    held[slot].used = 1;
    held[slot].start_action = current.action;
    held[slot].skip_start_reentry = current.moment == P4_PRESENT_TURN_START;
    strcpy(held[slot].text, item->text);
    ++p4_battle_present_pending;
    p4_mod_events_set_present(present_frame);
    p4_mod_safe_point |= P4_MOD_SAFE_PRESENT;
}

/* The frame safe point while a banner is held: ends those whose unit left the battle and raises the newest
 * whenever the box is idle, held until something replaces or ends it. */
static int present_frame(uint32_t frame)
{
    (void)frame;
    abandon_interrupted();
    const uint32_t battle = battle_state();
    int top = -1;
    for (int i = 0; i < P4_PRESENT_HELD; ++i) {
        if (!held[i].used) continue;
        if (!battle || !find_unit(battle, held[i].uid)) {
            held[i].used = 0;
            --p4_battle_present_pending;
        } else if (top < 0 || held[i].order > held[top].order) {
            top = i;
        }
    }
    if (top < 0) return 0;
    uint32_t ui;
    const uint32_t hud = hud_of(battle, &ui);
    if (!hud || (current.active && current.showing) || !(P4_G32(battle + BATTLE_FLAGS) & 0x200u) ||
        P4_G16(hud + HUD_PANEL_STATE) != 0 || btlTaskSlotIsSet() || !allocated(&guest.held, P4_PRESENT_TEXT_MAX + 1))
        return 1;
    memcpy(P4_GPTR(char, guest.held), held[top].text, strlen(held[top].text) + 1);
    btlPanelStartEntry((int32_t)ui, (int32_t)guest.held);
    btlPanelMarkEntryStarted((int32_t)ui);
    btlPanelSetCountdown((int32_t)ui, PANEL_HOLD);
    return 1;
}

int p4_battle_present_clear(uint32_t owner, uint32_t uid)
{
    int ended = 0;
    for (int i = 0; i < P4_PRESENT_HELD; ++i)
        if (held[i].used && held[i].owner == owner && (!uid || held[i].uid == uid)) {
            unhold(i);
            ++ended;
        }
    return ended;
}

int p4_battle_present_park(uint32_t action, int moment)
{
    abandon_interrupted();
    if (current.active || !action) return 0;
    const uint32_t unit = P4_G32(action + ACTION_UNIT);
    if (!unit) return 0;
    const uint32_t uid = P4_G32(unit + UNIT_UID);
    /* START is re-entered after our EVENT poll. Holds made during that same
     * start must survive the re-entry, but expire at the next real start. */
    if (moment == P4_PRESENT_TURN_START)
        for (int i = 0; i < P4_PRESENT_HELD; ++i) {
            if (!held[i].used || held[i].uid != uid) continue;
            if (held[i].skip_start_reentry && held[i].start_action == action &&
                P4_G16(action + ACTION_STATE) == STATE_START &&
                P4_G16(action + ACTION_OLD_STATE) == STATE_EVENT &&
                P4_G16(action + ACTION_EVENT_RESUME) == STATE_START)
                held[i].skip_start_reentry = 0;
            else unhold(i);
        }
    current.action = action;
    current.unit = unit;
    current.uid = uid;
    current.moment = (uint8_t)moment;
    current.active = 1;
    if (!advance()) return 0;
    /* Further items may have been queued while EVENT ran. If this START parks
     * again, its next re-entry is still part of the same turn. */
    if (moment == P4_PRESENT_TURN_START)
        for (int i = 0; i < P4_PRESENT_HELD; ++i)
            if (held[i].used && held[i].uid == uid) {
                held[i].start_action = action;
                held[i].skip_start_reentry = 1;
            }
    /* The retail boss-talk order: callback, resume state, then EVENT. */
    P4_G32(action + ACTION_EVENT_CALLBACK) = p4_fn_h2g_sig((const void *)p4_battle_present_poll, "i");
    P4_G16(action + ACTION_EVENT_RESUME) = moment == P4_PRESENT_TURN_START ? STATE_START : STATE_END;
    btlActionSetState(P4_G2H(action), STATE_EVENT);
    return 1;
}

static int wait_or_drop(const char *what)
{
    if (--current.left > 0) return 1;
    fprintf(stderr, "p4pc: mod %s: %s: dropped \"%s\" (%s stayed busy for %d frames)\n", current.item.name,
            current.item.kind == P4_PRESENT_SAY ? "p4.battle.say" : "p4.battle.announce", current.item.text, what,
            P4_PRESENT_WAIT);
    return advance();
}

static int32_t banner(uint32_t battle)
{
    const uint32_t ui = P4_G32(battle + BATTLE_UI_TASK);
    const uint32_t hud = ui ? P4_G32(ui + TASK_WORK) : 0;
    if (!hud) return advance(); /* no battle HUD task: nothing can be drawn */
    if (!(P4_G32(battle + BATTLE_FLAGS) & 0x200u) || (P4_G16(hud + HUD_PANEL_STATE) != 0 && !held_on_screen(hud)) ||
        btlTaskSlotIsSet())
        return wait_or_drop("the banner box");
    if (!allocated(&guest.text, P4_PRESENT_TEXT_MAX + 1)) return advance();
    memcpy(P4_GPTR(char, guest.text), current.item.text, strlen(current.item.text) + 1);
    btlPanelStartEntry((int32_t)ui, (int32_t)guest.text);
    btlPanelMarkEntryStarted((int32_t)ui);
    btlPanelSetCountdown((int32_t)ui, current.item.frames);
    if (current.item.hold) hold(&current.item);
    current.showing = 1;
    current.left = current.item.frames + FADE_FRAMES;
    return 1;
}

/* The one-record MSG1 table of api-research §1.7: speaker = substitution slot 0,
 * one page printing slot 1 and waiting for the confirm input. */
static void write_table(uint32_t base)
{
    static const uint8_t page[14] = {0xF2, 0x05, 0xFF, 0xFF, 0xF1, 0x41, 0xF2, 0x44, 0x02, 0x01, 0x0A, 0xF1, 0x21, 0x00};
    uint8_t *b = P4_GPTR(uint8_t, base);
    memset(b, 0, 0x68);
    P4_G32(base + 0x00) = 7;
    P4_G32(base + 0x04) = 0x58 + sizeof page;
    memcpy(b + 0x08, "MSG1", 4);
    P4_G32(base + 0x18) = 1;                 /* one entry */
    P4_G16(base + 0x1C) = 1;                 /* relocated: every pointer is an absolute guest address */
    P4_G16(base + 0x1E) = 2;                 /* version */
    P4_G32(base + 0x24) = base + 0x34;       /* entry 0: a message dialog */
    memcpy(b + 0x34, "P4PC_SAY", 8);
    P4_G16(base + 0x4C) = 1;                 /* pages */
    P4_G16(base + 0x4E) = 0x8000;            /* speaker: substitution slot 0 */
    P4_G32(base + 0x50) = base + 0x58;
    P4_G32(base + 0x54) = sizeof page;
    memcpy(b + 0x58, page, sizeof page);
}

static int32_t say(void)
{
    if (!current.showing) {
        if ((int32_t)P4_G32(WINDOW_CURRENT) >= 0 || btlTaskSlotIsSet()) return wait_or_drop("the message window");
        if (!allocated(&guest.table, 0x68) || !allocated(&guest.speaker, 0x20) ||
            !allocated(&guest.line, P4_PRESENT_TEXT_MAX + 1))
            return advance();
        uint32_t speaker = guest.speaker;
        if (current.item.speaker[0]) {
            memcpy(P4_GPTR(char, guest.speaker), current.item.speaker, strlen(current.item.speaker) + 1);
        } else {
            const uint16_t id = P4_G16(current.unit + UNIT_ID);
            speaker = P4_G8(current.unit + UNIT_GENUS) == 0 ? datPersonaNamePart1Alt((int16_t)id)
                : P4_GDATA(uint32_t, P4_GPADDR(ENEMY_NAMES)) + id * 0x15u;
        }
        memcpy(P4_GPTR(char, guest.line), current.item.text, strlen(current.item.text) + 1);
        write_table(guest.table);
        if (!nmMesWindowOpenCurrent(P4_GPTR(uint8_t, guest.table))) return wait_or_drop("the message window");
        nmMesWindowSetCurrentString(0, P4_G2H(speaker));
        nmMesWindowSetCurrentString(1, P4_G2H(guest.line));
        nmMesWindowShowCurrentRecord(0);
        if (current.item.frames) itfMesWindowSetFlagBits((int32_t)P4_G32(WINDOW_CURRENT), WINDOW_IGNORE_INPUT);
        current.showing = 1;
        current.left = current.item.frames;
        return 1;
    }
    nmMesWindowStepCurrentRelease();
    if (current.item.frames ? --current.left > 0 : nmMesWindowCurrentOpenState() != 0) return 1;
    nmMesWindowReleaseCurrent();
    nmMesWindowResetCurrent();
    return advance();
}

int32_t p4_battle_present_poll(void)
{
    if (!current.active) return 0;
    const uint32_t battle = battle_state();
    if (!battle) {
        current.active = 0;
        return 0;
    }
    if (current.item.kind == P4_PRESENT_SAY) return say();
    if (current.showing) return --current.left > 0 ? 1 : advance();
    return banner(battle);
}

void p4_battle_present_remove_owner(uint32_t owner)
{
    for (int i = 0; i < P4_PRESENT_QUEUE; ++i)
        if (items[i].used && items[i].owner == owner) drop(&items[i]);
    p4_battle_present_clear(owner, 0);
    /* An item already on screen finishes; one still waiting gives way to the next. */
    if (current.active && current.item.owner == owner && !current.showing) advance();
}
