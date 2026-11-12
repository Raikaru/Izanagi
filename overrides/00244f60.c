/* 00244f60: retain every original script operand/result word and dispatch order.
 * Only the owning tagged frame/query travels as local explicit arguments. */
#include "p4pc/game_elements_calc_native.h"
s32 datScriptRunCalc(u32 calculationKind, u16 *sourceUnit, u16 *targetUnit, u16 commandId, s32 parameter)
{
    return p4_game_elements_calculation_run(calculationKind, sourceUnit, targetUnit, commandId, parameter, NULL, NULL);
}
static void (*const p4_context_formulas[11])(const P4GameActionFrame *, const P4GameAffinityQuery *) = {
    NULL, p4_game_elements_formula_physical, p4_game_elements_formula_magic,
    p4_game_elements_formula_power_tier, p4_game_elements_formula_hit,
    p4_game_elements_formula_scaled_ratio, p4_game_elements_formula_pass,
    p4_game_elements_formula_weighted, p4_game_elements_formula_inverse,
    p4_game_elements_formula_scaled_physical, p4_game_elements_formula_products
};
int32_t p4_game_elements_calculation_run(uint32_t calculationKind, void *sourceUnit, void *targetUnit,
    uint32_t commandId, int32_t parameter, const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{
    D_0088146C[0] = (u16)commandId;
    D_00881460[0] = sourceUnit;
    D_00881464[0] = targetUnit;
    D_00881470[0] = parameter;
    D_00881474[0] &= 0xFFFE;
    K_ASSERT(calculationKind < 0xB, 0x7A);
    if (frame && (!p4_game_elements_native_enabled() || frame->skill_id != commandId || frame->element.valid > 1u))
        p4_game_elements_native_calculation_refuse(P4_TABLE_INVALID);
    if (calculationKind && (frame || p4_game_elements_native_enabled())) {
        P4GameActionFrame local;
        const P4GameActionFrame *owning = frame;
        /* Stat-only/escape/power-tier formulas do not consume an element: no
         * speculative frame/weapon/affinity reads or source tags for them. */
        if (!owning && (calculationKind == 1u || calculationKind == 2u || calculationKind == 9u)) {
            p4_game_elements_native_primary_frame(sourceUnit, commandId, 0, &local);
            owning = &local;
        } else if (!owning && (calculationKind == 8u || calculationKind == 10u)) {
            const uint32_t payload = (uint32_t)parameter;
            p4_game_elements_native_action_frame(sourceUnit, commandId,
                payload & 0x80000u ? P4_ACTION_COMPONENT_INSTANT_KO : P4_ACTION_COMPONENT_STATUS,
                payload, 0, &local);
            owning = &local;
        }
        p4_context_formulas[calculationKind](owning, query);
    } else {
        /* Kind0 is the REAL retail no-result routine, including its existing
         * result-flag assertion; it is not replaced with a successful no-op. */
        ((void (*)())D_00635950[calculationKind])();
    }
    K_ASSERT(D_00881474[0] & 1, 0x82);
    return D_00881468[0];
}
