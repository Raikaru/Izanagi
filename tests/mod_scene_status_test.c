/* Synthetic guest scene drives the production request admission and scheduler. */
#include "../src/platform/mods/game_services.c"
#undef NDEBUG
#include <assert.h>

uint8_t *p4_guest_base;
uint32_t p4_mod_safe_point, p4_mod_authorize_mask;
void *p4_g2h_slow(uint32_t a) { (void)a; return NULL; }
int p4_settings_native_active(void) { return 0; }
int p4_mod_hook_enable(P4ModHook *hook, int enabled) { (void)hook; (void)enabled; return 0; }
void p4gl_hud_hidden_set(const uint32_t *q, unsigned n, const uint32_t (*c)[2], unsigned m)
{ (void)q; (void)n; (void)c; (void)m; }
int p4_mod_services_native_resources_ready(void) { return 1; }
int p4_mod_services_native_field(uint32_t f, uint32_t r, uint32_t e, uint32_t d)
{ (void)f; (void)r; (void)e; (void)d; return 1; }
int p4_mod_services_native_encounter(uint32_t id) { (void)id; return 1; }
int p4_mod_services_native_battle_running(void) { return 0; }
void p4_mod_services_native_revive_party(void) { }
int p4_mod_field_host_busy(void) { return 0; }
int p4pc_pace_request(unsigned quarters) { (void)quarters; return 0; }
unsigned p4pc_pace_effective(void) { return 4; }
int p4_mod_authorize(int op, const P4ModEvent *e, char *reason, size_t n)
{ (void)op; (void)e; (void)reason; (void)n; return 1; }
static int16_t date = 145;
static uint8_t slot = 3;
static void put(uint32_t a, uint32_t v) { memcpy(p4_guest_base + a, &v, 4); }
void p4_mod_services_native_calendar_now(int16_t *d, uint8_t *s) { *d = date; *s = slot; }
int p4_mod_services_native_calendar_next(int16_t *d, uint8_t *s) { *d = date; *s = slot + 1; return 1; }
static int hold, passes;
int p4_mod_services_native_calendar_hold(void) { return hold; }
void p4_mod_services_native_calendar_pass(int16_t d, uint8_t s)
{ date = d; slot = s; ++passes; put(0x1201004, 25); }

