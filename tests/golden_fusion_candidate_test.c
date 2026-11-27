#include "p4pc/golden_fusion_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
typedef uint8_t u8; typedef int16_t s16; typedef uint16_t u16; typedef int32_t s32; typedef uint32_t u32;
static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static u8 metadata[256 * 14], *iGpffffb3d4 = metadata, *DAT_007644c4 = metadata;
static u8 D_006420A0[23 * 8];
static char D_00642F30[1];
static int unlocked;
static s32 datGetFlag(s32 flag) { assert(flag == 42); return unlocked; }
static void kAssertFail(void *p, s32 line) { (void)p; (void)line; abort(); }
int p4_game_personas_native_fusion_candidate(uint32_t id) { return id < 192; }
uint32_t p4_game_personas_native_fusion_next(uint32_t id) { return id + 1; }
#include "../overrides/003130e0.c"
#include "../overrides/003133b0.c"
int main(void)
{
    u8 charts[3 * 28 * 28] = {0}, gates[26 * 8] = {0}; u16 excluded[12] = {0};
    gates[25 * 8] = 42; gates[25 * 8 + 4] = 200; gates[25 * 8 + 6] = 1;
    assert(!p4_golden_fusion_install(P4_GAME_MODE_GOLDEN, charts, sizeof charts, gates, sizeof gates));
    metadata[200 * 14 + 2] = 27; metadata[200 * 14 + 3] = 10;
    metadata[201 * 14 + 2] = 27; metadata[201 * 14 + 3] = 20;
    mode = P4_GAME_MODE_GOLDEN; unlocked = 0;
    assert(fclFuseLowestCandidateAtLeast(27, 8, excluded) == 201);
    assert(fclFuseLowestCandidateAtMost(27, 15, excluded) == 0);
    unlocked = 1;
    assert(fclFuseLowestCandidateAtLeast(27, 8, excluded) == 200);
    assert(fclFuseLowestCandidateAtMost(27, 15, excluded) == 200);
    excluded[0] = 200;
    assert(fclFuseLowestCandidateAtLeast(27, 8, excluded) == 201);
    metadata[10 * 14 + 2] = 3; metadata[10 * 14 + 3] = 12;
    mode = P4_GAME_MODE_PS2; excluded[0] = 0;
    assert(fclFuseLowestCandidateAtLeast(3, 8, excluded) == 10);
    assert(fclFuseLowestCandidateAtMost(3, 15, excluded) == 10);
    return 0;
}
