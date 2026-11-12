#include "p4pc/game_elements_native.h"
#include "p4pc/game_knowledge_native.h"

/* Expanded-route enemy-knowledge learning at 001F14F0's eligible-outcome
 * gate. The frames are deterministic re-resolutions of the ones this row's
 * resolvers queried (0023E6F0/002411A0 primary, 00242990 instant KO), so
 * observing them equals observing what was resolved. No retail bitmap write
 * happens here: the knowledge owner projects original-resolver leaves. */
void p4_game_elements_native_learn(void *defender, void *attacker, uint32_t skill_id, uint32_t ko_status)
{
    P4GameActionFrame frame;
    p4_game_elements_native_primary_frame(attacker, skill_id, 0, &frame);
    int result = p4_game_knowledge_native_observe(defender, &frame);
    if (!result && (ko_status & 0x80000u)) {
        p4_game_elements_native_action_frame(attacker, skill_id, P4_ACTION_COMPONENT_INSTANT_KO, 0x80000u, 0, &frame);
        result = p4_game_knowledge_native_observe(defender, &frame);
    }
    if (result) p4_game_elements_native_calculation_refuse(result);
}
