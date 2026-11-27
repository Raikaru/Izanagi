#include "p4pc/golden_fusion_runtime.h"
#include <assert.h>
#include <stdint.h>
typedef int32_t s32;
typedef int16_t s16;
typedef uint32_t u32;
typedef uint8_t u8;
typedef struct CmmFlagGate { s32 flag; s16 community; s16 mode; } CmmFlagGate;
static s16 D_006420A0[23 * 4];
static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static int flag_value;
static s32 datGetFlag(s32 flag) { assert(flag == 42); return flag_value; }
#include "../overrides/00311d60.c"
#include "../overrides/00311e40.c"
int main(void)
{
    uint8_t charts[3 * 28 * 28] = {0}, gates[26 * 8] = {0};
    CmmFlagGate *retail = (CmmFlagGate *)D_006420A0;
    retail[22].flag = 42; retail[22].community = 55; retail[22].mode = 1;
    mode = P4_GAME_MODE_PS2;
    assert(cmmCommunityHasFlagGate(55));
    flag_value = 0; assert(!cmmCommunityPassesFlagGate(55));
    flag_value = 1; assert(cmmCommunityPassesFlagGate(55));
    gates[25 * 8] = 42; gates[25 * 8 + 4] = 56; gates[25 * 8 + 6] = 1;
    assert(!p4_golden_fusion_install(P4_GAME_MODE_GOLDEN, charts, sizeof charts, gates, sizeof gates));
    mode = P4_GAME_MODE_GOLDEN;
    assert(cmmCommunityHasFlagGate(56));
    flag_value = 0; assert(!cmmCommunityPassesFlagGate(56));
    flag_value = 1; assert(cmmCommunityPassesFlagGate(56));
    assert(!cmmCommunityHasFlagGate(55));
    return 0;
}
