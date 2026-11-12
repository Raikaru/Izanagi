/* Native half of the mod scene/encounter services (include/p4pc/mod_services.h).
 * Each function repeats one of the game's own script-command paths and runs
 * only from the scheduler frame boundary, never inside a task or hook:
 *   field    K_Cmd_CALL_FIELD       00176450: sequence 9 with a 0x1C record
 *   battle   btlScriptStartEncounter 001932f0 through the shared adapter
 *            (src/platform/battle_sim/entry.c): sequence 6 with a 0x3C setup
 *   calendar K_Cmd_CALL_TV          00176320: sequence 25, CALENDAR MAIN, whose
 *            calendarStepTimeSlot (00120f70) applies the staged next slot and
 *            starts the period's scheduler procedure (cldSchedulerStart)
 * Validation and lifecycle policy belong to the caller (game_services.c). */
#include "p4pc/mod_services.h"
#include "p4pc/battle_sim.h"
#include "p4pc/gmem.h"
#include <string.h>

extern void sequenceRequest(int32_t sequence, uint32_t data, int32_t size, int32_t delay);
extern int32_t mtSceneResListPrepareAll(void);
extern void sdlSchedulerEnable(void);
extern int32_t btlIsRunning(void);
extern int16_t datGetPartyId(int32_t slot);
extern uint32_t datCalcIsDeadById(int16_t unit);
extern void datSetUnitHp(int16_t unit, int16_t hp);
extern void datCalcClearBadStatusById(int16_t unit, uint32_t mask);
extern int16_t datGetDateIndex(void);
extern uint8_t datGetTimeSlot(void);
extern int16_t datGetPendingDateIndex(void);
extern uint8_t datGetPendingTimeSlot(void);
extern int32_t datGetPendingFlag(void);
extern void datSetPendingDateIndex(int16_t date);
extern void datSetPendingTimeSlot(uint8_t slot);
extern void datSetPendingFlag(uint32_t flags);
extern int32_t calHasDayEntry(int32_t day);
extern uint32_t datGetFlag(int32_t bit);
extern void *kwlnTaskGetTaskByName(const void *name);

enum { SEQ_FIELD = 9, SEQ_CALENDAR = 0x19 };

/* One port-owned record; sequenceRequest copies it when the start is delayed. */
static uint32_t record_address;

static uint8_t *record(void)
{
    if (!record_address) record_address = p4_gport_alloc(0x20, 16);
    return record_address ? P4_GPTR(uint8_t, record_address) : NULL;
}

int p4_mod_services_native_resources_ready(void)
{
    /* The script commands wait (return 0, retried next frame) until every
     * scene resource model is prepared; requests do the same. */
    return mtSceneResListPrepareAll() != 0;
}

int p4_mod_services_native_field(uint32_t field, uint32_t room, uint32_t entry, uint32_t direction)
{
    uint8_t *r = record();
    if (!r || !field) return 0;
    int16_t words[14];
    memset(words, 0, sizeof words);
    words[0] = (int16_t)field;
    words[1] = (int16_t)room;
    words[2] = (int16_t)entry;
    words[4] = (int16_t)direction;
    memcpy(r, words, 0x1C);
    sdlSchedulerEnable();
    sequenceRequest(SEQ_FIELD, record_address, 0x1C, 2);
    return 1;
}

int p4_mod_services_native_encounter(uint32_t id)
{
    return p4_battle_encounter_start(id);
}

int p4_mod_services_native_battle_running(void)
{
    return btlIsRunning() != 0;
}

/* btlScriptResolvePartyDeath (00193450) revives knocked-out allies to one HP
 * and clears status bit 0x80000 after a scripted battle; a practice launch has
 * no script, so its return applies that recovery to every party member. */
void p4_mod_services_native_revive_party(void)
{
    int16_t units[5] = {1, 0, 0, 0, 0};
    int count = 1;
    for (int32_t slot = 0; slot < 4 && count < 5; ++slot) {
        int16_t id = datGetPartyId(slot);
        if (id > 1 && id <= 10) units[count++] = id;
    }
    for (int i = 0; i < count; ++i) {
        if (!datCalcIsDeadById(units[i])) continue;
        datSetUnitHp(units[i], 1);
        datCalcClearBadStatusById(units[i], 0x80000u);
    }
}

