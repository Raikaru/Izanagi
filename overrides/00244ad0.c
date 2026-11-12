/* 00244ad0: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_pass_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datCalcFormulaPassCalc15(void)
{ p4_formula_pass_body(NULL, NULL); }
static void p4_formula_pass_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    datScriptTargetIsNotEnemy();
    datScriptSetResult(p4_game_elements_calculation_float(0x15, 0, p4_frame, p4_query));
}
void p4_game_elements_formula_pass(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_pass_body(frame, query); }
