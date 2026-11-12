/* The EE FPU's addition and subtraction, bit exact (integer only, independent of MXCSR).
 *
 * add.s / sub.s do not truncate the exact sum: the adder aligns the smaller operand by shifting its mantissa right, keeping one guard bit below the
 * 24 mantissa bits and dropping the rest, adds or subtracts the aligned mantissas, and truncates the normalised result. The golden traces (retail code
 * run in PCSX2, tools/mipsdiff/replay_notes.md) show it: with one guard bit RtQuatConvertFromMatrix's x-dominant branch (func_003dc490) matches 12 of 12
 * recorded calls, against 3 of 12 for the truncated exact sum and 10 of 12 for round-to-nearest. For operands of the same sign the result equals the
 * truncated exact sum; for opposite signs the dropped bits make it larger in magnitude by up to an ulp.
 *
 * Division and square root round to nearest (golden traces), through the x87 unit (hosts without it: integers, below) (extended precision, not under MXCSR control, no double rounding for float
 * operands; the empty asm keeps an optimiser from narrowing the pair back to a single-precision operation, which MXCSR would truncate): x / 0 is +-FLT_MAX with the
 * sign of the operands, the square root takes the magnitude of its operand (sqrt.s of a negative number), as the interpreter models them.
 *
 * Denormal operands read as zero, exponent 255 reads as the largest finite value (the EE has no Inf/NaN), a result below the normal range flushes to
 * zero and one above FLT_MAX clamps to it. Multiplication needs nothing here: MXCSR round-toward-zero truncates the exact product, as the EE does
 * (include/p4pc/hostmath.h). tools/ee_float_cc.py rewrites every float fadd/fsub of the decomp mirror to p4pc_ee_add / p4pc_ee_sub
 * (src/platform/ee_float.c); tools/mipsdiff/r5900.c uses p4pc_ee_add_bits for its model. */
#ifndef P4PC_EEFLOAT_H
#define P4PC_EEFLOAT_H

#include <stdint.h>

static inline uint32_t p4pc_ee_add_bits(uint32_t a, uint32_t b)
{
    uint32_t ma;
    uint32_t mb;
    uint32_t sign;
    uint32_t sa;
    uint32_t sb;
    uint32_t sum;
    int ea;
    int eb;
    int d;
    int top;
    int e;

    if (((a >> 23) & 255u) == 0u) {
        a &= 0x80000000u;
    } else if (((a >> 23) & 255u) == 255u) {
        a = (a & 0x80000000u) | 0x7F7FFFFFu;
    }
    if (((b >> 23) & 255u) == 0u) {
        b &= 0x80000000u;
    } else if (((b >> 23) & 255u) == 255u) {
        b = (b & 0x80000000u) | 0x7F7FFFFFu;
    }
    if ((a & 0x7FFFFFFFu) < (b & 0x7FFFFFFFu)) {
        const uint32_t t = a;

        a = b;
        b = t;
    }
    if ((b & 0x7FFFFFFFu) == 0u) {
        return (a & 0x7FFFFFFFu) == 0u ? (a & b & 0x80000000u) : a;
    }
    ea = (int)((a >> 23) & 255u);
    eb = (int)((b >> 23) & 255u);
    sign = a & 0x80000000u;
    ma = ((a & 0x7FFFFFu) | 0x800000u) << 1;
    d = ea - eb;
    sb = ((b & 0x7FFFFFu) | 0x800000u) << 1;
    mb = d >= 26 ? 0u : sb >> d;
    sa = ma;
    sum = ((a ^ b) & 0x80000000u) ? sa - mb : sa + mb;
    if (sum == 0u) {
        return 0u;
    }
    top = 31 - __builtin_clz(sum);
    e = ea + (top - 24);
    if (e <= 0) {
        return sign;
    }
    if (e >= 255) {
        return sign | 0x7F7FFFFFu;
    }
    sum = top >= 23 ? sum >> (top - 23) : sum << (23 - top);
    return sign | ((uint32_t)e << 23) | (sum & 0x7FFFFFu);
}

static inline uint32_t p4pc_ee_sub_bits(uint32_t a, uint32_t b) { return p4pc_ee_add_bits(a, b ^ 0x80000000u); }

static inline float p4pc_ee_float_in(uint32_t x)
{
    union {
        uint32_t u;
        float f;
    } c;

    if (((x >> 23) & 255u) == 0u) {
        x &= 0x80000000u;
    } else if (((x >> 23) & 255u) == 255u) {
        x = (x & 0x80000000u) | 0x7F7FFFFFu;
    }
    c.u = x;
    return c.f;
}

