/* Post-battle skill-offer prompt (shdPersonaSkillSelectSeq, 0011f5a0): the input
 * the simulator gives the real list and Yes/No window to decline an offered
 * skill. A persona that levels up with eight skills asks which one to forget;
 * its confirmation defaults to No, so a periodic Cross re-selects forever.
 * Declining picks the pending skill itself (list slot 8) and answers Yes, the
 * native "did not learn" path. A known skill is never forgotten. */
#include "p4pc/battle_sim.h"
#include "../pad/pad_sdl.h"

unsigned p4_battle_skill_prompt_buttons(const P4BattleSkillPrompt *prompt)
{
    switch (prompt->state) {
    case 0: case 1: case 6:
        /* Opening and "did not learn" messages: advance text; there is no choice. */
        return prompt->row_waiting ? P4PC_PAD_CROSS : 0;
    case 2:
        if (!prompt->list_active || prompt->list_cursor < 0 || prompt->pending <= 0) return 0;
        return prompt->list_cursor == 8 ? P4PC_PAD_CROSS : P4PC_PAD_LEFT;
    case 3:
        if (prompt->choice_active) {
            /* Yes (0) only for the pending skill; otherwise answer No (1). */
            const int32_t wanted = prompt->replacement_pending ? 0 : 1;
            if (prompt->choice_cursor == wanted) return P4PC_PAD_CROSS;
            return wanted ? P4PC_PAD_DOWN : P4PC_PAD_UP;
        }
        /* Only text rows consume Cross before the choice opens. */
        return prompt->row_waiting ? P4PC_PAD_CROSS : 0;
    default:
        return 0;
    }
}
