/* Synthetic flags, calendar and task lists drive the production native
 * calendar half (services_native.c): the hold, the next slot and its staging. */
#include "../src/platform/mods/services_native.c"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>

uint8_t *p4_guest_base;
void *p4_g2h_slow(uint32_t a) { (void)a; return NULL; }
uint32_t p4_gport_alloc(uint32_t size, uint32_t align) { (void)size; (void)align; return 0; }
int p4_battle_encounter_start(uint32_t encounter) { (void)encounter; return 0; }
int32_t mtSceneResListPrepareAll(void) { return 1; }
void sdlSchedulerEnable(void) { }
int32_t btlIsRunning(void) { return 0; }
int16_t datGetPartyId(int32_t slot) { (void)slot; return 0; }
uint32_t datCalcIsDeadById(int16_t unit) { (void)unit; return 0; }
void datSetUnitHp(int16_t unit, int16_t hp) { (void)unit; (void)hp; }
void datCalcClearBadStatusById(int16_t unit, uint32_t mask) { (void)unit; (void)mask; }

static int16_t date = 145, pending_date;
static uint8_t slot = 3, pending_slot;
static uint32_t pending_flag;
static int holiday, investigation, last_day, calendar_requests;
int16_t datGetDateIndex(void) { return date; }
uint8_t datGetTimeSlot(void) { return slot; }
int16_t datGetPendingDateIndex(void) { return pending_date; }
uint8_t datGetPendingTimeSlot(void) { return pending_slot; }
int32_t datGetPendingFlag(void) { return (int32_t)pending_flag; }
void datSetPendingDateIndex(int16_t d) { pending_date = d; }
void datSetPendingTimeSlot(uint8_t s) { pending_slot = s; }
void datSetPendingFlag(uint32_t f) { pending_flag = f; }
int32_t calHasDayEntry(int32_t day) { return day == date && holiday; }
void sequenceRequest(int32_t sequence, uint32_t data, int32_t size, int32_t delay)
{
    assert(sequence == SEQ_CALENDAR && !data && !size && !delay);
    ++calendar_requests;
}
uint32_t datGetFlag(int32_t bit) { return bit == 0x60F ? investigation : bit == 0x602 ? last_day : 0; }

/* The game's task lists as kwlnTaskGetTaskByName searches them (starting,
 * running and dying): at most one live task here, by name. */
static const char *live_task;
static const void *asked;
void *kwlnTaskGetTaskByName(const void *name)
{
    asked = name;
    return live_task && strcmp((const char *)name, live_task) == 0 ? (void *)&live_task : NULL;
}

int main(void)
{
    int16_t d;
    uint8_t s;
    p4_guest_base = calloc(1, P4_GUEST_SIZE);
    assert(p4_guest_base);
    memcpy(p4_guest_base + 0x00637348u, "sdlScheduler", 13); /* cldScheduler.c's task name */

    /* A free afternoon: nothing holds it, and the owner is looked up by the
     * game's own name string. */
    assert(p4_mod_services_native_calendar_hold() == P4_MOD_CALENDAR_HOLD_NONE);
    assert(asked == p4_guest_base + 0x00637348u);
    live_task = "fieldMain";
    assert(p4_mod_services_native_calendar_hold() == P4_MOD_CALENDAR_HOLD_NONE);
    /* The slot's procedure still runs (or has just ended and is dying). */
    live_task = "sdlScheduler";
    assert(p4_mod_services_native_calendar_hold() == P4_MOD_CALENDAR_HOLD_SCHEDULE);
    /* The story holds name their reason before the schedule's. */
    investigation = 1;
    assert(p4_mod_services_native_calendar_hold() == P4_MOD_CALENDAR_HOLD_INVESTIGATION);
    live_task = NULL;
    assert(p4_mod_services_native_calendar_hold() == P4_MOD_CALENDAR_HOLD_INVESTIGATION);
    investigation = 0, last_day = 1;
    assert(p4_mod_services_native_calendar_hold() == P4_MOD_CALENDAR_HOLD_LAST_DAY);
    last_day = 0;
    assert(p4_mod_services_native_calendar_hold() == P4_MOD_CALENDAR_HOLD_NONE);

    /* The next slot as calendarStepTimeSlot stages it: one step on a school
     * day, 0 -> 3 -> 5 on a day with a calendar entry, slot 5 to tomorrow. */
    assert(p4_mod_services_native_calendar_next(&d, &s) == 0 && d == 145 && s == 4);
    holiday = 1;
    assert(p4_mod_services_native_calendar_next(&d, &s) == 0 && d == 145 && s == 5);
    slot = 0;
    assert(p4_mod_services_native_calendar_next(&d, &s) == 0 && d == 145 && s == 3);
    slot = 5;
    assert(p4_mod_services_native_calendar_next(&d, &s) == 0 && d == 146 && s == 0);
    holiday = 0, slot = 3;

    /* Passing stages the computed slot through the game's setters once, and
     * keeps a slot the game has staged itself. */
    p4_mod_services_native_calendar_pass(145, 4);
    assert(pending_flag == 1 && pending_date == 145 && pending_slot == 4 && calendar_requests == 1);
    assert(p4_mod_services_native_calendar_next(&d, &s) == 1 && d == 145 && s == 4);
    pending_slot = 5;
    p4_mod_services_native_calendar_pass(145, 4);
    assert(pending_date == 145 && pending_slot == 5 && calendar_requests == 2);
    free(p4_guest_base);
    return 0;
}
