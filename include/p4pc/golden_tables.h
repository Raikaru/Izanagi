/* Golden battle tables translated to the PS2 layouts, as row replacements
 * over the selected (disc, developer or package) base input.
 *
 * Only rows that differ from the base are produced, so the Golden mechanics
 * package owns as few rows as possible. Golden-only content (skills in PS2
 * blank slots, Golden-only items, Golden enemies) never leaves this layer in
 * phase 1; see docs/modding/golden.md for the per-group rules. ENCOUNT
 * formations may also name IDs after the base rows (Golden random formations
 * whose IDs PS2 uses for other rows); the producer grows that block. */
#ifndef P4PC_GOLDEN_TABLES_H
#define P4PC_GOLDEN_TABLES_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_table_inputs.h"
#include "p4pc/golden_source.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Groups of the p4pc.golden package (its [setting] keys). */
enum {
    P4_GOLDEN_GROUP_NUMBERS = 1u << 0, /* skill rows, compendium learnsets, equipment and item prices */
    P4_GOLDEN_GROUP_ENEMIES = 1u << 1, /* enemy stats, affinities, rewards; PS2 skill lists for PS2 AI */
    P4_GOLDEN_GROUP_PARTY   = 1u << 2, /* party Personas: base rows, learnsets, affinities */
    P4_GOLDEN_GROUP_EXP     = 1u << 3, /* party EXP curves */
    P4_GOLDEN_GROUP_ENCOUNTERS = 1u << 4, /* random encounters and dungeon treasure (ENCOUNT 0, 1, 3, 4) */
};

/* The would-be-selected base span (packed spans start after the length word;
 * ITEM span 0 is the whole ITEMTBL). 0, or -1 when absent. */
typedef int (*P4GoldenBaseFn)(void *user, P4GameTableInputKind input, uint32_t span,
                              const uint8_t **bytes, size_t *size);

typedef struct P4GoldenRows {
    P4GameTableInputKind input;
    uint32_t span;
    uint32_t row_size;
    size_t count;
    uint32_t *ids;   /* strictly ascending */
    uint8_t *bytes;  /* count * row_size, exact PS2 layout */
} P4GoldenRows;

#define P4_GOLDEN_MAX_ROW_SETS 16u

typedef struct P4GoldenBuild {
    P4GoldenRows sets[P4_GOLDEN_MAX_ROW_SETS];
    size_t count;
} P4GoldenBuild;

/* Builds the row replacements for the selected groups. 0 built (possibly
 * empty), -1 when a base span is missing or not the retail layout the
 * translation was measured against (reason set). */
int p4_golden_build(const P4GoldenSource *source, unsigned groups, P4GoldenBaseFn base, void *user,
                    P4GoldenBuild *out, char *reason, size_t capacity);
void p4_golden_build_free(P4GoldenBuild *build);

#ifdef __cplusplus
}
#endif
#endif
