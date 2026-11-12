/* 00244540: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_magic_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datScriptCalcMagicDamage(void)
{ p4_formula_magic_body(NULL, NULL); }
static void p4_formula_magic_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    extern f32 sqrtf(f32);
    /* fGpffff824c: scale on the clamped result, after the 30-point offset. */
    extern f32 fGpffff824c;
    f32 damageTerm;
    f32 baseDamage;
    f32 scaledTerm;
    f32 powerTerm;
    f32 combinedTerm;
    f32 divisor;

    if (datScriptTargetIsNotEnemy() != 0) {
        damageTerm = p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
        damageTerm += p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
        powerTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
        powerTerm = operandOrderMultiply(30.0f, powerTerm);
        scaledTerm = 5.0f * damageTerm;
        damageTerm = powerTerm / scaledTerm;
        scaledTerm = p4_game_elements_calculation_float(0xD, 1, p4_frame, p4_query) / 5.0f;
        scaledTerm = operandOrderMultiply(damageTerm, scaledTerm);
        powerTerm = sqrtf(scaledTerm);
        baseDamage = 10.0f * powerTerm;
        baseDamage *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_TARGET_AFFINITY_PCT, 0, p4_frame, p4_query);
        damageTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_LEVEL_FACTOR, 0, p4_frame, p4_query);
        scaledTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_MAGIC_TIER_BONUS, 0, p4_frame, p4_query);
        damageTerm = scaledTerm + baseDamage * damageTerm;
    } else {
        damageTerm = p4_game_elements_calculation_float(0x20, 0, p4_frame, p4_query) / 2.0f;
        divisor = 2.0f;
        scaledTerm = p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
        combinedTerm = damageTerm / divisor;
        powerTerm = combinedTerm + divisor * scaledTerm;
        scaledTerm = fGpffff811c * powerTerm;
        damageTerm = scaledTerm;
        powerTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
        powerTerm = operandOrderMultiply(30.0f, powerTerm);
        scaledTerm = operandOrderMultiply(5.0f, damageTerm);
        damageTerm = powerTerm / scaledTerm;
        scaledTerm = p4_game_elements_calculation_float(0xD, 1, p4_frame, p4_query) / 5.0f;
        scaledTerm = operandOrderMultiply(damageTerm, scaledTerm);
        powerTerm = sqrtf(scaledTerm);
        baseDamage = 10.0f * powerTerm;
        damageTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_LEVEL_FACTOR, 0, p4_frame, p4_query);
        scaledTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_MAGIC_TIER_BONUS, 0, p4_frame, p4_query);
        combinedTerm = scaledTerm + baseDamage * damageTerm;
        scaledTerm = 30.0f;
        damageTerm = combinedTerm - scaledTerm;
        if (damageTerm <= 0.0f)
            damageTerm = 1.0f;
        damageTerm *= 2.0f;
        damageTerm *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_TARGET_AFFINITY_PCT, 0, p4_frame, p4_query);
    }
    datScriptSetResult(damageTerm);
}
void p4_game_elements_formula_magic(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_magic_body(frame, query); }
