#include "p4pc/game_mode.h"
#include "p4pc/golden_fusion_runtime.h"
static inline u8 fclNormalPairArcana(s32 first, s32 second)
{
    if (first == 0 || second == 0)
        kAssertFail(D_00642F30, 0x90);
    if (p4_game_mode() == P4_GAME_MODE_GOLDEN)
        return (u8)p4_golden_fusion_chart(0, first, second);
    if (first < second)
        return *(second + (D_00642147 + first * 24));
    return *(first + (D_00642147 + second * 24));
}
