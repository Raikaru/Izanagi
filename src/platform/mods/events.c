#include "p4pc/mod_events.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/guest.h"
#include "p4pc/mods.h"
#include "p4pc/mod_failure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FRAME, FIELD_ENTER, FIELD_LEAVE, BATTLE_START, BATTLE_END, MENU_OPEN, MENU_CLOSE, CALENDAR, SAVE, LOAD, EXIT, EVENT_COUNT };
static const char *const names[EVENT_COUNT] = {"frame_start", "field_enter", "field_leave", "battle_start", "battle_end", "menu_open", "menu_close", "calendar_advance", "save", "load", "exit"};
_Static_assert(P4_MOD_FRAME_START_BIT == 1u << FRAME && P4_MOD_EXIT_BIT == 1u << EXIT, "scheduler event bits");
struct P4ModEventSubscription {
    size_t owner;
    int event, enabled, removed;
    P4ModEventFn callback;
    void *user;
    P4ModEventSubscription *next;
};
static P4ModEventSubscription *subscriptions;
uint32_t p4_mod_event_mask;
uint32_t p4_mod_safe_point;
static unsigned event_depth;
static uint32_t field_root, field_ids;
static int field_ready;
static int32_t old_dates[64];
static uint8_t old_slots[64];
static unsigned calendar_depth;

int p4_mod_events_quiescent(void) { return event_depth == 0; }
static void require_quiescent(void)
{
    if (!event_depth) return;
    fprintf(stderr, "p4pc: mods: bulk event teardown requires quiescent callbacks\n");
    fflush(stderr);
    _Exit(EXIT_FAILURE);
}
#define BIT(x) (1u << (x))
#define FIELD_BITS (BIT(FIELD_ENTER) | BIT(FIELD_LEAVE))
#define MENU_BITS (BIT(MENU_OPEN) | BIT(MENU_CLOSE))

typedef struct Route {
    uint32_t bits, address;
    const char *signature;
    int phase;
    P4ModHook *hook;
} Route;
static Route routes[] = {
    {FIELD_BITS, 0x00155360, "ip", P4_MOD_BEFORE, NULL},
    {FIELD_BITS, 0x00155360, "ip", P4_MOD_AFTER, NULL},
    {FIELD_BITS, 0x00155bf0, "vp", P4_MOD_BEFORE, NULL},
    {BIT(BATTLE_START), 0x00192e90, "ii", P4_MOD_AFTER, NULL},
    {BIT(BATTLE_END), 0x00192cd0, "i", P4_MOD_BEFORE, NULL},
    {MENU_BITS, 0x0012d000, "iih", P4_MOD_AFTER, NULL},
    {MENU_BITS, 0x0012cc50, "vp", P4_MOD_BEFORE, NULL},
    {MENU_BITS, 0x0035c690, "ipi", P4_MOD_AFTER, NULL},
    {MENU_BITS, 0x0035e820, "vp", P4_MOD_BEFORE, NULL},
    {BIT(CALENDAR), 0x00106100, "vh", P4_MOD_BEFORE, NULL},
    {BIT(CALENDAR), 0x00106100, "vh", P4_MOD_AFTER, NULL},
    {BIT(CALENDAR), 0x001062f0, "vB", P4_MOD_BEFORE, NULL},
    {BIT(CALENDAR), 0x001062f0, "vB", P4_MOD_AFTER, NULL},
    {BIT(SAVE), 0x0010d7c0, "pipi", P4_MOD_AFTER, NULL},
    {BIT(LOAD), 0x0010e710, "vipi", P4_MOD_AFTER, NULL}
};

static int range(uint32_t address, size_t size)
{
    return address >= 0x00100000u && address < 0x02000000u && size <= 0x02000000u - address;
}
static uint32_t word(uint32_t address)
{
    uint32_t value = 0;
    if (range(address, sizeof value)) memcpy(&value, P4_G2H(address), sizeof value);
    return value;
}
static uint16_t half(uint32_t address)
{
    uint16_t value = 0;
    if (range(address, sizeof value)) memcpy(&value, P4_G2H(address), sizeof value);
    return value;
}
static void emit(int event, P4ModEvent *payload)
{
    if (!(p4_mod_event_mask & BIT(event))) return;
    payload->name = names[event];
    event_depth++;
    for (P4ModEventSubscription *s = subscriptions; s; s = s->next) {
        if (!s->removed && s->enabled && s->event == event && s->callback(payload, s->user)) {
            char reason[128];
            snprintf(reason, sizeof reason, "Native %s event callback failed", names[event]);
            p4_mod_failure(s->owner, reason);
        }
    }
    event_depth--;
}
static void field_event(int event)
{
    P4ModEvent payload = {NULL, 5, {{"task", field_root}, {"field", field_ids & 0xffffu}, {"room", field_ids >> 16},
                                  {"date", half(0x00797b7a)}, {"period", word(0x00797b7c) & 0xffu}}};
    emit(event, &payload);
}

