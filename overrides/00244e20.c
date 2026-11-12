/* 00244e20: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_products_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datCalcFormulaRatioOfProducts(void)
{ p4_formula_products_body(NULL, NULL); }
static void p4_formula_products_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    f32 numerator;
    f32 result;
    f32 quotient;

    if (datScriptTargetIsNotEnemy() != 0) {
        numerator = p4_game_elements_calculation_float(0x12, 0, p4_frame, p4_query);
        numerator *= p4_game_elements_calculation_float(0x10, 0, p4_frame, p4_query);
        result = p4_game_elements_calculation_float(0xD, 4, p4_frame, p4_query);
        result = operandOrderAddFloat(100.0f, result);
        numerator *= result;
        result = p4_game_elements_calculation_float(0xE, 4, p4_frame, p4_query);
        result = operandOrderAddFloat(100.0f, result);
        result = numerator / result;
    } else {
        numerator = p4_game_elements_calculation_float(0x12, 0, p4_frame, p4_query);
        numerator *= p4_game_elements_calculation_float(0x10, 0, p4_frame, p4_query);
        result = p4_game_elements_calculation_float(0xD, 4, p4_frame, p4_query);
        result = operandOrderAddFloat(100.0f, result);
        numerator *= result;
        result = (100.0f + p4_game_elements_calculation_float(0xE, 4, p4_frame, p4_query));
        quotient = numerator / result;
        result = fGpffff811c * quotient;
    }
    datScriptSetResult(result);
}
void p4_game_elements_formula_products(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_products_body(frame, query); }
