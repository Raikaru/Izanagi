/* RE-3 P10 arithmetic; forced Golden modes are NOT the PS2 level argument. */
#ifndef P4PC_GOLDEN_ENCOUNTER_RATES_H
#define P4PC_GOLDEN_ENCOUNTER_RATES_H
#include <stdint.h>
#include <stddef.h>
int32_t p4_golden_encounter_total(const uint8_t *golden_row, unsigned column,
    uint16_t mode, int exclude, int boost);
int32_t p4_golden_encounter_projected_total(const uint8_t *projected_row, unsigned column,
    uint16_t level, int exclude, int boost, int suppress);
/* 68 locally loaded rows, every total-affecting state class. Counts on success:
 * Golden 6528 (includes retry false/true), projected 3264. No payload output. */
int p4_golden_encounter_matrix(const uint8_t *, size_t, const uint8_t *, size_t,
    unsigned *golden_cells, unsigned *projected_cells);
#endif
