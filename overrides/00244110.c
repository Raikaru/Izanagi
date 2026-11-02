/* 00244110: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_physical_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datScriptCalcPhysicalDamage(void)
{ p4_formula_physical_body(NULL, NULL); }
static void p4_formula_physical_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    extern s32 datScriptCmdFlag2Set(void);
    extern f32 sqrtf(f32);
    /* fGpffff82c0: multiplier on the flag-2 branch of the enemy-target physical formula. */
    extern f32 fGpffff82c0;
    /* fGpffff824c: scale on the clamped result, after the 30-point offset. */
    extern f32 fGpffff824c;
    f32 damageTerm;
    f32 scaledTerm;
    f32 powerTerm;
    f32 levelTerm;
    f32 divisor;

    if (datScriptCmdFlag2Set() != 0) {
        if (datScriptTargetIsNotEnemy() != 0) {
            damageTerm = p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
            damageTerm += p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
            powerTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query) / 2.0f;
            powerTerm = operandOrderMultiply(30.0f, powerTerm);
            scaledTerm = 5.0f * damageTerm;
            damageTerm = powerTerm / scaledTerm;
            scaledTerm = p4_game_elements_calculation_float(0xD, 0, p4_frame, p4_query) / 5.0f;
            scaledTerm = operandOrderMultiply(damageTerm, scaledTerm);
            powerTerm = sqrtf(scaledTerm);
            damageTerm = 10.0f * powerTerm;
            damageTerm *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_TARGET_AFFINITY_PCT, 0, p4_frame, p4_query);
            damageTerm *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_LEVEL_FACTOR, 0, p4_frame, p4_query);
        } else {
            damageTerm = p4_game_elements_calculation_float(0x20, 0, p4_frame, p4_query) / 4.0f;
            divisor = 2.0f;
            scaledTerm = p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
            levelTerm = damageTerm / divisor;
            powerTerm = levelTerm + divisor * scaledTerm;
            scaledTerm = fGpffff811c * powerTerm;
            damageTerm = scaledTerm;
            powerTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
            powerTerm = operandOrderMultiply(30.0f, powerTerm);
            scaledTerm = operandOrderMultiply(5.0f, damageTerm);
            damageTerm = powerTerm / scaledTerm;
            scaledTerm = p4_game_elements_calculation_float(0xD, 0, p4_frame, p4_query) / 5.0f;
            scaledTerm = operandOrderMultiply(damageTerm, scaledTerm);
            powerTerm = sqrtf(scaledTerm);
            damageTerm = 10.0f * powerTerm;
            damageTerm *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_TARGET_AFFINITY_PCT, 0, p4_frame, p4_query);
            powerTerm = damageTerm * p4_game_elements_calculation_float(DAT_SCRIPT_TERM_LEVEL_FACTOR, 0, p4_frame, p4_query);
            damageTerm = fGpffff82c0 * powerTerm;
        }
    } else {
        if (datScriptTargetIsNotEnemy() != 0) {
            damageTerm = p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
            damageTerm += p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
            powerTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
            powerTerm = operandOrderMultiply(30.0f, powerTerm);
            scaledTerm = 5.0f * damageTerm;
            damageTerm = powerTerm / scaledTerm;
            scaledTerm = p4_game_elements_calculation_float(0xD, 0, p4_frame, p4_query) / 5.0f;
            scaledTerm = operandOrderMultiply(damageTerm, scaledTerm);
            powerTerm = sqrtf(scaledTerm);
            damageTerm = 10.0f * powerTerm;
            damageTerm *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_TARGET_AFFINITY_PCT, 0, p4_frame, p4_query);
            damageTerm *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_LEVEL_FACTOR, 0, p4_frame, p4_query);
        } else {
            damageTerm = p4_game_elements_calculation_float(0x20, 0, p4_frame, p4_query) / 2.0f;
            divisor = 2.0f;
            scaledTerm = p4_game_elements_calculation_float(0xE, 2, p4_frame, p4_query);
            levelTerm = damageTerm / divisor;
            powerTerm = levelTerm + divisor * scaledTerm;
            scaledTerm = fGpffff811c * powerTerm;
            damageTerm = scaledTerm;
            powerTerm = p4_game_elements_calculation_float(DAT_SCRIPT_TERM_SKILL_POWER, 0, p4_frame, p4_query);
            powerTerm = operandOrderMultiply(30.0f, powerTerm);
            scaledTerm = operandOrderMultiply(5.0f, damageTerm);
            damageTerm = powerTerm / scaledTerm;
            scaledTerm = p4_game_elements_calculation_float(0xD, 0, p4_frame, p4_query) / 5.0f;
            scaledTerm = operandOrderMultiply(damageTerm, scaledTerm);
            powerTerm = sqrtf(scaledTerm);
            damageTerm = 10.0f * powerTerm;
            powerTerm = damageTerm * p4_game_elements_calculation_float(DAT_SCRIPT_TERM_LEVEL_FACTOR, 0, p4_frame, p4_query);
            damageTerm = powerTerm - 30.0f;
            if (damageTerm <= 0.0f)
                damageTerm = 1.0f;
            damageTerm *= fGpffff824c;
            damageTerm *= p4_game_elements_calculation_float(DAT_SCRIPT_TERM_TARGET_AFFINITY_PCT, 0, p4_frame, p4_query);
        }
    }
    datScriptSetResult(damageTerm);
}
void p4_game_elements_formula_physical(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_physical_body(frame, query); }
