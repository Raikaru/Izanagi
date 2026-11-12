/* Equivalence checks for the VU0 emulation helpers in include/p4pc/vu0.h.
 * Each expected value is the scalar result the VU0 semantics document. */
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "p4pc/hostmath.h"
#include "p4pc/vu0.h"

static int failures;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);        \
            failures++;                                                   \
        }                                                                 \
    } while (0)

static void setv(int vf, float a, float b, float c, float d)
{
    p4pc_vu0.vf[vf].f[0] = a;
    p4pc_vu0.vf[vf].f[1] = b;
    p4pc_vu0.vf[vf].f[2] = c;
    p4pc_vu0.vf[vf].f[3] = d;
}

static int same4(int vf, float a, float b, float c, float d)
{
    return p4pc_vu0.vf[vf].f[0] == a && p4pc_vu0.vf[vf].f[1] == b &&
           p4pc_vu0.vf[vf].f[2] == c && p4pc_vu0.vf[vf].f[3] == d;
}

static void test_mul_add_sub(void)
{
    setv(2, 1.0f, 2.0f, 3.0f, 4.0f);
    setv(3, 10.0f, 20.0f, 30.0f, 40.0f);

    /* Broadcast x of vf3 under the XYZ mask; w keeps its old value. */
    setv(4, 9.0f, 9.0f, 9.0f, 9.0f);
    p4_vu0_mul(4, P4_VU_XYZ, 2, 3, P4_VU_BC_X);
    CHECK(same4(4, 10.0f, 20.0f, 30.0f, 9.0f));

    /* No broadcast: lane-wise add. */
    p4_vu0_add(5, P4_VU_XYZW, 2, 3, P4_VU_NOBC);
    CHECK(same4(5, 11.0f, 22.0f, 33.0f, 44.0f));

    p4_vu0_sub(6, P4_VU_XYZW, 2, 3, P4_VU_NOBC);
    CHECK(same4(6, -9.0f, -18.0f, -27.0f, -36.0f));
}

static void test_fixup_and_vf0(void)
{
    /* NaN -> 0, +inf -> FLT_MAX, denormal -> 0, -inf -> -FLT_MAX. */
    setv(7, __builtin_nanf(""), __builtin_inff(), 1e-40f, -__builtin_inff());
    p4_vu0_add(8, P4_VU_XYZW, 7, 0, P4_VU_BC_X); /* + vf0.x (0) */
    CHECK(same4(8, 0.0f, FLT_MAX, 0.0f, -FLT_MAX));

    /* vf0 is hardwired to (0,0,0,1): writes are ignored. */
    setv(2, 5.0f, 6.0f, 7.0f, 8.0f);
    p4_vu0_move(0, P4_VU_XYZW, 2);
    CHECK(same4(0, 0.0f, 0.0f, 0.0f, 1.0f));
}

static void test_q_register(void)
{
    /* vsqrt: Q = sqrt(ft[bc]); Q is mirrored into vi[22]. */
    setv(9, 4.0f, 0.0f, 0.0f, 0.0f);
    p4_vu0_sqrt(9, P4_VU_BC_X);
    CHECK(p4pc_vu0.q == 2.0f);
    CHECK((uint32_t)p4pc_vu0.vi[22] == 0x40000000u); /* bits of 2.0f */

    /* vrsqrt: Q = fs[fbc] / sqrt(ft[tbc]). */
    setv(10, 1.0f, 0.0f, 0.0f, 0.0f);
    setv(11, 4.0f, 0.0f, 0.0f, 0.0f);
    p4_vu0_rsqrt(10, P4_VU_BC_X, 11, P4_VU_BC_X);
    CHECK(p4pc_vu0.q == 0.5f);
    CHECK((uint32_t)p4pc_vu0.vi[22] == 0x3F000000u); /* bits of 0.5f */
}

