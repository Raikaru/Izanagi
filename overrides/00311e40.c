#include "p4pc/game_mode.h"
#include "p4pc/golden_fusion_runtime.h"
s32 cmmCommunityHasFlagGate(s32 community)
{
    u32 i;

    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        for (i = 0; i < p4_golden_fusion_gate_count(); ++i)
            if ((community & 0xffff) == p4_golden_fusion_gate_persona(i)) return 1;
        return 0;
    }
    i = 0;
    while (i < 0x17U) {
        if ((community & 0xFFFF) == D_006420A0[i * 4 + 2]) {
            return 1;
        }
        i++;
    }
    return 0;
}
