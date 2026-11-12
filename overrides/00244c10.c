/* 00244c10: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_inverse_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datCalcFormulaInverseRatio(void)
{ p4_formula_inverse_body(NULL, NULL); }
static void p4_formula_inverse_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    f32 result;

    result = 3.0f * (p4_game_elements_calculation_float(1, 0, p4_frame, p4_query) - 1.0f) + 2.0f;
    result *= p4_game_elements_calculation_float(0x10, 0, p4_frame, p4_query);
    if (result > 0.0f) {
        result = 50.0f * (p4_game_elements_calculation_float(0xD, 4, p4_frame, p4_query) / result);
    } else {
        result = 1.0f;
    }
    datScriptSetResult(result);
}
void p4_game_elements_formula_inverse(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_inverse_body(frame, query); }