/* Division, square root and reciprocal square root in integers: the same results without the x87 unit, for hosts that have none (AArch64: `long double` is a
 * software quad there, and the FPCR rounding mode the EE's multiplication needs would apply to a double operation). Correctly rounded to nearest, ties to
 * even, as the long double path is (a float result of a 64-bit significand: the double rounding it has is below anything a float quotient or root can
 * come near); tests/ee_float_test.c compares the two on millions of operands, and the long double formulas against both on every host. */
static inline uint32_t p4pc_ee_in_bits(uint32_t x)
{
    if (((x >> 23) & 255u) == 0u) {
        return x & 0x80000000u;
    }
    if (((x >> 23) & 255u) == 255u) {
        return (x & 0x80000000u) | 0x7F7FFFFFu;
    }
    return x;
}

/* The float with sign `sign` nearest to sig * 2^(eu - t) (+ a remainder below sig when `sticky`), `sig` having its top bit at position t >= 25, as the
 * EE stores it: clamped to +-FLT_MAX, and a result below the normal range keeps only the sign (a long double just under 2^-126 that the float conversion
 * rounds up to 2^-126 is the one exception). */
static inline uint32_t p4pc_ee_pack_nearest(uint32_t sign, int eu, uint64_t sig, int t, int sticky)
{
    uint32_t mant = (uint32_t)(sig >> (t - 23));
    const uint32_t guard = (uint32_t)(sig >> (t - 24)) & 1u;
    const int e0 = eu + 127;
    int e = e0;

    sticky = sticky || (sig & ((1ull << (t - 24)) - 1ull)) != 0u;
    if (e <= 0) {
        if (e == 0) {
            /* the denormal grid keeps 23 of the 24 bits: round there and see whether it carries into 2^-126 */
            const uint32_t m23 = mant >> 1;
            const uint32_t g2 = mant & 1u;

            if (g2 && (guard || sticky || (m23 & 1u)) && m23 == 0x7FFFFFu) {
                return sign | 0x00800000u;
            }
        }
        return sign;
    }
    if (guard && (sticky || (mant & 1u))) {
        mant++;
    }
    if (mant == (1u << 24)) {
        mant >>= 1;
        e++;
    }
    if (e >= 255) {
        return sign | 0x7F7FFFFFu;
    }
    return sign | ((uint32_t)e << 23) | (mant & 0x7FFFFFu);
}

