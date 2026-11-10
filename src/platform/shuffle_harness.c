/* Shuffle Time harness: P4PC_SHUFFLE=<pattern>:<variant>, with P4PC_START_BATTLE (src/platform/battle_harness.c), ends the harness battle in
 * Shuffle Time with that pattern (0..4: btlShuffleSeq1Run..btlShuffleSeq5Run, dispatched by btlShuffleSeqCardTask 00379b70) and variant, and
 * exits 0 once the game's own shuffle task has run its sequence to the end and been destroyed (tests/shuffle_gate.sh).
 *
 * The game draws whether Shuffle Time happens, its pattern, its variant and its card count from retail tables: btlEncountRollChance
 * (001ef7e0) rolls the chance for the protagonist's turn count, btlShuffleSelectMember (0036ee60) the pattern
 * (btlShuffleRollBelow 0036e920, cumulative weights per level decade), the variant (btlShuffleRollFlagged 0036eb50, only once the pattern's
 * flag is set) and the card count (btlShuffleRollCard 0036ea00, from the pattern). The harness only rewrites those weights in guest memory
 * before the battle, so the code that runs, and the card count and cards that follow from the rolls, stay the game's:
 *   - the chance by protagonist turns (u8[5] at 0x00763AA0, retail 50 25 20 20 20 percent) becomes 100 percent;
 *   - every decade's pattern weights (u8[10][5] at 0x0064E610) become 100 for <pattern>;
 *   - <variant> `first`: Shuffle Time and the pattern are new to the player. Flag 0x1430 (the introduction windows of btlShuffleTaskUpdate)
 *     and the pattern's flag (0x1431 patterns 0-2, 0x1432 pattern 3, 0x1433 pattern 4: its tutorial; the variants stay locked) are cleared,
 *     so the variant is 0;
 *   - <variant> N: both flags are set (introduction and tutorial seen) and every decade's variant weights of the pattern (u8[10][3] at
 *     0x0064E650 for patterns 0-2, u8[10][4] at 0x0064E670 for pattern 3, u8[10][3] at 0x0064E6A0 for pattern 4) become 100 for N.
 * The gate fails when the shuffle task reports another pattern or variant than the one asked for, or is destroyed before its sequence
 * ended (BtlShuffleTaskWork phase 6). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/crt.h"
#include "p4pc/gmem.h"

extern void *kwlnTaskGetTaskByName(const void *name);
extern void datSetFlag(int32_t flag, int32_t value);

#define SHUFFLE_CHANCE 0x00763AA0u     /* iGpffffa9b0: u8[5], the chance by protagonist turns (btlEncountRollChance) */
#define PATTERN_WEIGHTS 0x0064E610u    /* D_0064E610: u8[10][5] */
#define TASK_NAME 0x0064E780u          /* D_0064E780: "battle_shuffle" (btlShuffleTaskCreate) */
/* BtlShuffleTaskWork (btl_shuffle_task_internal.h): phase at +8, the draw context (BtlShuffleContext) at +0x70 */
#define WORK_PHASE 0x8u
#define CONTEXT 0x70u
#define CONTEXT_PHASE (CONTEXT + 0x1F2F8u)
#define CONTEXT_PATTERN (CONTEXT + 0x1F2FCu)
#define CONTEXT_VARIANT (CONTEXT + 0x1F300u)
#define CONTEXT_CARDS (CONTEXT + 0x1F304u)
#define SEQUENCE_ENDED 6               /* btlShuffleTaskUpdate: btlShuffleSeqCardTask returned nonzero */

static const struct {
    uint32_t weights, columns;
    int32_t flag;
} kVariants[5] = {
    {0x0064E650u, 3, 0x1431}, {0x0064E650u, 3, 0x1431}, {0x0064E650u, 3, 0x1431}, {0x0064E670u, 4, 0x1432}, {0x0064E6A0u, 3, 0x1433},
};

static int sPattern = -1, sVariant = -1;   /* sVariant -1: `first` */
static int sSeen, sEnded;
static int32_t sPhase = -1;
static uint32_t sSeqPhase = 0xFFFFFFFFu;

static void fail(const char *why)
{
    fprintf(stderr, "p4pc: shuffle gate: FAIL: %s\n", why);
    fflush(stderr);
    p4pc_exit(1);
}

