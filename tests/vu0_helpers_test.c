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
    /* OPMULA is only the first half; OPMSUB with reversed operands finishes
     * the cross product.  Preserve W and support destination/source aliasing. */
    setv(12, 1.0f, 2.0f, 3.0f, 7.0f);
    setv(13, 4.0f, 5.0f, 6.0f, 8.0f);
    p4pc_vu0.acc.f[3] = 9.0f;
    p4_vu0_opmula(12, 13);
    CHECK(p4pc_vu0.acc.f[0] == 12.0f);
    CHECK(p4pc_vu0.acc.f[1] == 12.0f);
    CHECK(p4pc_vu0.acc.f[2] == 5.0f);
    CHECK(p4pc_vu0.acc.f[3] == 9.0f);
    p4_vu0_opmsub(12, 13, 12);
    CHECK(same4(12, -3.0f, 6.0f, -3.0f, 7.0f));

    /* The particle quaternion routine adds its two scalar terms between the
     * instructions; a full cross product in each instruction doubles the
     * cross term and produces a non-unit orientation. */
    setv(10, 0.5f, 0.5f, 0.5f, 0.5f);
    setv(11, 0.5f, -0.5f, 0.5f, 0.5f);
    p4_vu0_mul(2, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    p4_vu0_opmula(10, 11);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 11, 10, P4_VU_BC_W);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 10, 11, P4_VU_BC_W);
    p4_vu0_opmsub(10, 11, 10);
    p4_vu0_mul(P4_VU_ACC, P4_VU_W, 10, 11, P4_VU_NOBC);
    p4_vu0_msub(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_msub(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
    p4_vu0_msub(10, P4_VU_W, 0, 2, P4_VU_BC_Z);
    CHECK(same4(10, 1.0f, 0.0f, 0.0f, 0.0f));
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

#if defined(__SSE2__) || defined(__aarch64__)
/* The packed operations (src/vu0.c, p4_vu0_*_s) against the scalar helpers of vu0.h: the same register file and the same arguments give the same 33
 * registers bit for bit. The macros of vu0.h send p4_vu0_mul(...) to the packed version, so the scalar one is called as (p4_vu0_mul)(...). */
static uint32_t simd_pattern(uint32_t *state, int kind)
{
    static const uint32_t edges[] = {
        0, 0x80000000u, 1, 0x80000001u, 0x007fffffu, 0x807fffffu, 0x00800000u, 0x80800000u, 0x00800001u,
        0x7f7fffffu, 0xff7fffffu, 0x7f800000u, 0xff800000u, 0x7f800001u, 0x7fc12345u, 0xffa12345u, 0xffffffffu, 0x7fffffffu,
        0x3f800000u, 0xbf800000u, 0x3f800001u, 0x3f7fffffu, 0x33800000u, 0xb3800000u, 0x4b000000u, 0xcb000000u, 0x4f000000u, 0xcf000000u,
        0x4effffffu, 0xceffffffu, 0x7e800000u, 0xfe800000u, 0x01000000u, 0x81000000u
    };
    const uint32_t r = madd_random(state);
    const uint32_t r2 = madd_random(state);

    switch (kind) {
    case 0:
        return edges[r % (sizeof edges / sizeof edges[0])];
    case 1:
        return r;                                                  /* any pattern */
    case 2:
        return (r2 & 0x807fffffu) | ((100u + r % 56u) << 23);      /* ordinary magnitudes */
    case 3:
        return (r2 & 0x807fffffu) | ((r % 6u) << 23);              /* around the denormal edge */
    default:
        return (r2 & 0x807fffffu) | ((250u + r % 6u) << 23);       /* around the overflow edge */
    }
}

static void simd_fill(uint32_t *state, int kind)
{
    for (unsigned reg = 1; reg < 32; reg++) {
        for (unsigned lane = 0; lane < 4; lane++) {
            p4pc_vu0.vf[reg].u[lane] = simd_pattern(state, kind < 5 ? kind : (int)(madd_random(state) % 5u));
        }
    }
    for (unsigned lane = 0; lane < 4; lane++) {
        p4pc_vu0.acc.u[lane] = simd_pattern(state, kind < 5 ? kind : (int)(madd_random(state) % 5u));
    }
    p4pc_vu0.vf[0].f[0] = p4pc_vu0.vf[0].f[1] = p4pc_vu0.vf[0].f[2] = 0.0f;
    p4pc_vu0.vf[0].f[3] = 1.0f;
    p4pc_vu0.q = *(float *)(uint32_t[]){simd_pattern(state, 1)};
}

static void simd_check_op(int op, const char *name)
{
    static const int destinations[] = {0, 1, 2, 5, 17, 31, P4_VU_ACC};
    uint32_t random = 0x1234abcdu + (uint32_t)op * 7919u;
    P4Vu0State before;
    P4Vu0State scalar;

    for (unsigned test = 0; test < 400000u; test++) {
        const unsigned mask = test & 15u;
        const int bc = (int)((test >> 4) % 5u) - 1;
        const int dst = destinations[(test >> 7) % 7u];
        const int s = (int)(madd_random(&random) % 33u) & 31;   /* includes vf0 and aliases of dst */
        const int t = (int)(madd_random(&random) % 33u) & 31;
        const int kind = (int)((test >> 3) % 6u);

        simd_fill(&random, kind);
        if (test % 5u == 0u) {
            p4pc_vu0.vf[dst == P4_VU_ACC ? 3 : dst].u[test & 3u] = p4pc_vu0.vf[s].u[(test >> 2) & 3u];   /* aliases */
        }
#ifndef __OPTIMIZE__
        if (op == 6) {
            /* an unoptimised scalar p4_vu0_fixup compares a denormal under DAZ (equal to zero) and so keeps it; the optimised one (what the game runs)
             * classifies the bits and makes it +0, as the packed fix-up does: leave that case to the optimised build */
            for (unsigned lane = 0; lane < 4; lane++) {
                uint32_t *w = &p4pc_vu0.vf[s].u[lane];

                if ((*w & 0x7f800000u) == 0u && s != 0) {
                    *w = 0u;
                }
            }
        }
#endif
        before = p4pc_vu0;
        switch (op) {
        case 0: (p4_vu0_mul)(dst, mask, s, t, bc); break;
        case 1: (p4_vu0_add)(dst, mask, s, t, bc); break;
        case 2: (p4_vu0_sub)(dst, mask, s, t, bc); break;
        case 3: (p4_vu0_madd)(dst, mask, s, t, bc); break;
        case 4: (p4_vu0_msub)(dst, mask, s, t, bc); break;
        case 5: (p4_vu0_mulq)(dst, mask, s); break;
        case 6: (p4_vu0_move)(dst, mask, s); break;
        case 7: (p4_vu0_opmula)(s, t); break;
        case 8: (p4_vu0_opmsub)(dst, s, t); break;
        case 9: (p4_vu0_itof)(dst, mask, s); break;
        default: (p4_vu0_ftoi)(dst, mask, s); break;
        }
        scalar = p4pc_vu0;
        p4pc_vu0 = before;
        switch (op) {
        case 0: p4_vu0_mul_s(dst, mask, s, t, bc); break;
        case 1: p4_vu0_add_s(dst, mask, s, t, bc); break;
        case 2: p4_vu0_sub_s(dst, mask, s, t, bc); break;
        case 3: p4_vu0_madd_s(dst, mask, s, t, bc); break;
        case 4: p4_vu0_msub_s(dst, mask, s, t, bc); break;
        case 5: p4_vu0_mulq_s(dst, mask, s); break;
        case 6: p4_vu0_move_s(dst, mask, s); break;
        case 7: p4_vu0_opmula_s(s, t); break;
        case 8: p4_vu0_opmsub_s(dst, s, t); break;
        case 9: p4_vu0_itof_s(dst, mask, s); break;
        default: p4_vu0_ftoi_s(dst, mask, s); break;
        }
        if (memcmp(&scalar, &p4pc_vu0, sizeof scalar) != 0) {
            printf("simd %s case %u dst %d mask %x s %d t %d bc %d\n", name, test, dst, mask, s, t, bc);
            for (unsigned reg = 0; reg < 33; reg++) {
                const P4Vu0Reg *a = reg == 32 ? &scalar.acc : &scalar.vf[reg];
                const P4Vu0Reg *b = reg == 32 ? &p4pc_vu0.acc : &p4pc_vu0.vf[reg];

                if (memcmp(a, b, sizeof *a) != 0) {
                    printf("reg %u: scalar %08x %08x %08x %08x, packed %08x %08x %08x %08x (src s %08x %08x %08x %08x, t %08x %08x %08x %08x, acc %08x %08x %08x %08x)\n", reg,
                           a->u[0], a->u[1], a->u[2], a->u[3], b->u[0], b->u[1], b->u[2], b->u[3],
                           before.vf[s].u[0], before.vf[s].u[1], before.vf[s].u[2], before.vf[s].u[3],
                           before.vf[t].u[0], before.vf[t].u[1], before.vf[t].u[2], before.vf[t].u[3],
                           before.acc.u[0], before.acc.u[1], before.acc.u[2], before.acc.u[3]);
                }
            }
            CHECK(0);
            return;
        }
    }
}

static void test_simd_equivalence(void)
{
    static const char *const names[] = {"mul", "add", "sub", "madd", "msub", "mulq", "move", "opmula", "opmsub", "itof", "ftoi"};

    for (int op = 0; op < 11; op++) {
        simd_check_op(op, names[op]);
    }
}
#endif

int main(void)
{
    p4pc_fpu_ee_mode(); /* the helpers truncate through the MXCSR rounding mode */
    test_mul_add_sub();
    test_fixup_and_vf0();
    test_q_register();
    test_opmula();
    test_colour_pack();
    test_madd_bits();
#if defined(__SSE2__) || defined(__aarch64__)
    test_simd_equivalence();
#endif

    if (failures) {
        printf("%d check(s) failed\n", failures);
        return 1;
    }
    printf("vu0 helper checks passed\n");
    return 0;
}
