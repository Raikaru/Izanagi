/* 002449c0: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_scaled_ratio_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datCalcFormulaScaledRatio(void)
{ p4_formula_scaled_ratio_body(NULL, NULL); }
static void p4_formula_scaled_ratio_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    f32 ratio;
    f32 result;

    if (datScriptTargetIsNotEnemy() != 0) {
        ratio = (50.0f + p4_game_elements_calculation_float(0xD, 4, p4_frame, p4_query)) /
                   (50.0f + p4_game_elements_calculation_float(0xE, 4, p4_frame, p4_query));
        result = p4_game_elements_calculation_float(0x14, 0, p4_frame, p4_query);
        result *= ratio;
    } else {
        ratio = (50.0f + p4_game_elements_calculation_float(0xD, 4, p4_frame, p4_query)) /
                   (50.0f + p4_game_elements_calculation_float(0xE, 4, p4_frame, p4_query));
        result = fGpffff811c * operandOrderMultiply(p4_game_elements_calculation_float(0x14, 0, p4_frame, p4_query), ratio);
    }
    datScriptSetResult(result);
}
void p4_game_elements_formula_scaled_ratio(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_scaled_ratio_body(frame, query); }