int main(void)
{
    const char *detail, *reason = NULL;
    int outcome;
    p4_guest_base = calloc(1, P4_GUEST_SIZE);
    assert(p4_guest_base);
    put(P4_GPADDR(0xb1a8), 0x1200000); put(0x1200038, 0x1201000); put(0x1201004, 9);
    put(0x007d2510, 0x1202000); put(0x1202038, 0x1203000);
    put(0x1203000, 13); put(0x1203018, 8 | (2u << 16));
    put(0x00762ea0, 0x1204000); put(0x1204000, 8); put(0x1204004, 2);
    put(0x00764adc, 0x1205000); memcpy(p4_guest_base + 0x1205000, "rotate pc", 10);
    for (int i = SEAM_FIELD_START; i <= SEAM_STEP_START; ++i) seams[i].hook = (P4ModHook *)1;

    assert(p4_mod_calendar_pass(P4_MOD_NATIVE_OWNER, &reason) == 0);
    assert(strcmp(p4_mod_services_status(P4_MOD_NATIVE_OWNER, &detail, &outcome), "requested") == 0);
    assert(p4_mod_calendar_pass(7, &reason) == -1 && strcmp(reason, "another scene request is in flight") == 0);
    run_request(); /* request calendar sequence */
    run_request(); /* observe it */
    put(0x1201004, 9);
    run_request(); /* observe the completed slot */
    assert(strcmp(p4_mod_services_status(P4_MOD_NATIVE_OWNER, &detail, &outcome), "done") == 0);
    assert(strcmp(detail, "passed") == 0);
    assert(p4_mod_calendar_pass(7, &reason) == 0);
    assert(strcmp(p4_mod_services_status(P4_MOD_NATIVE_OWNER, &detail, &outcome), "done") == 0);
    assert(strcmp(detail, "passed") == 0);
    p4_mod_services_release_owner(P4_MOD_NATIVE_OWNER);
    assert(strcmp(p4_mod_services_status(P4_MOD_NATIVE_OWNER, &detail, &outcome), "idle") == 0);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "requested") == 0);
    p4_mod_services_release_owner(7);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "idle") == 0);
    assert(strcmp(p4_mod_services_status(P4_MOD_NATIVE_OWNER, &detail, &outcome), "idle") == 0);

    slot = 3;
    assert(p4_mod_calendar_pass(7, &reason) == 0);
    put(0x00764604, 1); /* the field becomes unavailable before the safe point */
    run_request();
    put(0x00764604, 0);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "refused") == 0);
    assert(strcmp(detail, "an event or message holds the input") == 0);
    assert(p4_mod_calendar_pass(P4_MOD_NATIVE_OWNER, &reason) == 0);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "refused") == 0);
    assert(strcmp(detail, "an event or message holds the input") == 0);
    p4_mod_services_release_owner(7);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "idle") == 0);
    assert(strcmp(p4_mod_services_status(P4_MOD_NATIVE_OWNER, &detail, &outcome), "requested") == 0);
    p4_mod_services_release_owner(P4_MOD_NATIVE_OWNER);
    assert(strcmp(p4_mod_services_status(P4_MOD_NATIVE_OWNER, &detail, &outcome), "idle") == 0);

    /* A story hold refuses the request before anything changes... */
    slot = 3;
    assert(p4_mod_calendar_hold() == NULL);
    hold = P4_MOD_CALENDAR_HOLD_INVESTIGATION;
    assert(strcmp(p4_mod_calendar_hold(), "the 12/05 investigation holds the day") == 0);
    assert(p4_mod_calendar_pass(7, &reason) == -1 && strcmp(reason, "the 12/05 investigation holds the day") == 0);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "idle") == 0);
    /* ...and one that begins after the request refuses it at the boundary,
     * before the slot is staged. */
    hold = P4_MOD_CALENDAR_HOLD_NONE;
    int before = passes;
    assert(p4_mod_calendar_pass(7, &reason) == 0);
    hold = P4_MOD_CALENDAR_HOLD_LAST_DAY;
    run_request();
    assert(passes == before && slot == 3);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "refused") == 0);
    assert(strcmp(detail, "the last day holds the time") == 0);
    hold = P4_MOD_CALENDAR_HOLD_NONE;
    p4_mod_services_release_owner(7);
    /* The slot's scheduler procedure still running holds the time the same
     * way: refused when requested, refused at the boundary when it holds the
     * slot after the request, and staged once it has ended. */
    hold = P4_MOD_CALENDAR_HOLD_SCHEDULE;
    assert(strcmp(p4_mod_calendar_hold(), "the schedule still runs this time of day") == 0);
    assert(p4_mod_calendar_pass(7, &reason) == -1 && strcmp(reason, "the schedule still runs this time of day") == 0);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "idle") == 0);
    hold = P4_MOD_CALENDAR_HOLD_NONE;
    assert(p4_mod_calendar_pass(7, &reason) == 0);
    hold = P4_MOD_CALENDAR_HOLD_SCHEDULE;
    run_request();
    assert(passes == before && slot == 3);
    assert(strcmp(p4_mod_services_status(7, &detail, &outcome), "refused") == 0);
    assert(strcmp(detail, "the schedule still runs this time of day") == 0);
    hold = P4_MOD_CALENDAR_HOLD_NONE;
    assert(p4_mod_calendar_pass(7, &reason) == 0);
    run_request();
    assert(passes == before + 1 && slot == 4);
    p4_mod_services_release_owner(7);
    free(p4_guest_base);
    return 0;
}
