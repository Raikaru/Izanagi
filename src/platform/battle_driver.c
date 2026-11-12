/* P4PC_BATTLE_DRIVER=<plan>: commands in a real battle, driven by the battle's own state the way the title driver drives the title.
 *
 * A plan is a comma list of buttons (up down left right cross circle triangle square l1 r1), pressed 14 flips apart each time a party member's
 * command menu is up. The menu is up when btlMain (the battle context's state word, iGpffffb3ac + 0x2FC) is in state 8, the pending action
 * (ctx + 0x104; its unit is the word at +0x30) belongs to the party (group 0 of the unit lists at ctx + 0x178, linked through unit + 0xA6C),
 * and the context's flag 0x400000 (an action is running) is clear, for P4PC_BATTLE_CMD_DELAY flips (default 20). Every other moment of a fight
 * (tutorial Help windows, enemy turns, the result screens) gets Cross every 24 flips, like the walk script's skipmsg, because a held-back walk
 * script is silent while a plan is set (the hold registered with p4pc_pad_set_walk_hold): its own Cross every 24 flips would pick Attack on every turn.
 *
 *   P4PC_BATTLE_DRIVER=cross,cross,cross   the plan for every command
 *   P4PC_BATTLE_FIRST=down,cross,cross,cross   the plan for a character's first command of a fight (default: the plan). The wheel remembers
 *                       each character's last command within a fight and starts on Attack in the next one, so "Skill, first entry, Zio" is
 *                       down,cross,cross,cross once and cross,cross,cross after (Yu's skill list opens on Zio; the last cross is the target).
 *   P4PC_BATTLE_SKIP=<flips>   Cross only, for the first flips of the fight: the tutorial Help windows of the first fights (140 for the TV world).
 *   P4PC_BATTLE_CMD_DELAY=<flips>   how long the menu must have been up before the plan starts (default 20).
 *   P4PC_BATTLE_LOG=1   every btlMain state change, command menu up/gone and press, with the flip number.
 *
 * Cross-only play is a coin flip: the enemy's first 2% critical knocks the hero down and every hit after it is x1.25 (retail loses the next boss
 * with Cross only too), so a script that has to win a fight sets a plan. Verified: the TV-world tutorial fight (two Slipping Hablerie, Yu 70 HP) is
 * won with BATTLE_FIRST=down,cross,cross,cross BATTLE_DRIVER=cross,cross,cross BATTLE_SKIP=140 (Zio each turn; btlMain 8 -> 11 -> 16 -> 17). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/gmem.h"

extern void *kwlnTaskGetTaskByName(const void *name);
extern void p4pc_pad_inject(unsigned bits, unsigned frames);
extern void p4pc_pad_set_walk_hold(int (*hold)(void));

#define iGpffffb3ac P4_GPDATA(int32_t, 0xb3ac)   /* the battle context (btlTaskCreate) */

enum { PLAN_MAX = 32, PARTY_MAX = 8, CMD_STATE = 8, ACTION_RUNNING = 0x400000 };

static unsigned planBits[PLAN_MAX], firstBits[PLAN_MAX];
static int planLen = -1, firstLen, logOn, owns, commanded[PARTY_MAX];
static unsigned cmdDelay = 20, skipFlips;

static unsigned buttonBit(const char *s, size_t n)
{
    static const struct { const char *name; unsigned bit; } t[] = {
        { "up", 0x0010 }, { "right", 0x0020 }, { "down", 0x0040 }, { "left", 0x0080 }, { "cross", 0x4000 }, { "circle", 0x2000 },
        { "triangle", 0x1000 }, { "square", 0x8000 }, { "l1", 0x0400 }, { "r1", 0x0800 },
    };
    size_t i;

    for (i = 0; i < sizeof t / sizeof t[0]; i++) {
        if (strlen(t[i].name) == n && strncmp(t[i].name, s, n) == 0) {
            return t[i].bit;
        }
    }
    fprintf(stderr, "p4pc: P4PC_BATTLE_DRIVER: unknown button '%.*s'\n", (int)n, s);
    exit(2);
}

static int parsePlan(const char *s, unsigned *bits)
{
    int len = 0;

    while (s != NULL && *s != '\0' && len < PLAN_MAX) {
        const char *c = strchr(s, ',');
        const size_t n = c != NULL ? (size_t)(c - s) : strlen(s);

        bits[len++] = buttonBit(s, n);
        s += n + (c != NULL);
    }
    return len;
}