static int route_callback(P4ModCall *call, void *user)
{
    Route *route = user;
    P4ModEvent event = {0};
    switch (route->address) {
    case 0x00155360: {
        uint32_t root = (uint32_t)call->args[0].u;
        uint32_t work = range(root, 0x3c) ? word(root + 0x38) : 0;
        uint32_t scene = word(0x00762ea0), state = word(work);
        uint32_t ids = word(work + 0x18);
        int ready = range(work, 0x24) && state >= 12 && state <= 14 && range(scene, 0x14) &&
                    !(word(scene + 0xc) & 6u) && (word(scene) & 0xffffu) == (ids & 0xffffu) &&
                    (word(scene + 4) & 0xffffu) == (ids >> 16);
        if (field_ready && (!ready || field_root != root || field_ids != ids)) {
            field_event(FIELD_LEAVE); field_ready = 0;
        }
        if (call->phase == P4_MOD_AFTER && ready && !field_ready) {
            field_root = root; field_ids = ids; field_ready = 1; field_event(FIELD_ENTER);
        }
        break;
    }
    case 0x00155bf0:
        if (field_ready && field_root == (uint32_t)call->args[0].u) { field_event(FIELD_LEAVE); field_ready = 0; }
        break;
    case 0x00192e90:
        if (!call->result->i) break;
        event.count = 3;
        event.fields[0] = (P4ModEventField){"task", call->result->i};
        event.fields[1] = (P4ModEventField){"setup", call->args[0].u};
        event.fields[2] = (P4ModEventField){"battle", word(0x0076449c)};
        emit(BATTLE_START, &event);
        break;
    case 0x00192cd0:
        event.count = 2;
        event.fields[0] = (P4ModEventField){"battle", word(0x0076449c)};
        event.fields[1] = (P4ModEventField){"outcome", (int16_t)half(word(0x0076449c) + 0x1c)};
        emit(BATTLE_END, &event);
        break;
    case 0x0012d000: case 0x0035c690:
        if (!call->result->i) break;
        event.count = 2;
        event.fields[0] = (P4ModEventField){"task", call->result->i};
        event.fields[1] = (P4ModEventField){"kind", route->address == 0x0035c690};
        emit(MENU_OPEN, &event);
        break;
    case 0x0012cc50: case 0x0035e820:
        event.count = 2;
        event.fields[0] = (P4ModEventField){"task", call->args[0].u};
        event.fields[1] = (P4ModEventField){"kind", route->address == 0x0035e820};
        emit(MENU_CLOSE, &event);
        break;
    case 0x00106100: case 0x001062f0:
        if (call->phase == P4_MOD_BEFORE) {
            if (calendar_depth == 64) return -1;
            old_dates[calendar_depth] = (int16_t)half(0x00797b7a);
            old_slots[calendar_depth++] = (uint8_t)word(0x00797b7c);
        } else {
            if (!calendar_depth) return -1;
            calendar_depth--;
            int32_t date = (int16_t)half(0x00797b7a);
            uint8_t period = (uint8_t)word(0x00797b7c);
            if (date == old_dates[calendar_depth] && period == old_slots[calendar_depth]) break;
            event.count = 4;
            event.fields[0] = (P4ModEventField){"old_date", old_dates[calendar_depth]};
            event.fields[1] = (P4ModEventField){"old_period", old_slots[calendar_depth]};
            event.fields[2] = (P4ModEventField){"date", date};
            event.fields[3] = (P4ModEventField){"period", period};
            emit(CALENDAR, &event);
        }
        break;
    case 0x0010d7c0:
        event.count = 3;
        event.fields[0] = (P4ModEventField){"version", call->args[0].i};
        event.fields[1] = (P4ModEventField){"data", call->result->u};
        event.fields[2] = (P4ModEventField){"size", word((uint32_t)call->args[1].u)};
        emit(SAVE, &event);
        break;
    case 0x0010e710:
        event.count = 3;
        event.fields[0] = (P4ModEventField){"version", call->args[0].i};
        event.fields[1] = (P4ModEventField){"data", call->args[1].u};
        event.fields[2] = (P4ModEventField){"size", call->args[2].i};
        emit(LOAD, &event);
        break;
    }
    return 0;
}

