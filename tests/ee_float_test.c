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
#include "p4pc/hostmath.h"

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

/* The host-adder version of add/sub (p4pc_ee_add_hw_bits) against the guard-bit integer adder, in the EE float mode it needs. Operands: every exponent pair
 * with mantissa patterns that exercise the dropped bits, then random patterns of several kinds. */
static uint32_t sparse_operand(void)
{
    const uint32_t r = next32();
    const uint32_t m = (r & 1u) ? 0u : (next32() & next32() & 0x7FFFFFu);

    return (r & 0x80000000u) | ((next32() % 256u) << 23) | m;
}

static void check_add_hw(uint32_t a, uint32_t b)
{
    check("add(hw)", a, b, p4pc_ee_add_hw_bits(a, b), p4pc_ee_add_bits(a, b));
    check("sub(hw)", a, b, p4pc_ee_add_hw_bits(a, b ^ 0x80000000u), p4pc_ee_sub_bits(a, b));
}

static void test_add_hw(void)
{
    static const uint32_t mant[] = {0u, 1u, 2u, 0x7FFFFFu, 0x7FFFFEu, 0x400000u, 0x3FFFFFu, 0x155555u, 0x2AAAAAu, 0x000100u, 0x7FF000u};
    unsigned i;
    unsigned ea;
    unsigned eb;
    unsigned ma;
    unsigned mb;
    int kind;
    int kb;

    for (ea = 0; ea < 256; ea++) {
        for (eb = 0; eb < 256; eb++) {
            for (ma = 0; ma < sizeof mant / sizeof mant[0]; ma++) {
                for (mb = 0; mb < sizeof mant / sizeof mant[0]; mb++) {
                    const uint32_t a = (ea << 23) | mant[ma];
                    const uint32_t b = (eb << 23) | mant[mb];

                    check_add_hw(a, b);
                    check_add_hw(a | 0x80000000u, b);
                    check_add_hw(a, b | 0x80000000u);
                    check_add_hw(a | 0x80000000u, b | 0x80000000u);
                }
            }
        }
    }
    for (kind = 0; kind < 6; kind++) {
        for (kb = 0; kb < 6; kb++) {
            for (i = 0; i < 1500000u; i++) {
                const uint32_t a = kind == 5 ? sparse_operand() : operand(kind);
                const uint32_t b = kb == 5 ? sparse_operand() : operand(kb);

                check_add_hw(a, b);
                check_add_hw(a, a ^ (next32() & 0x807FFFFFu & (0u - (next32() & 1u))));   /* nearly equal magnitudes: cancellation */
            }
        }
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
    /* the host adder needs the EE mode (the divisions above do not depend on it, but run first anyway) */
    if (p4pc_fpu_ee_mode()) {
        test_add_hw();
    }
    if (fails != 0) {
        fprintf(stderr, "ee_float_test: FAIL, %lu of %lu differ\n", fails, checked);
        return 1;
    }
    (void)f_of;
    printf("ee_float_test: ok (%lu results)\n", checked);
    return 0;
}
