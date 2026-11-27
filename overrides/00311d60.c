#include "p4pc/game_mode.h"
#include "p4pc/golden_fusion_runtime.h"
s32 cmmCommunityPassesFlagGate(s32 community)
{
    s16 flags;
    u32 index;
    s32 status;
    s32 value;
    u8 *entry;
    s32 result;

    if (p4_game_mode() == P4_GAME_MODE_GOLDEN) {
        int allowed = 1;
        for (unsigned i = 0; i < p4_golden_fusion_gate_count(); ++i) {
            if ((community & 0xffff) != p4_golden_fusion_gate_persona(i)) continue;
            int bits = p4_golden_fusion_gate_bits(i);
            if (bits & 1) { if (!datGetFlag(p4_golden_fusion_gate_flag(i))) return 0; }
            else if (bits & 2) { allowed = 0; if (datGetFlag(p4_golden_fusion_gate_flag(i)) == 1) return 1; }
        }
        return allowed;
    }
    status = 1;
    index = 0;
    value = community & 0xFFFF;
    goto loop_test;
loop_body:
    entry = (u8 *)D_006420A0 + (index * 8);
    if (value != ((CmmFlagGate *)entry)->community) {
        goto block_8;
    }
    flags = ((CmmFlagGate *)entry)->mode;
    if (flags & 1) {
        if (datGetFlag(((CmmFlagGate *)entry)->flag) == 0) {
            result = 0;
            goto done;
        }
    } else {
        if ((flags & 2) == 0) {
            goto block_8;
        }
        status = 0;
        if (datGetFlag(((CmmFlagGate *)entry)->flag) == 1) {
            result = 1;
            goto done;
        }
    }
block_8:
    index += 1;
loop_test:
    if (index < 0x17U) {
        goto loop_body;
    }
    result = status;
done:
    return result;
}
