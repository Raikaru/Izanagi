/* 00244b10: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_weighted_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datCalcFormulaWeightedRatio(void)
{ p4_formula_weighted_body(NULL, NULL); }
static void p4_formula_weighted_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    f32 firstRatio;
    f32 result;

    result = 100.0f + p4_game_elements_calculation_float(0xD, 3, p4_frame, p4_query);
    firstRatio = result / (100.0f + p4_game_elements_calculation_float(0xE, 3, p4_frame, p4_query));
    result = 100.0f + p4_game_elements_calculation_float(0xD, 4, p4_frame, p4_query);
    result = 75.0f * (firstRatio * (result / (100.0f + p4_game_elements_calculation_float(0xE, 4, p4_frame, p4_query))));
    result += 5.0f * p4_game_elements_calculation_float(0x22, 0, p4_frame, p4_query);
    datScriptSetResult(result);
}
void p4_game_elements_formula_weighted(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_weighted_body(frame, query); }
