/* 00244870: one canonical original equation; only frame/query transport differs. */
#include "p4pc/game_elements_calc_native.h"
#include "datScript_internal.h"
static void p4_formula_hit_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query);
void datScriptCalcHitRate(void)
{ p4_formula_hit_body(NULL, NULL); }
static void p4_formula_hit_body(const P4GameActionFrame *p4_frame, const P4GameAffinityQuery *p4_query)
{
    extern f32 fGpffff824c;
    f32 agilityRatio;
    f32 accuracyTerm;
    f32 hitRate;
    f32 skillAccuracy;

    if (datScriptTargetIsNotEnemy() != 0) {
        accuracyTerm = (200.0f + p4_game_elements_calculation_float(0xD, 3, p4_frame, p4_query));
        hitRate = (200.0f + p4_game_elements_calculation_float(0xE, 3, p4_frame, p4_query));
        accuracyTerm = accuracyTerm / hitRate;
        hitRate = accuracyTerm * p4_game_elements_calculation_float(0x11, 0, p4_frame, p4_query);
    } else {
        accuracyTerm = (200.0f + p4_game_elements_calculation_float(0xD, 3, p4_frame, p4_query));
        hitRate = (200.0f + p4_game_elements_calculation_float(0xE, 3, p4_frame, p4_query));
        agilityRatio = accuracyTerm / hitRate;
        accuracyTerm = p4_game_elements_calculation_float(0x1F, 0, p4_frame, p4_query) / fGpffff824c;
        skillAccuracy = p4_game_elements_calculation_float(0x11, 0, p4_frame, p4_query) - accuracyTerm;
        if (skillAccuracy <= 0.0f)
            skillAccuracy = 1.0f;
        hitRate = agilityRatio * skillAccuracy;
    }
    datScriptSetResult(hitRate);
}
void p4_game_elements_formula_hit(const P4GameActionFrame *frame, const P4GameAffinityQuery *query)
{ p4_formula_hit_body(frame, query); }
