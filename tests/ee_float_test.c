/* The EE's nearest-rounded division, square root and reciprocal square root (include/p4pc/eefloat.h): the integer implementations against the long double formulas
 * they stand for (the definition the x87 path computes: the operation in extended precision, one rounding to float, clamped to +-FLT_MAX, a denormal result
 * keeping only its sign), and, where the host has the x87 unit, against the x87 path itself. The operands are random bit patterns (every exponent, denormals,
 * 255), random operands in the range games use, and the edge values. Every host runs it: AArch64's long double is a software quad, so there it checks the
 * integers against a reference more precise than the unit they replace. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "p4pc/eefloat.h"

static uint64_t rng = 0x9E3779B97F4A7C15ull;

static uint32_t next32(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return (uint32_t)(rng >> 16);
}

static float f_of(uint32_t u)
{
    float f;

    memcpy(&f, &u, sizeof f);
    return f;
}

static uint32_t ref_out(long double v)
{
    uint32_t sign;
    uint32_t u;
    long double m;
    float f;

    if (v != v) {
        return 0x7F7FFFFFu;
    }
    sign = signbit(v) ? 0x80000000u : 0u;
    m = fabsl(v);
    if (m >= 3.4028234663852886e38L) {
        return sign | 0x7F7FFFFFu;
    }
    f = (float)m;
    memcpy(&u, &f, sizeof u);
    return ((u >> 23) & 255u) == 0u ? sign : sign | u;
}

static uint32_t ref_div(uint32_t a, uint32_t b)
{
    const float x = p4pc_ee_float_in(a);
    const float y = p4pc_ee_float_in(b);

    if (y == 0.0f) {
        return ((a ^ b) & 0x80000000u) | 0x7F7FFFFFu;
    }
    return ref_out((long double)x / (long double)y);
}

static uint32_t ref_sqrt(uint32_t b)
{
    return ref_out(sqrtl(fabsl((long double)p4pc_ee_float_in(b))));
}

static uint32_t ref_rsqrt(uint32_t a, uint32_t b)
{
    const float y = p4pc_ee_float_in(b);

    if (y == 0.0f) {
        return (a & 0x80000000u) | 0x7F7FFFFFu;
    }
    return ref_out((long double)p4pc_ee_float_in(a) / sqrtl(fabsl((long double)y)));
}

static unsigned long fails;
static unsigned long checked;

static void check(const char *op, uint32_t a, uint32_t b, uint32_t got, uint32_t want)
{
    checked++;
    if (got != want) {
        if (fails < 12) {
            fprintf(stderr, "ee_float_test: %s(%08x, %08x) = %08x, expected %08x\n", op, (unsigned)a, (unsigned)b, (unsigned)got, (unsigned)want);
        }
        fails++;
    }
}

static uint32_t operand(int kind)
{
    static const uint32_t edge[] = {0x00000000u, 0x80000000u, 0x00000001u, 0x007FFFFFu, 0x00800000u, 0x80800000u, 0x3F800000u, 0xBF800000u, 0x40000000u,
                                    0x7F7FFFFFu, 0xFF7FFFFFu, 0x7F800000u, 0x7FFFFFFFu, 0xFFFFFFFFu, 0x7E800000u, 0x01000000u, 0x7F000000u, 0x3F7FFFFFu};
    const uint32_t r = next32();

    switch (kind) {
    case 0:
        return r;                                                     /* any pattern */
    case 1:
        return (r & 0x807FFFFFu) | ((100u + next32() % 56u) << 23);    /* the range games use */
    case 2:
        return (r & 0x807FFFFFu) | ((next32() % 5u) << 23);           /* near the bottom of the range */
    case 3:
        return (r & 0x807FFFFFu) | ((250u + next32() % 6u) << 23);    /* near the top */
    default:
        return edge[r % (sizeof edge / sizeof edge[0])];
    }
}

int main(void)
{
    const unsigned n = 400000;
    int kind;
    unsigned i;
    int kb;

    for (kind = 0; kind < 5; kind++) {
        for (kb = 0; kb < 5; kb++) {
            for (i = 0; i < n / 5u; i++) {
                const uint32_t a = operand(kind);
                const uint32_t b = operand(kb);

                check("div", a, b, p4pc_ee_div_bits_int(a, b), ref_div(a, b));
                check("sqrt", a, 0, p4pc_ee_sqrt_bits_int(a), ref_sqrt(a));
#ifdef __SIZEOF_INT128__
                check("rsqrt", a, b, p4pc_ee_rsqrt_bits_int(a, b), ref_rsqrt(a, b));
#endif
#if defined(__i386__) || defined(__x86_64__)
                check("div(x87)", a, b, p4pc_ee_div_bits(a, b), ref_div(a, b));
                check("sqrt(x87)", a, 0, p4pc_ee_sqrt_bits(a), ref_sqrt(a));
                check("rsqrt(x87)", a, b, p4pc_ee_rsqrt_bits(a, b), ref_rsqrt(a, b));
#endif
            }
        }
    }
    if (fails != 0) {
        fprintf(stderr, "ee_float_test: FAIL, %lu of %lu differ\n", fails, checked);
        return 1;
    }
    (void)f_of;
    printf("ee_float_test: ok (%lu results)\n", checked);
    return 0;
}