void p4_mod_services_native_calendar_now(int16_t *date, uint8_t *slot)
{
    *date = datGetDateIndex();
    *slot = datGetTimeSlot();
}

/* The slot the calendar step will apply: the staged one when the game has
 * staged it, else the one calendarStepTimeSlot (00120f70) itself stages from
 * the current slot (5 rolls over to slot 0 of the next day; days without a
 * calendar entry step by one; others step 0 -> 3 -> 5). */
int p4_mod_services_native_calendar_next(int16_t *date, uint8_t *slot)
{
    int16_t today = datGetDateIndex();
    uint8_t now = datGetTimeSlot();
    if (datGetPendingFlag()) {
        *date = datGetPendingDateIndex();
        *slot = datGetPendingTimeSlot();
        return 1;
    }
    if (now == 5) {
        *date = (int16_t)(today + 1);
        *slot = 0;
    } else if (!calHasDayEntry(today)) {
        *date = today;
        *slot = (uint8_t)(now + 1);
    } else {
        *date = today;
        *slot = now == 0 ? 3 : now == 3 ? 5 : (uint8_t)(now + 1);
    }
    return 0;
}

/* Whether the time is the player's to spend. Each calendar step starts the
 * slot's owner, cldSchedulerStart (00260450): the "sdlScheduler" task runs
 * that slot's sdlMM_DD_<slot> procedure, and when it ends without having
 * handed the player a scene (sdlSchedulerEnable) cldSchedulerTaskUpdate
 * (00260020) requests the calendar sequence itself. Until the task is gone the
 * slot is still the schedule's: a pass staged then would race its own step.
 * kwlnTaskRunUpdate moves the finished task to the dying list, which
 * kwlnTaskGetTaskByName still searches, and kwlnTaskRunFrame frees it two
 * frames later, so this hold never outlasts the procedure by more than that.
 *
 * Then the story hold: the script flags (BIT_CHK, which is scrGetFlag ->
 * datGetFlag) on which the optional time-spending activities of the free
 * fields all refuse (Aiya, the shrine, the bus stop and the capsule machines
 * of the shopping district, the Junes elevator, the library, the desk, sofa
 * and bed at home, riverbank fishing). Only scheduler procedures set and clear
 * them (no field or event script does, and no native call names them):
 *   0x60F (1551): sdl12_05_AM_A .. sdl12_05_PM_D, the 12/05 investigation
 *                 (MSG_1215_STOP, "You have to gather information"), after
 *                 which PM_D's deduction picks the ending
 *   0x602 (1538): sdl03_20_PM_B .. sdl03_21_AM_A, the last day, when only the
 *                 farewell branches run ("There's no reason for you to be here")
 * From 8/24 these are the only afternoons whose free field carries an
 * obligation; docs/modding/golden.md (Bike rides) lists what the other
 * scheduler procedures test. */
#define D_00637348 P4_GARRAY(char, 0x00637348u)   /* "sdlScheduler" (cldScheduler.c) */
int p4_mod_services_native_calendar_hold(void)
{
    if (datGetFlag(0x60F)) return P4_MOD_CALENDAR_HOLD_INVESTIGATION;
    if (datGetFlag(0x602)) return P4_MOD_CALENDAR_HOLD_LAST_DAY;
    if (kwlnTaskGetTaskByName(D_00637348)) return P4_MOD_CALENDAR_HOLD_SCHEDULE;
    return P4_MOD_CALENDAR_HOLD_NONE;
}

/* Stages the slot through the game's own setters (as K_Cmd_DATE_SCHEDULE,
 * 0025c560, does) when nothing is staged yet, then requests the calendar. */
void p4_mod_services_native_calendar_pass(int16_t date, uint8_t slot)
{
    if (!datGetPendingFlag()) {
        datSetPendingDateIndex(date);
        datSetPendingTimeSlot(slot);
        datSetPendingFlag(1);
    }
    sequenceRequest(SEQ_CALENDAR, 0, 0, 0);
}
