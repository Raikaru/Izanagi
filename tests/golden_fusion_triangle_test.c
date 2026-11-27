#include "p4pc/golden_fusion_runtime.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
typedef uint8_t u8; typedef int8_t s8; typedef int16_t s16; typedef uint16_t u16; typedef int32_t s32; typedef uintptr_t u32;
typedef struct { uint16_t flags, id; uint8_t level; } DatPersonaRecord;
static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static u8 metadata[256 * 14], *iGpffffb3d4 = metadata;
static s8 iGpffffa8d0[6] = {0, 1, 1, 2, 0, 1};
static u8 D_00642727[24 * 24], D_00642967[24 * 24];
static u8 *DAT_007644c4 = metadata, D_006420A0[23 * 8];
static int p4_test_flag;
static s32 datGetFlag(s32 flag) { assert(flag == 42); return p4_test_flag; }
int p4_game_personas_native_fusion_candidate(uint32_t id) { return id < 192; }
uint32_t p4_game_personas_native_fusion_next(uint32_t id) { return id + 1; }
static char D_00642F30[1];
static void kAssertFail(void *p, s32 line) { (void)p; (void)line; abort(); }
#include "../overrides/static/fclTriangleUpperArcana.c"
#include "../overrides/00311ea0.c"
#include "../overrides/003130e0.c"
int main(void)
{
    uint8_t charts[3][28 * 28] = {{0}}, gates[26 * 8] = {0}; u16 excluded[12] = {1, 2, 3};
    DatPersonaRecord a = {0, 1, 30}, b = {0, 2, 20}, c = {0, 3, 10};
    metadata[1 * 14 + 2] = 27; metadata[2 * 14 + 2] = 25; metadata[3 * 14 + 2] = 24;
    metadata[3 * 14] = 1;
    charts[1][24 * 28 + 25] = 26; charts[2][26 * 28 + 27] = 24;
    gates[25 * 8] = 42; gates[25 * 8 + 4] = 202; gates[25 * 8 + 6] = 1;
    metadata[202 * 14 + 2] = 24; metadata[202 * 14 + 3] = 50;
    assert(!p4_golden_fusion_install(P4_GAME_MODE_GOLDEN, &charts[0][0], sizeof charts, gates, sizeof gates));
    mode = P4_GAME_MODE_GOLDEN;
    assert(fclFuseTripleArcana((u8 *)&a, (u8 *)&b, (u8 *)&c) == 24);
    assert(fclFuseLowestCandidateAtLeast(fclFuseTripleArcana((u8 *)&a, (u8 *)&b, (u8 *)&c), 40, excluded) == 0);
    p4_test_flag = 1;
    assert(fclFuseLowestCandidateAtLeast(fclFuseTripleArcana((u8 *)&a, (u8 *)&b, (u8 *)&c), 40, excluded) == 202);
    metadata[1 * 14 + 2] = 1; metadata[2 * 14 + 2] = 3; metadata[3 * 14 + 2] = 2;
    D_00642727[2 * 24 + 3] = 4; D_00642967[1 * 24 + 4] = 5;
    mode = P4_GAME_MODE_PS2;
    assert(fclFuseTripleArcana((u8 *)&a, (u8 *)&b, (u8 *)&c) == 5);
    return 0;
}
