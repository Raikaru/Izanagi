/* Pin the impulse response of a synthetic preset with non-aliasing addresses. */
#include <stdint.h>
#include <stdio.h>

#include "p4snd_reverb.h"

int main(void)
{
    static const int16_t regs[32] = {
        3, 5, 0x6000, 0x3000, 0x2000, 0x1000, 0x0800, 0x4000,
        0x5000, 0x4000, 8, 40, 7, 39, 12, 44,
        4, 36, 16, 48, 20, 52, 24, 56,
        28, 60, 64, 96, 80, 112, 0x7fff, 0x6000,
    };
    float input[4096 * 2] = {0};
    float output[4096 * 2] = {0};
    uint32_t hash = 2166136261u;
    int i;

    input[0] = 0.75f;
    input[1] = 0.375f;
    p4rev_setup(regs, 0x1000);
    p4rev_volume(0x7fff, 0x6000);
    p4rev_enable(1);
    p4rev_process(input, output, 4096);
    p4rev_enable(0);
    /* FNV-1a over signed 16-bit samples, low byte first. */
    for (i = 0; i < 4096 * 2; ++i) {
        uint16_t sample = (uint16_t)(int16_t)(output[i] * 32768.0f);
        hash = (hash ^ (sample & 0xffu)) * 16777619u;
        hash = (hash ^ (sample >> 8)) * 16777619u;
    }
    if (hash != 0xb593dee7u) {
        fprintf(stderr, "reverb impulse hash %08x, expected b593dee7\n", hash);
        return 1;
    }
    return 0;
}
