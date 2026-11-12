/* The quaternion overrides whose sums retail accumulates (mula.s / madda.s / madd.s chains) against the unchanged retail instructions. The R5900
 * interpreter (tools/mipsdiff/r5900.c) runs the retail words of each function and the production override runs on a copy of the same guest bytes;
 * every byte of the keyframes, outputs and slerp record must be equal. The EE adder is not associative, so a sum written in another order than
 * retail's chain differs here in a share of the cases: the slerp setup (003dcc70), RtAnimKeyFrameInterpolate (00415bc0), RtAnimKeyFrameBlend
 * (004165a0), RtAnimKeyFrameMulRecip (00416e10) and RtAnimKeyFrameAdd (00417310). The cases are deterministic: unit quaternion pairs from near
 * parallel (the acos near 1 that magnifies an ulp of the dot) to far apart, random blend factors and translations. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/hostmath.h"
#include "p4pc/image.h"
#include "p4pc/rtanim_half.h"
#include "r5900.h"

void func_003dcc70(float *q1, float *q2, float *out);
void func_00415bc0(unsigned char *out, unsigned char *in1, unsigned char *in2, float time, float *custom);
void func_004165a0(unsigned char *out, unsigned char *in1, unsigned char *in2, float alpha);
void func_00416e10(unsigned char *frame, unsigned char *start);
void func_00417310(unsigned char *out, unsigned char *in1, unsigned char *in2);

#define CASES 4096
#define AREA_ADDR 0x01c00000u   /* guest RAM the interpreter accepts (below 32 MiB), above the loaded image */
#define STACK_TOP 0x01ff0000u
#define AREA 0x200u
#define IN1 0x00u     /* keyframe or quaternion */
#define IN2 0x40u
#define OUT 0x80u     /* keyframe or slerp record */
#define CUSTOM 0xc0u  /* 00415bc0: offset x y z, scale x y z */

enum { SETUP, INTERP, BLEND, MULRECIP, ADD, KINDS };
static const char *const names[KINDS] = {"003dcc70 slerp setup", "00415bc0 RtAnimKeyFrameInterpolate", "004165a0 RtAnimKeyFrameBlend",
                                         "00416e10 RtAnimKeyFrameMulRecip", "00417310 RtAnimKeyFrameAdd"};
static const uint32_t entries[KINDS] = {0x003dcc70u, 0x00415bc0u, 0x004165a0u, 0x00416e10u, 0x00417310u};

static unsigned char *area;
static uint64_t rng = UINT64_C(0x9e3779b97f4a7c15);

static uint32_t next(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 32);
}

/* A uniform float in [-1, 1) with 24 exact bits. */
static float unit(void)
{
    return (float)((int32_t)(next() >> 8) - 0x800000) / 8388608.0f;
}

static uint32_t bits(float f)
{
    uint32_t u;

    memcpy(&u, &f, sizeof u);
    return u;
}

static void normalize(float *q)
{
    const float n = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
    int i;

    for (i = 0; i < 4; i++) {
        q[i] = q[i] / n;
    }
}

/* q2 = q1 turned by a step whose size runs from 2^-14 to 1, and negated in a quarter of the cases (the negative-dot branch). */
static void pair(float *q1, float *q2)
{
    const float step = ldexpf(1.0f, -(int)(next() % 15u));
    int i;

    do {
        for (i = 0; i < 4; i++) {
            q1[i] = unit();
        }
    } while (fabsf(q1[0]) + fabsf(q1[1]) + fabsf(q1[2]) + fabsf(q1[3]) < 0.25f);
    normalize(q1);
    for (i = 0; i < 4; i++) {
        q2[i] = q1[i] + step * unit();
    }
    normalize(q2);
    if ((next() & 3u) == 0u) {
        for (i = 0; i < 4; i++) {
            q2[i] = -q2[i];
        }
    }
}

static void put_float(unsigned off, float f)
{
    memcpy(area + off, &f, sizeof f);
}

static void put_half(unsigned off, float f)
{
    const unsigned short h = p4_anim_float_to_half(f);

    memcpy(area + off, &h, sizeof h);
}