static int update_routes(uint32_t mask)
{
    for (size_t i = 0; i < sizeof routes / sizeof routes[0]; i++) {
        Route *route = &routes[i];
        int enabled = (mask & route->bits) != 0;
        if (!route->hook && enabled) {
            P4ModFunction *function = p4_mod_function_find(route->address, route->signature);
            route->hook = p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, route->phase, route_callback, route);
            if (!route->hook) {
                fprintf(stderr, "p4pc: mods: event seam unavailable: %08x signature %s\n", route->address, route->signature);
                return -1;
            }
        } else if (route->hook && p4_mod_hook_enable(route->hook, enabled)) return -1;
    }
    return 0;
}

static int update(void)
{
    uint32_t mask = 0;
    for (P4ModEventSubscription *s = subscriptions; s; s = s->next)
        if (s->enabled && !s->removed) mask |= BIT(s->event);
    if (update_routes(mask)) { update_routes(p4_mod_event_mask); return -1; }
    p4_mod_event_mask = mask;
    return 0;
}

P4ModEventSubscription *p4_mod_event_subscribe(size_t owner, const char *name, P4ModEventFn callback, void *user)
{
    if (!name || !callback || event_depth) return NULL;
    int event;
    for (event = 0; event < EVENT_COUNT && strcmp(name, names[event]); event++) {}
    if (event == EVENT_COUNT) return NULL;
    P4ModEventSubscription *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->owner = owner; s->event = event; s->enabled = 1; s->callback = callback; s->user = user;
    P4ModEventSubscription **link = &subscriptions;
    while (*link && (*link)->owner <= owner) link = &(*link)->next;
    s->next = *link; *link = s;
    if (update()) { *link = s->next; free(s); return NULL; }
    return s;
}
int p4_mod_event_enable(P4ModEventSubscription *s, int enabled)
{
    if (!s || s->removed) return -1;
    int previous = s->enabled;
    s->enabled = !!enabled;
    if (update()) { s->enabled = previous; return -1; }
    return 0;
}
int p4_mod_event_remove(P4ModEventSubscription *s)
{
    if (!s) return -1;
    if (s->removed) return 0;
    int previous = s->enabled;
    s->enabled = 0;
    if (update()) { s->enabled = previous; return -1; }
    s->removed = 1;
    return 0;
}
void p4_mod_events_remove_owner(size_t owner)
{
    require_quiescent();
    for (P4ModEventSubscription *s = subscriptions; s; s = s->next)
        if (s->owner == owner) p4_mod_event_remove(s);
}
void p4_mod_events_clear(void)
{
    require_quiescent();
    for (size_t i = 0; i < sizeof routes / sizeof routes[0]; i++) {
        if (routes[i].hook) p4_mod_hook_remove(routes[i].hook);
        routes[i].hook = NULL;
    }
    while (subscriptions) { P4ModEventSubscription *next = subscriptions->next; free(subscriptions); subscriptions = next; }
    p4_mod_event_mask = 0;
    field_ready = 0;
    calendar_depth = 0;
}
static int (*published_notify)(void);
static void (*quit_finish)(void);
void p4_mod_events_set_published(int (*notify)(void)) { published_notify = notify; }
static int (*services_pump)(uint32_t frame);
void p4_mod_events_set_services(int (*pump)(uint32_t frame)) { services_pump = pump; }
void p4_mod_event_frame(uint32_t frame)
{
    /* The scheduler's frame boundary is the game-thread safe point for queued
     * work: durable-save publication notices first, then an orderly quit. */
    if (p4_mod_safe_point & P4_MOD_SAFE_PUBLISHED) {
        if (!published_notify || !published_notify()) p4_mod_safe_point &= ~P4_MOD_SAFE_PUBLISHED;
    }
    if (p4_mod_safe_point & P4_MOD_SAFE_SERVICES) {
        if (!services_pump || !services_pump(frame)) p4_mod_safe_point &= ~P4_MOD_SAFE_SERVICES;
    }
    if (p4_mod_safe_point & P4_MOD_SAFE_EXIT) {
        P4ModEvent quit = {NULL, 0, {{0}}};
        emit(EXIT, &quit);
        fprintf(stderr, "p4pc: orderly quit after the mod exit event\n");
        fflush(stderr);
        if (quit_finish) quit_finish();
        exit(0);
    }
    if (!(p4_mod_event_mask & P4_MOD_FRAME_START_BIT)) return;
    P4ModEvent event = {NULL, 1, {{"frame", frame}}};
    emit(FRAME, &event);
}
int p4_mod_event_quit_request(void (*finish)(void))
{
    /* A second close request, or no exit subscriber, keeps the immediate exit. */
    if (!(p4_mod_event_mask & P4_MOD_EXIT_BIT) || (p4_mod_safe_point & P4_MOD_SAFE_EXIT)) return 0;
    quit_finish = finish;
    p4_mod_safe_point |= P4_MOD_SAFE_EXIT;
    return 1;
}