static inline uint64_t p4pc_isqrt64(uint64_t n)
{
    uint64_t r = 0;
    uint64_t bit = 1ull << 62;

    while (bit > n) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (n >= r + bit) {
            n -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    return r;
}

static inline uint32_t p4pc_ee_div_bits_int(uint32_t a, uint32_t b)
{
    uint32_t sign;
    uint64_t n;
    uint64_t q;
    uint32_t ma;
    uint32_t mb;
    int t;

    a = p4pc_ee_in_bits(a);
    b = p4pc_ee_in_bits(b);
    sign = (a ^ b) & 0x80000000u;
    if ((b & 0x7FFFFFFFu) == 0u) {
        return sign | 0x7F7FFFFFu;
    }
    if ((a & 0x7FFFFFFFu) == 0u) {
        return sign;
    }
    ma = (a & 0x7FFFFFu) | 0x800000u;
    mb = (b & 0x7FFFFFu) | 0x800000u;
    n = (uint64_t)ma << 26;
    q = n / mb;
    t = (q >> 26) != 0u ? 26 : 25;
    return p4pc_ee_pack_nearest(sign, (int)((a >> 23) & 255u) - (int)((b >> 23) & 255u) + (t - 26), q, t, (n % mb) != 0u);
}

static inline uint32_t p4pc_ee_sqrt_bits_int(uint32_t b)
{
    uint64_t n;
    uint64_t s;
    uint32_t m2;
    int k;
    int t;

    b = p4pc_ee_in_bits(b);
    if ((b & 0x7FFFFFFFu) == 0u) {
        return 0u;
    }
    m2 = (b & 0x7FFFFFu) | 0x800000u;
    k = (int)((b >> 23) & 255u) - 127 - 23;
    if (k & 1) {
        m2 <<= 1;
        k -= 1;
    }
    n = (uint64_t)m2 << 28;
    s = p4pc_isqrt64(n);
    t = (s >> 26) != 0u ? 26 : 25;
    return p4pc_ee_pack_nearest(0u, t + (k >> 1) - 14, s, t, s * s != n);
}

#ifdef __SIZEOF_INT128__
/* a / sqrt(|b|): the root to 62 bits, the quotient to 61, then one rounding to float */
static inline uint32_t p4pc_ee_rsqrt_bits_int(uint32_t a, uint32_t b)
{
    unsigned __int128 n;
    unsigned __int128 r;
    unsigned __int128 bit;
    unsigned __int128 num;
    uint64_t s;
    uint64_t q;
    uint32_t sign;
    uint32_t m2;
    uint32_t ma;
    int k;
    int t;

    if ((p4pc_ee_in_bits(b) & 0x7FFFFFFFu) == 0u) {
        return (a & 0x80000000u) | 0x7F7FFFFFu;
    }
    a = p4pc_ee_in_bits(a);
    b = p4pc_ee_in_bits(b);
    sign = a & 0x80000000u;
    if ((a & 0x7FFFFFFFu) == 0u) {
        return sign;
    }
    m2 = (b & 0x7FFFFFu) | 0x800000u;
    k = (int)((b >> 23) & 255u) - 127 - 23;
    if (k & 1) {
        m2 <<= 1;
        k -= 1;
    }
    n = (unsigned __int128)m2 << 100;
    r = 0;
    bit = (unsigned __int128)1 << 126;
    while (bit > n) {
        bit >>= 2;
    }
    while (bit != 0) {
        if (n >= r + bit) {
            n -= r + bit;
            r = (r >> 1) + bit;
        } else {
            r >>= 1;
        }
        bit >>= 2;
    }
    s = (uint64_t)r;                      /* floor(sqrt(m2) * 2^50); n is what is left of m2 << 100 */
    ma = (a & 0x7FFFFFu) | 0x800000u;
    num = (unsigned __int128)ma << 100;
    q = (uint64_t)(num / s);
    t = 63 - __builtin_clzll(q);
    return p4pc_ee_pack_nearest(sign, t + (int)((a >> 23) & 255u) - 200 - (k >> 1), q, t, n != 0 || (num % s) != 0);
}
#endif

#if defined(__i386__) || defined(__x86_64__)
/* x87 versions (the host has the unit): */
/* A nearest-rounded result as the EE stores it: clamped to +-FLT_MAX, denormal and zero results keep only the sign. */
static inline uint32_t p4pc_ee_out_nearest(long double v)
{
    union {
        uint32_t u;
        float f;
    } c;
    long double m;
    uint32_t sign;

    if (v != v) {
        return 0x7F7FFFFFu;
    }
    sign = __builtin_signbitl(v) ? 0x80000000u : 0u;
    m = __builtin_fabsl(v);
    if (m >= 3.4028234663852886e38L) {
        return sign | 0x7F7FFFFFu;
    }
    __asm__("" : "+t"(m));
    c.f = (float)m;
    if (((c.u >> 23) & 255u) == 0u) {
        return sign;
    }
    return sign | c.u;
}

static inline uint32_t p4pc_ee_div_bits(uint32_t a, uint32_t b)
{
    const float x = p4pc_ee_float_in(a);
    const float y = p4pc_ee_float_in(b);
    long double q;

    if (y == 0.0f) {
        return ((a ^ b) & 0x80000000u) | 0x7F7FFFFFu;
    }
    q = (long double)x / (long double)y;
    __asm__("" : "+t"(q));
    return p4pc_ee_out_nearest(q);
}

/* rsqrt.s / vrsqrt: a / sqrt(|b|) rounded once; b == 0 gives +-FLT_MAX with the sign of a. */
static inline uint32_t p4pc_ee_rsqrt_bits(uint32_t a, uint32_t b)
{
    const float y = p4pc_ee_float_in(b);
    long double q;

    if (y == 0.0f) {
        return (a & 0x80000000u) | 0x7F7FFFFFu;
    }
    q = (long double)p4pc_ee_float_in(a) / __builtin_sqrtl(__builtin_fabsl((long double)y));
    __asm__("" : "+t"(q));
    return p4pc_ee_out_nearest(q);
}

static inline uint32_t p4pc_ee_sqrt_bits(uint32_t b)
{
    long double r = __builtin_sqrtl(__builtin_fabsl((long double)p4pc_ee_float_in(b)));

    __asm__("" : "+t"(r));
    return p4pc_ee_out_nearest(r);
}

#else
#define p4pc_ee_div_bits p4pc_ee_div_bits_int
#define p4pc_ee_sqrt_bits p4pc_ee_sqrt_bits_int
#define p4pc_ee_rsqrt_bits p4pc_ee_rsqrt_bits_int
#endif

/* The same operations as functions (src/platform/ee_float.c) for port code that ports game code by hand; division and square root round to nearest (x87, not under
 * MXCSR control; the golden traces show the EE doing that), see tools/ee_float_cc.py. */
float p4pc_ee_add(float a, float b);
float p4pc_ee_sub(float a, float b);
float p4pc_ee_div(float a, float b);
float p4pc_ee_sqrt(float a);
float p4pc_ee_rsqrt(float a, float b);

/* Double-libm uses the retail software-double operations, not host libm. */
#include "p4pc/ee_dfloat.h"

#endif /* P4PC_EEFLOAT_H */