/* Fills the area for one case of `kind` and returns the float argument (f12). */
static float fill(int kind)
{
    float q1[4], q2[4];
    float arg = 0.0f;
    int i;

    memset(area, 0, AREA);
    pair(q1, q2);
    switch (kind) {
    case SETUP:
        for (i = 0; i < 4; i++) {
            put_float(IN1 + i * 4, q1[i]);
            put_float(IN2 + i * 4, q2[i]);
        }
        break;
    case INTERP:
    case MULRECIP:
        put_float(IN1 + 4, 0.0f);
        put_float(IN2 + 4, 1.0f);
        for (i = 0; i < 4; i++) {
            put_half(IN1 + 8 + i * 2, q1[i]);
            put_half(IN2 + 8 + i * 2, q2[i]);
        }
        for (i = 0; i < 3; i++) {
            put_half(IN1 + 0x10 + i * 2, unit());
            put_half(IN2 + 0x10 + i * 2, unit());
            put_float(CUSTOM + i * 4, 64.0f * unit());
            put_float(CUSTOM + 12 + i * 4, 1.0f + unit());
        }
        arg = (float)(next() >> 8) / 16777216.0f;
        break;
    case BLEND:
    case ADD:
        put_float(IN1 + 4, 0.0f);
        put_float(IN2 + 4, 1.0f);
        for (i = 0; i < 4; i++) {
            put_float(IN1 + 8 + i * 4, q1[i]);
            put_float(IN2 + 8 + i * 4, q2[i]);
        }
        for (i = 0; i < 3; i++) {
            put_float(IN1 + 0x18 + i * 4, 100.0f * unit());
            put_float(IN2 + 0x18 + i * 4, 100.0f * unit());
        }
        arg = (float)(next() >> 8) / 16777216.0f;
        break;
    }
    return arg;
}

static int retail(int kind, float arg)
{
    const uint32_t base = AREA_ADDR;
    R5900 c;
    int rc;

    memset(&c, 0, sizeof c);
    c.lo_pc = 0x00100000u;
    c.hi_pc = 0x005e4000u;
    c.max_steps = 1000000;
    c.r[28] = 0x007690f0u;
    c.r[29] = STACK_TOP;
    switch (kind) {
    case SETUP:
        c.r[4] = base + IN1;
        c.r[5] = base + IN2;
        c.r[6] = base + OUT;
        break;
    case MULRECIP:
        c.r[4] = base + IN1;
        c.r[5] = base + IN2;
        break;
    default:
        c.r[4] = base + OUT;
        c.r[5] = base + IN1;
        c.r[6] = base + IN2;
        c.r[7] = base + CUSTOM;
        c.f[12] = bits(arg);
        break;
    }
    rc = r5900_call(&c, entries[kind]);
    if (rc != R5900_OK) {
        fprintf(stderr, "%s: interpreter rc=%d pc=%08x word=%08x reason=%s\n", names[kind], rc, c.bad_pc, c.bad_word,
                c.fault_why != NULL ? c.fault_why : "none");
    }
    return rc;
}

static void native(int kind, float arg)
{
    switch (kind) {
    case SETUP:
        func_003dcc70((float *)(area + IN1), (float *)(area + IN2), (float *)(area + OUT));
        break;
    case INTERP:
        func_00415bc0(area + OUT, area + IN1, area + IN2, arg, (float *)(area + CUSTOM));
        break;
    case BLEND:
        func_004165a0(area + OUT, area + IN1, area + IN2, arg);
        break;
    case MULRECIP:
        func_00416e10(area + IN1, area + IN2);
        break;
    case ADD:
        func_00417310(area + OUT, area + IN1, area + IN2);
        break;
    }
}

static int test_main(int argc, char **argv)
{
    const char *elf = getenv("P4PC_ELF");
    static unsigned char input[AREA], expected[AREA];
    int kind, n, failures = 0;

    (void)argc;
    (void)argv;
    if (elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "anim_accumulator_order_test: P4PC_ELF unset, skipped\n");
        return 77;
    }
    p4pc_load_image(elf);
    p4pc_apply_patches();
    if (!p4pc_fpu_ee_mode()) {
        fprintf(stderr, "anim_accumulator_order_test: P4PC_EE_ROUNDING=near, skipped\n");
        return 77;
    }
    area = P4_G2H(AREA_ADDR);
    for (kind = 0; kind < KINDS; kind++) {
        int differ = 0;

        for (n = 0; n < CASES; n++) {
            const float arg = fill(kind);

            memcpy(input, area, AREA);
            if (retail(kind, arg) != R5900_OK) {
                return 1;
            }
            memcpy(expected, area, AREA);
            memcpy(area, input, AREA);
            native(kind, arg);
            if (memcmp(area, expected, AREA) != 0) {
                unsigned off = 0;

                while (area[off] == expected[off]) {
                    off++;
                }
                off &= ~3u;
                if (differ < 4) {
                    uint32_t got, want;

                    memcpy(&got, area + off, 4);
                    memcpy(&want, expected + off, 4);
                    printf("FAIL %s case %d: word +0x%02x native %08x, retail %08x\n", names[kind], n, off, (unsigned)got,
                           (unsigned)want);
                }
                differ++;
            }
        }
        printf("%s: %d of %d cases differ from the retail instructions\n", names[kind], differ, CASES);
        failures += differ;
    }
    return failures != 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
