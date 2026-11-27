#include "p4pc/golden_fusion_runtime.h"
#include <assert.h>
#include <stdlib.h>
typedef unsigned char u8;
typedef int s32;
static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static u8 D_00642147[24 * 24], D_00642387[24 * 24], D_00642967[24 * 24];
static char D_00642F30[1];
static void kAssertFail(void *file, s32 line) { (void)file; (void)line; abort(); }
#include "../overrides/static/fclNormalPairArcana.c"
#include "../overrides/static/fclAlternatePairArcana.c"
#include "../overrides/static/fclTriangleUpperArcana.c"
int main(void)
{
    uint8_t charts[3][28 * 28], gates[26 * 8] = {0};
    for (unsigned a = 0; a < 24; ++a) for (unsigned b = 0; b < 24; ++b) {
        D_00642147[a * 24 + b] = (u8)((a + b) % 24);
        D_00642387[a * 24 + b] = (u8)((a + 2 * b) % 24);
        D_00642967[a * 24 + b] = (u8)((2 * a + b) % 24);
    }
    for (unsigned c = 0; c < 3; ++c) for (unsigned a = 0; a < 28; ++a)
        for (unsigned b = 0; b < 28; ++b) charts[c][a * 28 + b] = (u8)((c + a + b) % 28);
    mode = P4_GAME_MODE_PS2;
    for (unsigned a = 1; a < 24; ++a) for (unsigned b = 1; b < 24; ++b) {
        unsigned lo = a < b ? a : b, hi = a < b ? b : a;
        assert(fclNormalPairArcana(a, b) == D_00642147[lo * 24 + hi]);
        assert(fclAlternatePairArcana(a, b) == D_00642387[lo * 24 + hi]);
        assert(fclTriangleUpperArcana(a, b) == D_00642967[lo * 24 + hi]);
    }
    assert(!p4_golden_fusion_install(P4_GAME_MODE_GOLDEN, &charts[0][0], sizeof charts, gates, sizeof gates));
    mode = P4_GAME_MODE_GOLDEN;
    for (unsigned a = 1; a < 28; ++a) for (unsigned b = 1; b < 28; ++b) {
        unsigned lo = a < b ? a : b, hi = a < b ? b : a;
        assert(fclNormalPairArcana(a, b) == charts[0][lo * 28 + hi]);
        assert(fclAlternatePairArcana(a, b) == charts[0][lo * 28 + hi]);
        assert(fclTriangleUpperArcana(a, b) == charts[2][lo * 28 + hi]);
    }
    return 0;
}
