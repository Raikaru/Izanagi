/* 002447c0: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_power_tier_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datCalcFormulaTerms13And1A(void)
{ p4_formula_power_tier_body(NULL, NULL); }
static void p4_formula_power_tier_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    f32 result;
    f32 extraTerm;

    if (datScriptTargetIsNotEnemy() != 0) {
        result = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
        result += p4_game_elements_calculation_float(0x1A, 0, p4_frame, p4_query);
    } else {
        extraTerm = p4_game_elements_calculation_float(0x1A, 0, p4_frame, p4_query);
        result = D_00761260 * extraTerm + p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
    }
    datScriptSetResult(result);
}
void p4_game_elements_formula_power_tier(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_power_tier_body(frame, query); }
