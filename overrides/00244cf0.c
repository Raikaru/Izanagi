/* 00244cf0: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_scaled_physical_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datScriptCalcPhysicalDamageScaled(void)
{ p4_formula_scaled_physical_body(NULL, NULL); }
static void p4_formula_scaled_physical_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    extern f32 sqrtf(f32);
    f32 damage;
    f32 scaledTerm;
    f32 powerTerm;

    damage = p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
    damage += p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
    scaledTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
    powerTerm = scaledTerm / 2.0f;
    scaledTerm = 30.0f * powerTerm;
    powerTerm = 5.0f * damage;
    damage = scaledTerm / powerTerm;
    scaledTerm = p4_game_elements_calculation_float(0xD, 0, p4_frame, p4_query);
    scaledTerm = scaledTerm / 5.0f;
    scaledTerm = operandOrderMultiply(damage, scaledTerm);
    powerTerm = sqrtf(scaledTerm);
    damage = 10.0f * powerTerm;
    damage *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_TARGET_AFFINITY_PCT, 0, p4_frame, p4_query);
    damage *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_LEVEL_FACTOR, 0, p4_frame, p4_query);
    damage *= p4_game_elements_calculation_float(0x1B, 0, p4_frame, p4_query);
    powerTerm = damage * p4_game_elements_calculation_float(0x21, 0, p4_frame, p4_query);
    scaledTerm = powerTerm * fGpffff811c;
    datScriptSetResult(scaledTerm);
}
void p4_game_elements_formula_scaled_physical(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_scaled_physical_body(frame, query); }
