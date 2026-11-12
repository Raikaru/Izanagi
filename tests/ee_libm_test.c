/* Cross-target check of the retail libm cores (src/platform/ee_libm.c, compiled through tools/ee_float_cc.py like in the game) in the EE float mode:
 * sinf, cosf, tanf, atanf, atan2f, floorf and sqrtf over a fixed input sweep, folded into one FNV-1a hash per function. The hashes are the same on
 * Linux i386 and 32-bit Windows (and the same as the retail interpreter's, tools/mipsdiff): a build or a float state that differs on one target (the
 * x87 control word a driver changed, a fused multiply-add, a host libm call in the middle) changes a hash. The expected values are recorded from the Linux
 * build; P4PC_LIBM_PRINT=1 prints the hashes to record them again. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/hostmath.h"
#include "ee_libm_expect.h"

float func_0044b7b0(float x);           /* sinf */
float func_0044b610(float x);           /* cosf */
float func_0044b868(float x);           /* tanf */
float func_0044b340(float x);           /* atanf (retail 0x0044b340, named by its place in newlib's libm; the address is what is fixed) */
float func_004494b8(float y, float x);  /* atan2f (retail 0x004494b8, likewise) */
float func_0044b6d8(float x);           /* floorf */
float func_0044a4d0(float x);           /* __ieee754_sqrtf */

static uint32_t bits(float f)
{
    uint32_t u;

    memcpy(&u, &f, sizeof u);
    return u;
}

static float from_bits(uint32_t u)
{
    float f;

    memcpy(&f, &u, sizeof f);
    return f;
}

static uint32_t fold(uint32_t h, uint32_t v)
{
    int i;

    for (i = 0; i < 4; i++) {
        h = (h ^ ((v >> (8 * i)) & 255u)) * 16777619u;
    }
    return h;
}

/* Inputs: a linear sweep over [-20, 20], the angles the game feeds (multiples of pi/20), and a fixed pseudo-random spread of float patterns. */
#define SWEEP 4001
#define SPREAD 4000

static float input(int k)
{
    static uint32_t s = 12345u;

    if (k < SWEEP) {
        return (float)(k - SWEEP / 2) * 0.01f;
    }
    if (k < 2 * SWEEP) {
        return (float)(k - SWEEP) * 0.15707964f;
    }
    s = s * 1664525u + 1013904223u;
    {
        const float f = from_bits(0x30000000u + (s >> 3) % 0x1C000000u);

        return (s & 1u) ? -f : f;
    }
}

struct row {
    const char *name;
    uint32_t expect;
};

int main(void)
{
    static struct row rows[] = {
        {"sinf", 0}, {"cosf", 0}, {"tanf", 0}, {"atanf", 0}, {"atan2f", 0}, {"floorf", 0}, {"sqrtf", 0},
    };
    uint32_t h[7];
    int i, k, bad = 0;
    const int print = getenv("P4PC_LIBM_PRINT") != NULL;
    const float halfpi = 1.57079637f;

    if (!p4pc_fpu_ee_mode()) {
        fprintf(stderr, "ee_libm_test: no EE float mode on this target\n");
        return 77;
    }
    for (i = 0; i < 7; i++) {
        h[i] = 2166136261u;
    }
    for (k = 0; k < 2 * SWEEP + SPREAD; k++) {
        const float x = input(k);
        const float y = input((k * 7 + 3) % (2 * SWEEP + SPREAD));

        h[0] = fold(h[0], bits(func_0044b7b0(x)));
        h[1] = fold(h[1], bits(func_0044b610(x)));
        h[2] = fold(h[2], bits(func_0044b868(x)));
        h[3] = fold(h[3], bits(func_0044b340(x)));
        h[4] = fold(h[4], bits(func_004494b8(y, x)));
        h[5] = fold(h[5], bits(func_0044b6d8(x)));
        h[6] = fold(h[6], bits(func_0044a4d0(x < 0 ? -x : x)));
    }
    /* the case that diverged first: the field panel's sinf(pi/2 * n / 10) at n = 10 */
    printf("ee_libm_test: sinf(pi/2)=%08x cosf(pi/2)=%08x\n", bits(func_0044b7b0((halfpi * 10.0f) / 10.0f)), bits(func_0044b610(halfpi)));
    for (i = 0; i < 7; i++) {
        printf("ee_libm_test: %s %08x\n", rows[i].name, h[i]);
    }
    if (print) {
        return 0;
    }
    {
        static const uint32_t expect[7] = {EXPECT_HASHES};

        for (i = 0; i < 7; i++) {
            if (h[i] != expect[i]) {
                printf("ee_libm_test: %s differs from the recorded %08x\n", rows[i].name, expect[i]);
                bad = 1;
            }
        }
    }
    return bad;
}
