/* Afflictions this battle: per unit, the ailments landed since the battle
 * constructor began (include/p4pc/game_afflictions_native.h). */
#include "p4pc/battle_events.h"
#include "p4pc/game_afflictions_native.h"
#include "p4pc/game_chance_stage_native.h"
#include "p4pc/game_wearoff_stage_native.h"
#include <stddef.h>

/* A battle holds at most 4 party members and 5 enemies at once; the margin
 * covers reinforcements whose DatUnit is new. A full table stops counting
 * new units until the next battle. */
static struct {
    const void *unit;
    uint32_t count;
} entries[32];
static unsigned used;
static uint32_t observer;

static void battle_request(const P4BattleRequest *request, void *context)
{
    (void)context;
    if (request->stage == P4_BATTLE_TASK_BEGIN) used = 0;
}

int p4_game_afflictions_native_active(void)
{
    return p4_game_chance_native_active() || p4_game_wearoff_native_active();
}

void p4_game_afflictions_native_landed(const void *unit, uint32_t statuses)
{
    if (!(statuses & P4_AFFLICTION_MASK) || !unit) return;
    /* The first landing installs the battle-start reset; until then no count
     * exists to reset. Without it nothing is counted. */
    if (!observer) {
        const P4BattleObserver reset = {.request = battle_request};
        observer = p4_battle_observer_add(&reset);
        if (!observer) return;
    }
    for (unsigned i = 0; i < used; i++) {
        if (entries[i].unit != unit) continue;
        if (entries[i].count != UINT32_MAX) entries[i].count++;
        return;
    }
    if (used == sizeof entries / sizeof *entries) return;
    entries[used].unit = unit;
    entries[used].count = 1;
    used++;
}

uint32_t p4_game_afflictions_native_count(const void *unit)
{
    for (unsigned i = 0; i < used; i++) {
        if (entries[i].unit == unit) return entries[i].count;
    }
    return 0;
}
