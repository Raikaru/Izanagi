/* P4PC_BATTLE_TRACE=1: one stderr line per committed native battle command and
 * hit, copied from the existing observer broker (p4pc/battle_events.h). It is a
 * read-only diagnostic for witness logs (for example a WEAK hit that adds Down,
 * then the same actor's One More command); it never changes game state, input
 * or RNG, and a retail run without the variable registers nothing. */
#include "p4pc/battle_events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned flip; /* The presented flip of the current game frame, for captures. */
static void command(const P4BattleCommand *event, void *context)
{
    (void)context;
    fprintf(stderr, "p4pc: battle trace command actor %u:%u kind %u skill %u item %u targets %u flip %u\n",
        (unsigned)event->actor.side, (unsigned)event->actor.native_id, (unsigned)event->action_kind,
        (unsigned)event->skill_id, (unsigned)event->item_id, (unsigned)event->target_count, flip);
}
static void hit(const P4BattleHit *event, void *context)
{
    (void)context;
    fprintf(stderr, "p4pc: battle trace hit source %u:%u target %u:%u skill %u target_flags 0x%04x "
        "result_flags 0x%04x hp %u->%u added_status 0x%08x status 0x%08x->0x%08x flip %u\n",
        (unsigned)event->source.side, (unsigned)event->source.native_id,
        (unsigned)event->target.side, (unsigned)event->target.native_id, (unsigned)event->skill_id,
        (unsigned)event->target_flags, (unsigned)event->result_flags, (unsigned)event->hp_before,
        (unsigned)event->hp_after, (unsigned)event->added_status, (unsigned)event->status_before,
        (unsigned)event->status_after, flip);
}
void p4pc_battle_trace_frame(unsigned frame)
{
    static int started;
    flip = frame;
    if (started) return;
    started = 1;
    const char *value = getenv("P4PC_BATTLE_TRACE");
    if (!value || strcmp(value, "1")) return;
    static const P4BattleObserver observer = {.command = command, .hit = hit};
    if (!p4_battle_observer_add(&observer))
        fprintf(stderr, "p4pc: battle trace: no observer slot\n");
}