static int ownsPad(void)
{
    return owns;
}

void p4pc_battle_driver_frame(unsigned flip)
{
    static int32_t seen = -2;
    static unsigned stateStart, waitSince, startAt, step, nextSkip;
    static int acted, lastWaiting, first;
    const uint8_t *c;
    int32_t state;

    if (planLen < 0) {
        const char *e;

        logOn = getenv("P4PC_BATTLE_LOG") != NULL;
        p4pc_pad_set_walk_hold(ownsPad);
        planLen = parsePlan(getenv("P4PC_BATTLE_DRIVER"), planBits);
        firstLen = parsePlan(getenv("P4PC_BATTLE_FIRST"), firstBits);
        if (firstLen == 0) {   /* no separate first-command plan: the plan every time */
            memcpy(firstBits, planBits, sizeof planBits);
            firstLen = planLen;
        }
        if ((e = getenv("P4PC_BATTLE_SKIP")) != NULL) {
            skipFlips = (unsigned)atoi(e);
        }
        if ((e = getenv("P4PC_BATTLE_CMD_DELAY")) != NULL) {
            cmdDelay = (unsigned)atoi(e);
        }
        if (planLen > 0) {
            fprintf(stderr, "p4pc: battle driver on: %d presses per command (%d for a first one), help skip %u flips\n", planLen, firstLen, skipFlips);
        }
    }
    if (kwlnTaskGetTaskByName("battle") == NULL) {
        owns = 0;
        seen = -2;
        lastWaiting = 0;
        memset(commanded, 0, sizeof commanded);   /* the wheel starts on Attack again in the next fight */
        return;
    }
    owns = planLen > 0;
    c = (const uint8_t *)P4_G2H((uint32_t)iGpffffb3ac);
    state = *(const int32_t *)(c + 0x2FC);
    if (state != seen) {
        if (logOn) {
            fprintf(stderr, "p4pc: battle driver: flip %u btlMain state %d -> %d\n", flip, (int)seen, (int)state);
        }
        seen = state;
        stateStart = flip;
    }
    if (planLen == 0) {
        return;
    }
    {
        const uint32_t actor = *(const uint32_t *)(c + 0x104), flags = *(const uint32_t *)(c + 0xC);
        const uint32_t actorUnit = actor > 0x100000u && actor < 0x02000000u ? *(const uint32_t *)P4_G2H(actor + 0x30u) : 0;
        int partyIndex = -1, n;
        uint32_t u;
        int waiting;

        for (n = 0, u = *(const uint32_t *)(c + 0x178); u > 0x100000u && u < 0x02000000u && n < PARTY_MAX; n++, u = *(const uint32_t *)P4_G2H(u + 0xA6Cu)) {
            if (u == actorUnit) {
                partyIndex = n;
            }
        }
        waiting = state == CMD_STATE && partyIndex >= 0 && !(flags & ACTION_RUNNING) && flip - stateStart >= skipFlips;
        if (waiting != lastWaiting) {
            if (logOn) {
                fprintf(stderr, "p4pc: battle driver: flip %u command menu %s (actor %08x party %d flags %08x)\n", flip, waiting ? "up" : "gone", (unsigned)actor,
                        partyIndex, (unsigned)flags);
            }
            lastWaiting = waiting;
            waitSince = flip;
            acted = 0;
        }
        if (!waiting) {
            if (flip >= nextSkip) {   /* Help windows, enemy turns, results */
                p4pc_pad_inject(0x4000, 5);
                nextSkip = flip + 24;
            }
            return;
        }
        if (!acted && flip - waitSince >= cmdDelay) {
            acted = 1;
            startAt = flip;
            step = 0;
            first = partyIndex < PARTY_MAX && !commanded[partyIndex];
            commanded[partyIndex] = 1;
        }
        {
            const unsigned *bits = first ? firstBits : planBits;
            const int len = first ? firstLen : planLen;

            if (acted && step >= (unsigned)len && flip - startAt > 600) {   /* the plan did not take: again */
                acted = 0;
                waitSince = flip - cmdDelay;
            }
            if (acted && step < (unsigned)len && flip >= startAt + step * 14u) {
                p4pc_pad_inject(bits[step], 5);
                if (logOn) {
                    fprintf(stderr, "p4pc: battle driver: flip %u press %u of %d (%04x)\n", flip, step + 1, len, bits[step]);
                }
                step++;
            }
        }
    }
}
