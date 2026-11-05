/* Native half of the mod scene/encounter services (include/p4pc/mod_services.h).
 * Each function repeats one of the game's own script-command paths and runs
 * only from the scheduler frame boundary, never inside a task or hook:
 *   field   K_Cmd_CALL_FIELD       00176450: sequence 9 with a 0x1C record
 *   battle  btlScriptStartEncounter 001932f0 through the shared adapter
 *           (src/platform/battle_sim/entry.c): sequence 6 with a 0x3C setup
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

enum { SEQ_FIELD = 9 };

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
