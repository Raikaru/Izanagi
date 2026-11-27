#include "p4pc/golden_fusion_runtime.h"
#include <assert.h>
#include <string.h>
int main(void)
{
    uint8_t charts[3][28 * 28], gates[26 * 8] = {0};
    for (unsigned c = 0; c < 3; ++c)
        for (unsigned a = 0; a < 28; ++a)
            for (unsigned b = 0; b < 28; ++b)
                charts[c][a * 28 + b] = (uint8_t)((c + a * 3 + b) % 28);
    gates[25 * 8] = 0x78; gates[25 * 8 + 1] = 0x56;
    gates[25 * 8 + 4] = 0xfe; gates[25 * 8 + 5] = 0xff;
    gates[25 * 8 + 6] = 2;
    assert(p4_golden_fusion_install(P4_GAME_MODE_PS2, &charts[0][0], sizeof charts, gates, sizeof gates) == -1);
    assert(!p4_golden_fusion_install(P4_GAME_MODE_GOLDEN, &charts[0][0], sizeof charts, gates, sizeof gates));
    for (unsigned c = 0; c < 3; ++c)
        for (unsigned a = 0; a < 28; ++a)
            for (unsigned b = a; b < 28; ++b) {
                assert(p4_golden_fusion_chart(c, a, b) == charts[c][a * 28 + b]);
                assert(p4_golden_fusion_chart(c, b, a) == charts[c][a * 28 + b]);
            }
    assert(p4_golden_fusion_gate_count() == 26);
    assert(p4_golden_fusion_gate_flag(25) == 0x5678);
    assert(p4_golden_fusion_gate_persona(25) == -2);
    assert(p4_golden_fusion_gate_bits(25) == 2);
    charts[0][27] = 28;
    assert(p4_golden_fusion_install(P4_GAME_MODE_GOLDEN, &charts[0][0], sizeof charts, gates, sizeof gates) == -1);
    assert(p4_golden_fusion_chart(0, 0, 27) == 27);
    p4_golden_fusion_reset();
    return 0;
}