static void test_opmula(void)
{
    /* Cross product: ACC.xyz = vs x vt, here (1,0,0) x (0,1,0) = (0,0,1). */
    setv(12, 1.0f, 0.0f, 0.0f, 0.0f);
    setv(13, 0.0f, 1.0f, 0.0f, 0.0f);
    p4_vu0_opmula(12, 13);
    CHECK(p4pc_vu0.acc.f[0] == 0.0f);
    CHECK(p4pc_vu0.acc.f[1] == 0.0f);
    CHECK(p4pc_vu0.acc.f[2] == 1.0f);
}

static void test_colour_pack(void)
{
    P4Mmi128 lanes;

    /* Bytes of 0x04030201 land in word lanes x..w as 1,2,3,4. */
    lanes = p4_vu0_word_bytes_to_lanes(0x04030201u);
    CHECK(lanes.w[0] == 1u && lanes.w[1] == 2u && lanes.w[2] == 3u && lanes.w[3] == 4u);

    /* vf10 * 255, truncated, lane x in the lowest byte:
     * 0.5 -> 127, 1.0 -> 255, 0.25 -> 63, 0.0 -> 0. */
    setv(10, 0.5f, 1.0f, 0.25f, 0.0f);
    CHECK(p4_vu0_pack_vf10_255() == 0x003FFF7Fu);
}

static uint32_t madd_random(uint32_t *state)
{
    uint32_t x = *state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

static void test_madd_bits(void)
{
    static const uint32_t edges[] = {
        0, 0x80000000u, 1, 0x007fffffu, 0x00800000u, 0x80800000u,
        0x7f7fffffu, 0xff7fffffu, 0x7f800000u, 0xff800000u,
        0x7fc12345u, 0xffa12345u, 0x3f800000u, 0xbf800000u,
        0x3f800001u, 0x3f7fffffu, 0x33800000u, 0xb3800000u
    };
    static const int destinations[] = {0, 1, 2, 3, P4_VU_ACC};
    uint32_t random = 0x92d68ca2u;

    /* Compare the previous scalar algorithm bit for bit, including masked NaN payloads,
     * source/destination/ACC aliases, every lane mask and every broadcast selection. */
    for (unsigned test = 0; test < 262144u; test++) {
        const unsigned mask = test & 15u;
        const int bc = (int)((test >> 4) % 5u) - 1;
        const int dst = destinations[(test >> 7) % 5u];
        P4Vu0Reg expected;
        P4Vu0Reg *out;

        for (unsigned reg = 0; reg < 4; reg++) {
            P4Vu0Reg *value = reg == 3 ? &p4pc_vu0.acc : &p4pc_vu0.vf[reg + 1];
            for (unsigned lane = 0; lane < 4; lane++) {
                uint32_t bits = madd_random(&random);
                value->u[lane] = test < 4096u ? edges[bits % (sizeof edges / sizeof edges[0])] : bits;
            }
        }
        out = dst == P4_VU_ACC ? &p4pc_vu0.acc : &p4pc_vu0.vf[dst];
        expected = *out;
        if (dst != 0) {
            for (unsigned lane = 0; lane < 4; lane++) {
                if (mask & (8u >> lane)) {
                    expected.f[lane] = p4_vu0_fixup((float)(p4_vu0_in(p4pc_vu0.acc.f[lane]) +
                        (double)(float)(p4_vu0_in(p4pc_vu0.vf[1].f[lane]) * p4_vu0_in(p4_vu0_t(2, (int)lane, bc)))));
                }
            }
        }
        p4_vu0_madd(dst, mask, 1, 2, bc);
        if (memcmp(out, &expected, sizeof expected) != 0) {
            printf("madd case %u dst %d mask %x bc %d\n", test, dst, mask, bc);
            for (unsigned lane = 0; lane < 4; lane++) {
                printf("lane %u: got %08x expected %08x\n", lane, out->u[lane], expected.u[lane]);
            }
            CHECK(0);
            return;
        }
    }
}

int main(void)
{
    p4pc_fpu_ee_mode(); /* the helpers truncate through the MXCSR rounding mode */
    test_mul_add_sub();
    test_fixup_and_vf0();
    test_q_register();
    test_opmula();
    test_colour_pack();
    test_madd_bits();

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("vu0 helper checks passed\n");
    return 0;
}
