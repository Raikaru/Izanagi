/* Mod presentation in battle, held for a unit's turn start or end:
 * p4.battle.announce (capability battle.announce@1), a one-line banner in the
 * skill-name box, optionally held there until the unit's next turn, and
 * p4.battle.say (battle.say@1), a "<speaker>: <line>" message window.
 *
 * Delivery is the retail boss-talk seam (docs/modding/systems/battle-presentation.md):
 * the mirror's btlActionStartInit (0x001A06D0) and btlActionEndInit (0x001AFA50),
 * after their own boss-talk check (config/mirror_fixes.txt), call
 * p4_battle_present_park; when an item waits for the acting unit, it parks the
 * actor in EVENT (0x16) with p4_battle_present_poll as its per-frame callback
 * (+0x440), to resume START (2) or END (0x21) (+0x43C) when the poll returns 0,
 * re-entering the init as retail does after a boss talk. A held banner is
 * re-raised at the frame safe point (P4_MOD_SAFE_PRESENT) whenever the box is
 * idle, and ends when its unit's next turn reaches START. Retail always wins:
 * an item waits while the skill-name box or the message window is busy or a
 * boss talk runs, and the game's own lines replace a held banner while they
 * show. With nothing queued or held the inits run the retail body (one branch on
 * p4_battle_present_pending): nothing is allocated, no packet UID or RNG is
 * consumed. */
#ifndef P4PC_BATTLE_PRESENT_H
#define P4PC_BATTLE_PRESENT_H
#include <stdint.h>

#define P4_PRESENT_TEXT_MAX 40      /* characters of one banner or message line */
#define P4_PRESENT_SPEAKER_MAX 16   /* characters of a speaker name */
#define P4_PRESENT_FRAMES_MIN 16
#define P4_PRESENT_FRAMES_MAX 600
#define P4_PRESENT_QUEUE 32         /* items in all */
#define P4_PRESENT_PER_TURN 4       /* items for one unit at one moment */
#define P4_PRESENT_WAIT 240         /* frames an item waits for a busy box or window before it is dropped */
#define P4_PRESENT_HELD 4           /* held banners in all */

enum { P4_PRESENT_TURN_START = 0, P4_PRESENT_TURN_END = 1 };
enum { P4_PRESENT_BANNER = 0, P4_PRESENT_SAY = 1 };
enum {
    P4_PRESENT_OK = 0,
    P4_PRESENT_NO_BATTLE,   /* no battle is running */
    P4_PRESENT_NO_UNIT,     /* no unit of this battle has that uid */
    P4_PRESENT_FULL,        /* the unit already has P4_PRESENT_PER_TURN items for that moment, or the queue is full */
    P4_PRESENT_BAD_TEXT     /* empty, too long, or not one line of game glyphs */
};

typedef struct P4PresentRequest {
    uint32_t uid;           /* BtlUnit uid (+0xA8) */
    int kind;               /* P4_PRESENT_BANNER or P4_PRESENT_SAY */
    int moment;             /* P4_PRESENT_TURN_START or P4_PRESENT_TURN_END */
    const char *text;       /* the line, game glyphs */
    const char *speaker;    /* say: the name shown, or NULL for the unit's own name */
    int frames;             /* banner: frames shown; say: frames shown, or 0 to wait for the confirm button */
    int hold;               /* banner: after its frames, held in the box (re-raised whenever the
                             * box is idle) until the same unit's next turn starts or it is cleared */
} P4PresentRequest;

/* Items queued or banners held now: the inits' fast path. */
extern int p4_battle_present_pending;

/* Queues an item. owner identifies the mod (for removal); name is its id for
 * log lines and must outlive the item. */
int p4_battle_present_queue(uint32_t owner, const char *name, const P4PresentRequest *request);
/* From the inits (action: guest BtlAction): when an item waits for the action's
 * unit at that moment, parks the action in EVENT as above and returns 1; the
 * init then returns. 0 leaves the action alone. */
int p4_battle_present_park(uint32_t action, int moment);
/* The EVENT callback: nonzero while something is shown or waited for. */
int32_t p4_battle_present_poll(void);
/* Ends this owner's held banners for the unit with that uid (all units: uid 0); the number ended. One on
 * screen fades out. */
int p4_battle_present_clear(uint32_t owner, uint32_t uid);
/* A mod's VM is going away: its waiting items and held banners are dropped (one already on screen finishes). */
void p4_battle_present_remove_owner(uint32_t owner);
/* A short reason for a P4_PRESENT_* refusal. */
const char *p4_battle_present_reason(int status);

#endif
