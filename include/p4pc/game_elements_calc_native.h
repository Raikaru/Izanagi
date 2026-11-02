#ifndef P4PC_GAME_ELEMENTS_CALC_NATIVE_H
#define P4PC_GAME_ELEMENTS_CALC_NATIVE_H
#include "p4pc/game_elements_native.h"
#include "p4pc/table_result.h"
#include <stddef.h>
/* Original script operand/result words remain the game's authoritative script
 * state. Only the owning full-width action frame and explicit term12 target
 * affinity query are additional local arguments; no global last action. */
int32_t p4_game_elements_calculation_run(uint32_t kind, void *source, void *target,
    uint32_t skill_id, int32_t parameter, const P4GameActionFrame *frame,
    const P4GameAffinityQuery *query);
float p4_game_elements_calculation_float(uint32_t term, int32_t parameter,
    const P4GameActionFrame *frame, const P4GameAffinityQuery *query);
void p4_game_elements_formula_physical(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_magic(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_power_tier(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_hit(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_scaled_ratio(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_pass(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_weighted(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_inverse(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_scaled_physical(const P4GameActionFrame *, const P4GameAffinityQuery *);
void p4_game_elements_formula_products(const P4GameActionFrame *, const P4GameAffinityQuery *);
#endif