static void setWeights(uint32_t table, uint32_t columns, uint32_t chosen)
{
    uint8_t *w = P4_GARRAY(uint8_t, table);
    uint32_t decade, i;

    for (decade = 0; decade < 10; decade++)
        for (i = 0; i < columns; i++)
            w[decade * columns + i] = i == chosen ? 100 : 0;
}

/* Called by the battle harness after the new-game state and before the encounter starts. */
void p4pc_shuffle_harness_start(void)
{
    const char *s = getenv("P4PC_SHUFFLE");
    const char *variant;
    char *end;
    long p;
    uint8_t *chance;
    int i;

    if (s == NULL || *s == '\0') {
        return;
    }
    p = strtol(s, &end, 10);
    if (end == s || *end != ':' || p < 0 || p > 4) {
        fprintf(stderr, "p4pc: P4PC_SHUFFLE=%s: want <pattern 0..4>:<first|variant>\n", s);
        exit(2);
    }
    sPattern = (int)p;
    variant = end + 1;
    if (strcmp(variant, "first") != 0) {
        long n = strtol(variant, &end, 10);
        if (end == variant || *end != '\0' || n < 0 || (uint32_t)n >= kVariants[sPattern].columns) {
            fprintf(stderr, "p4pc: P4PC_SHUFFLE=%s: pattern %d has variants 0..%u\n", s, sPattern, (unsigned)kVariants[sPattern].columns - 1);
            exit(2);
        }
        sVariant = (int)n;
    }
    chance = P4_GARRAY(uint8_t, SHUFFLE_CHANCE);
    for (i = 0; i < 5; i++) {
        chance[i] = 100;
    }
    setWeights(PATTERN_WEIGHTS, 5, (uint32_t)sPattern);
    datSetFlag(0x1430, sVariant >= 0);
    datSetFlag(kVariants[sPattern].flag, sVariant >= 0);
    if (sVariant >= 0) {
        setWeights(kVariants[sPattern].weights, kVariants[sPattern].columns, (uint32_t)sVariant);
    }
    fprintf(stderr, "p4pc: shuffle gate: pattern %d variant %s: chance 100, flags 0x1430 and 0x%x %s\n", sPattern, variant,
            (unsigned)kVariants[sPattern].flag, sVariant < 0 ? "clear" : "set");
}

/* Every flip once the harness battle has started: follow the shuffle task, exit when it is gone. */
void p4pc_shuffle_harness_frame(unsigned frame)
{
    uint8_t *task, *work;
    int32_t phase;
    uint32_t seqPhase;

    if (sPattern < 0) {
        return;
    }
    task = (uint8_t *)kwlnTaskGetTaskByName(P4_GARRAY(char, TASK_NAME));
    if (task == NULL) {
        if (sSeen) {
            if (!sEnded) {
                fail("the shuffle task was destroyed before its sequence ended");
            }
            fprintf(stderr, "p4pc: shuffle gate: pattern %d variant %d: sequence ended and the task destroyed at frame %u: PASS\n", sPattern,
                    sVariant < 0 ? 0 : sVariant, frame);
            fflush(stderr);
            p4pc_exit(0);
        }
        return;
    }
    work = (uint8_t *)P4_G2H(*(uint32_t *)(task + 0x38));
    if (work == NULL) {
        return;
    }
    if (!sSeen) {
        int32_t pattern = *(int32_t *)(work + CONTEXT_PATTERN), variant = *(int32_t *)(work + CONTEXT_VARIANT);
        sSeen = 1;
        fprintf(stderr, "p4pc: shuffle gate: frame %u: shuffle task, pattern %d variant %d cards %d\n", frame, (int)pattern, (int)variant,
                (int)*(int32_t *)(work + CONTEXT_CARDS));
        if (pattern != sPattern || variant != (sVariant < 0 ? 0 : sVariant)) {
            fail("the shuffle task has another pattern or variant than the one asked for");
        }
    }
    phase = *(int32_t *)(work + WORK_PHASE);
    seqPhase = *(uint32_t *)(work + CONTEXT_PHASE);
    if (phase != sPhase || seqPhase != sSeqPhase) {
        fprintf(stderr, "p4pc: shuffle gate: frame %u: task phase %d, sequence phase %u\n", frame, (int)phase, (unsigned)seqPhase);
        sPhase = phase;
        sSeqPhase = seqPhase;
    }
    if (phase == SEQUENCE_ENDED) {
        sEnded = 1;
    }
}
