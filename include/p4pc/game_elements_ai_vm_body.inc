#ifndef P4PC_GAME_ELEMENTS_AI_VM_BODY_INC
#define P4PC_GAME_ELEMENTS_AI_VM_BODY_INC
#include "p4pc/game_elements_ai_native.h"
#include "p4pc/gmem.h"

/* One explicitly owned scratch object per scoped interpreter invocation, not a
 * per-op/per-action allocation and not unknown padding in a real script work.
 * Only the scalar numeric prefix is read by the canonical private primitives. */
typedef struct P4ScrAiScratch {
    ScrScriptWork work;
    uint32_t saved_temporaries[8];
} P4ScrAiScratch;
static void p4_scr_ai_shadow(void *scratch_pointer, uint32_t opcode, uint32_t count,
    uint32_t left_type, const uint32_t *left_words, uint32_t right_type,
    const uint32_t *right_words, uint32_t *out_type, uint32_t *out_words,
    uint32_t *invalid_mask)
{
    P4ScrAiScratch *scratch = (P4ScrAiScratch *)scratch_pointer;
    uint32_t i, value_type;
    if (!scratch || !count || count > P4_GAME_AI_CANDIDATE_COUNT || left_type > 1u ||
        ((opcode != 18u && opcode != 19u && opcode != 32u && opcode != 33u) && right_type > 1u)) p4_game_elements_ai_native_fail();
    /* Actual 0029B1B0 temporaries, proved at7645e0..7645fc. Save the POST-actual
     * operator state once and restore it after all virtual numeric evaluations. */
    for (i = 0; i < 8u; ++i) scratch->saved_temporaries[i] = P4_G32(0x007645e0u + i * 4u);
    for (i = 0; i < count; ++i) {
        out_words[i] = 0;
        if (*invalid_mask & (1u << i)) continue;
        scratch->work.pc = 0;
        scratch->work.sp = 1;
        scratch->work.valueTypes[0] = left_type;
        scratch->work.stackValues[0].iVal = left_words[i];
        /* A virtual candidate can encounter an undefined/asserting operation
         * that the actually executed scalar path did not. Such a candidate is
         * ineligible; never run an extra assertion, callback or RNG operation. */
        if ((opcode == 18u && left_type == 0u && left_words[i] == 0x80000000u) ||
            (opcode == 32u && left_type == 1u &&
                !(scratch->work.stackValues[0].fVal >= -2147483648.0f &&
                  scratch->work.stackValues[0].fVal < 2147483648.0f))) {
            *invalid_mask |= 1u << i;
            continue;
        }
        if (opcode == 18u) p4_scr_neg_base(&scratch->work);
        else if (opcode == 19u) p4_scr_not_base(&scratch->work);
        else if (opcode == 32u) {
            scratch->work.stackValues[0].iVal = scrPopInt(&scratch->work);
            scratch->work.valueTypes[0] = 0;
        } else if (opcode == 33u) {
            scratch->work.stackValues[0].fVal = scrPopFloat(&scratch->work);
            scratch->work.valueTypes[0] = 1;
        } else {
            if (!((opcode >= 14u && opcode <= 17u) || (opcode >= 20u && opcode <= 27u))) p4_game_elements_ai_native_fail();
            scratch->work.sp = 2;
            scratch->work.valueTypes[1] = left_type;
            scratch->work.stackValues[1].iVal = left_words[i];
            scratch->work.valueTypes[0] = right_type;
            scratch->work.stackValues[0].iVal = right_words[i];
            if (opcode == 17u &&
                ((right_type == 0u && right_words[i] == 0u) ||
                 (right_type == 1u && scratch->work.stackValues[0].fVal == 0.0f) ||
                 (left_type == 0u && right_type == 0u && left_words[i] == 0x80000000u && right_words[i] == 0xffffffffu))) {
                *invalid_mask |= 1u << i;
                continue;
            }
            p4_scr_binary_base(&scratch->work, opcode < 18u ? opcode - 14u : opcode - 16u);
        }
        value_type = (uint8_t)scratch->work.valueTypes[0];
        if (value_type != *out_type) p4_game_elements_ai_native_fail();
        out_words[i] = (uint32_t)scratch->work.stackValues[0].iVal;
    }
    for (i = 0; i < 8u; ++i) P4_G32(0x007645e0u + i * 4u) = scratch->saved_temporaries[i];
}
#endif
