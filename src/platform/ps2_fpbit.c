/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* PS2 libgcc soft-float helpers, written from the retail asm.
 *
 * These take pointers to the fp-bit struct and convert between IEEE bit patterns and it.
 * They are not GPR-pair helpers. The behaviour follows the retail instructions, including
 * where it departs from IEEE:
 *   - exponent 0 unpacks to class ZERO (denormals flushed), whatever the mantissa;
 *   - INF, NaN and overflow on pack all give exponent 255, mantissa 0x7fffff;
 *   - normal_exp < -126 packs to zero (flush to zero).
 *
 * Checked by tests/fpbit_test.c against input/output pairs worked from the same asm. */
#include <string.h>
#include "ps2_fpbit.h"

static uint32_t f32_bits(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);
    return u;
}

static float f32_from_bits(uint32_t u)
{
    float f;
    memcpy(&f, &u, sizeof f);
    return f;
}

static uint64_t f64_bits(double d)
{
    uint64_t u;
    memcpy(&u, &d, sizeof u);
    return u;
}

/* __pack_f: struct -> float. Returned in $f0 on retail. */
float func_0044db98(const P4FpF *fp)
{
    uint32_t biased = 0;
    uint32_t mant = 0;

    if ((uint32_t)fp->cls < P4_FP_ZERO || fp->cls == P4_FP_INF) {
        biased = 255;
        mant = 0x7fffff;
    } else if (fp->cls != P4_FP_ZERO && fp->frac != 0 && fp->normal_exp >= -126) {
        int32_t b = fp->normal_exp + 127;
        if (b < 256) {
            biased = (uint32_t)b;
            mant = (fp->frac >> 7) & 0x7fffff;
        } else {
            biased = 255;
            mant = 0x7fffff;
        }
    }
    return f32_from_bits((((uint32_t)fp->sign & 1) << 31) | (biased << 23) | mant);
}

/* __unpack_f: float -> struct. Implicit bit goes at 0x40000000 (bit 30). */
void func_0044dc50(const float *in, P4FpF *out)
{
    uint32_t w = f32_bits(*in);
    uint32_t e = (w >> 23) & 0xff;

    out->sign = (int32_t)(w >> 31);
    if (e == 0) {
        out->cls = P4_FP_ZERO;
        return;
    }
    out->normal_exp = (int32_t)e - 127;
    out->cls = P4_FP_NUMBER;
    out->frac = ((w & 0x7fffff) << 7) | 0x40000000u;
}

/* __unpack_d: double -> struct. Implicit bit goes at 1 << 60 after a shift of 8. */
void func_0044ddf0(const double *in, P4FpD *out)
{
    uint64_t w = f64_bits(*in);
    uint32_t e = (uint32_t)(w >> 52) & 0x7ff;
    uint64_t frac = w & 0x000fffffffffffffULL;

    out->sign = (int32_t)(w >> 63);
    if (e == 0) {
        out->cls = P4_FP_ZERO;
    } else if (e == 0x7ff) {
        if (frac == 0) {
            out->cls = P4_FP_INF;
        } else {
            out->cls = (frac & (1ULL << 51)) ? P4_FP_QNAN : P4_FP_SNAN;
            out->frac = frac;
        }
    } else {
        out->normal_exp = (int32_t)e - 1023;
        out->cls = P4_FP_NUMBER;
        out->frac = (frac << 8) | (1ULL << 60);
    }
}

/* __fpcmp_parts_d: three-way compare of two unpacked doubles. NaN compares as 1. */
int func_0044e508(const P4FpD *a, const P4FpD *b)
{
    int mag;

    if ((uint32_t)a->cls < P4_FP_ZERO || (uint32_t)b->cls < P4_FP_ZERO) {
        return 1;
    }
    if (a->cls == P4_FP_INF && b->cls == P4_FP_INF) {
        return b->sign - a->sign;
    }
    if (a->cls == P4_FP_INF) {
        return a->sign ? -1 : 1;
    }
    if (b->cls == P4_FP_INF) {
        return b->sign ? 1 : -1;
    }
    if (a->cls == P4_FP_ZERO) {
        if (b->cls == P4_FP_ZERO) {
            return 0;
        }
        return b->sign ? 1 : -1;
    }
    if (b->cls == P4_FP_ZERO) {
        return a->sign ? -1 : 1;
    }
    if (a->sign != b->sign) {
        return a->sign ? -1 : 1;
    }
    /* Same sign, both finite and nonzero: compare magnitude, then flip for negatives. */
    if (a->normal_exp != b->normal_exp) {
        mag = a->normal_exp > b->normal_exp ? 1 : -1;
    } else if (a->frac != b->frac) {
        mag = a->frac > b->frac ? 1 : -1;
    } else {
        return 0;
    }
    return a->sign ? -mag : mag;
}

/* __pack_d: struct -> double bit pattern, returned in $v0. NaN keeps its quiet bit (51) and gets
 * exponent 2047; INF and overflow give exponent 2047 with a zero fraction; zero, denormal-range
 * and underflow give zero (flush to zero). */
uint64_t func_0044dd18(const P4FpD *fp)
{
    uint64_t mant;
    int32_t exp;

    if ((uint32_t)fp->cls < 2u) {
        exp = 2047;
        mant = (fp->frac | (1ull << 51)) & 0x000fffffffffffffull;
    } else if (fp->cls == P4_FP_INF) {
        exp = 2047;
        mant = 0;
    } else if (fp->cls == P4_FP_ZERO || fp->frac == 0 || fp->normal_exp < -1022) {
        exp = 0;
        mant = 0;
    } else if (fp->normal_exp < 1024) {
        exp = fp->normal_exp + 1023;
        mant = (fp->frac >> 8) & 0x000fffffffffffffull;
    } else {
        exp = 2047;
        mant = 0;
    }
    return mant | ((uint64_t)(exp & 0x7ff) << 52) | ((uint64_t)(fp->sign & 1) << 63);
}

/* Wrapper: stores its four arguments into a fp-bit struct and returns __pack_d of it. */
uint64_t func_0044e7a8(int32_t cls, int32_t sign, int32_t normal_exp, uint64_t frac)
{
    P4FpD fp;

    fp.cls = cls;
    fp.sign = sign;
    fp.normal_exp = normal_exp;
    fp.frac = frac;
    return func_0044dd18(&fp);
}

/* __add_core: unpacked a + b into out. Returns one of the operands (or the NaN object) when the
 * result is an operand, otherwise out. Used by dpsub after its sign flip. */
P4FpD *func_0044de80(P4FpD *a, P4FpD *b, P4FpD *out)
{
    int32_t ea, eb, rexp, d;
    uint64_t fa, fb, frac;
    int64_t t;

    if ((uint32_t)a->cls < P4_FP_ZERO)
        return a;
    if ((uint32_t)b->cls < P4_FP_ZERO)
        return b;
    if (a->cls == P4_FP_INF) {
        if (b->cls != P4_FP_INF || a->sign == b->sign)
            return a;
        return P4_FP_NAN_OBJECT;
    }
    if (b->cls == P4_FP_INF)
        return b;
    if (b->cls == P4_FP_ZERO) {
        if (a->cls != P4_FP_ZERO)
            return a;
        *out = *a;
        out->sign = a->sign & b->sign;
        return out;
    }
    if (a->cls == P4_FP_ZERO)
        return b;

    ea = a->normal_exp;
    eb = b->normal_exp;
    fa = a->frac;
    fb = b->frac;
    rexp = ea;
    d = ea - eb;
    if (d < 0)
        d = -d;
    if (d < 64) {
        if (eb < ea) {
            for (int32_t i = ea - eb; i > 0; i--)
                fb = (fb >> 1) | (fb & 1);
        } else {
            for (; rexp < eb; rexp++)
                fa = (fa >> 1) | (fa & 1);
        }
    } else if (eb < ea) {
        fb = 0;
    } else {
        fa = 0;
        rexp = eb;
    }

    if (a->sign == b->sign) {
        out->sign = a->sign;
        out->normal_exp = rexp;
        out->frac = fa + fb;
    } else {
        t = (a->sign == 0) ? (int64_t)(fa - fb) : (int64_t)(fb - fa);
        out->normal_exp = rexp;
        if (t < 0) {
            out->sign = 1;
            t = -t;
        } else {
            out->sign = 0;
        }
        out->frac = (uint64_t)t;
        frac = (uint64_t)t;
        if (frac - 1 <= 0x0FFFFFFFFFFFFFFEull) {
            do {
                frac <<= 1;
                out->frac = frac;
                out->normal_exp--;
            } while (frac <= 0x0FFFFFFFFFFFFFFFull);
        }
    }

    out->cls = P4_FP_NUMBER;
    frac = out->frac;
    if (frac > 0x1FFFFFFFFFFFFFFFull) {
        out->frac = (frac >> 1) | (frac & 1);
        out->normal_exp++;
    }
    return out;
}

/* Sets x's sign to x.sign ^ y.sign (as 0 or 1) and returns x. Covers the NaN, INF and zero cases
 * of __mul_core, where retail returns one operand with its sign combined. */
static P4FpD *mul_sign_into(P4FpD *x, const P4FpD *y)
{
    x->sign = (x->sign ^ y->sign) != 0;
    return x;
}

/* __mul_core: unpacked a * b into out. The 128-bit product is built from 32x32-bit partial
 * products as the retail MMI code does, because i386 has no 128-bit integer type. */
P4FpD *func_0044e1c0(P4FpD *a, P4FpD *b, P4FpD *out)
{
    uint64_t fa, fb, a_hi, a_lo, b_hi, b_lo;
    uint64_t p1, p2, p3, p4, cross, c1, c2, lo, hi;
    int32_t e;
    int32_t sign;

    if ((uint32_t)a->cls < P4_FP_ZERO)
        return mul_sign_into(a, b);
    if ((uint32_t)b->cls < P4_FP_ZERO)
        return mul_sign_into(b, a);
    if (a->cls == P4_FP_INF) {
        if (b->cls == P4_FP_ZERO)
            return P4_FP_NAN_OBJECT;
        return mul_sign_into(a, b);
    }
    if (b->cls == P4_FP_INF) {
        if (a->cls == P4_FP_ZERO)
            return P4_FP_NAN_OBJECT;
        return mul_sign_into(b, a);
    }
    if (a->cls == P4_FP_ZERO)
        return mul_sign_into(a, b);
    if (b->cls == P4_FP_ZERO)
        return mul_sign_into(b, a);

    fa = a->frac;
    fb = b->frac;
    e = (int32_t)((uint32_t)a->normal_exp + (uint32_t)b->normal_exp + 4u);
    sign = (a->sign ^ b->sign) != 0;

    a_hi = fa >> 32;
    a_lo = (uint32_t)fa;
    b_hi = fb >> 32;
    b_lo = (uint32_t)fb;
    p1 = (uint64_t)(uint32_t)b_hi * (uint32_t)a_lo;
    p2 = (uint64_t)(uint32_t)b_lo * (uint32_t)a_hi;
    p3 = (uint64_t)(uint32_t)b_lo * (uint32_t)a_lo;
    p4 = (uint64_t)(uint32_t)b_hi * (uint32_t)a_hi;

    /* product = p4 * 2^64 + cross * 2^32 + p3, with cross = p1 + p2 carrying into bit 64 */
    cross = p1 + p2;
    c1 = cross < p1;
    lo = p3 + ((cross & 0xFFFFFFFFu) << 32);
    c2 = lo < p3;
    hi = (c1 << 32) + c2 + (cross >> 32) + p4;

    out->normal_exp = e;
    out->sign = sign;
    if (hi > 0x1FFFFFFFFFFFFFFFull) {
        /* Not reachable for unpacked operands (hi < 2^58). Kept for retail parity. The low word
         * is not updated: the result is taken from hi, and this path never reads lo again. */
        do {
            hi >>= 1;
            e++;
            out->normal_exp = e;
        } while (hi > 0x1FFFFFFFFFFFFFFFull);
    }
    if (hi <= 0x0FFFFFFFFFFFFFFFull) {
        do {
            hi = (hi << 1) | (lo >> 63);
            lo <<= 1;
            e--;
        } while (hi <= 0x0FFFFFFFFFFFFFFFull);
        out->normal_exp = e;
    }
    out->frac = hi;
    out->cls = P4_FP_NUMBER;
    return out;
}

/* __div_core: unpacked a / b, stored into a. Returns a or b when the result is an operand (NaN
 * input, INF / INF, 0 / 0 give the NaN object or the operand), otherwise a. The quotient is the
 * retail restoring-division loop: 61 quotient bits from 1 << 60 down, no sticky bit. The result
 * sign is a.sign ^ b.sign, applied before the class tests, as the retail code does. */
P4FpD *func_0044e410(P4FpD *a, P4FpD *b)
{
    uint64_t fa, fb, q, bit;
    int less;

    if ((uint32_t)a->cls < P4_FP_ZERO)
        return a;
    if ((uint32_t)b->cls < P4_FP_ZERO)
        return b;
    a->sign ^= b->sign;
    if (a->cls == P4_FP_INF || a->cls == P4_FP_ZERO) {
        if (a->cls == b->cls)
            return P4_FP_NAN_OBJECT;
        return a;
    }
    if (b->cls == P4_FP_INF) {
        a->frac = 0;
        a->normal_exp = 0;
        return a;
    }
    if (b->cls == P4_FP_ZERO) {
        a->cls = P4_FP_INF;
        return a;
    }
    fb = b->frac;
    fa = a->frac;
    a->normal_exp -= b->normal_exp;
    less = fa < fb;
    if (less) {
        fa <<= 1;
        a->normal_exp -= 1;
        less = fa < fb;
    }
    q = 0;
    bit = 1ull << 60;
    do {
        if (less) {
            fa <<= 1;
        } else {
            q |= bit;
            fa = (fa - fb) << 1;
        }
        bit >>= 1;
        less = fa < fb;
    } while (bit != 0);
    a->frac = q;
    return a;
}

/* Wrapper for __pack_f: stores its four arguments into a fp-bit struct and returns __pack_f of
 * it. Returned in $f0. */
float func_0044dca8(int32_t cls, int32_t sign, int32_t normal_exp, uint32_t frac)
{
    P4FpF fp;

    fp.cls = cls;
    fp.sign = sign;
    fp.normal_exp = normal_exp;
    fp.frac = frac;
    return func_0044db98(&fp);
}

/* fptodp: float -> double bit pattern, returned in $v0. */
uint64_t fptodp(float f)
{
    P4FpF fp;

    func_0044dc50(&f, &fp);
    return func_0044e7a8(fp.cls, fp.sign, fp.normal_exp, (uint64_t)fp.frac << 30);
}

/* Copies double bit pattern into a double and unpacks it. */
static void unpack_d_bits(uint64_t bits, P4FpD *out)
{
    double d;

    memcpy(&d, &bits, sizeof d);
    func_0044ddf0(&d, out);
}

/* 0044e0a8 / 0044e3b8 / 0044e608: the library wrappers unpack both raw-bit
 * operands, invoke the corresponding retail core, and pack/return its result.
 * The double-libm and decimal CRT use these, not native double arithmetic. */
uint64_t func_0044e0a8(uint64_t a_bits, uint64_t b_bits)
{
    P4FpD a, b, out;

    unpack_d_bits(a_bits, &a);
    unpack_d_bits(b_bits, &b);
    return func_0044dd18(func_0044de80(&a, &b, &out));
}

uint64_t dpdiv(uint64_t a_bits, uint64_t b_bits)
{
    P4FpD a, b;

    unpack_d_bits(a_bits, &a);
    unpack_d_bits(b_bits, &b);
    return func_0044dd18(func_0044e410(&a, &b));
}

int dpcmp(uint64_t a_bits, uint64_t b_bits)
{
    P4FpD a, b;

    unpack_d_bits(a_bits, &a);
    unpack_d_bits(b_bits, &b);
    return func_0044e508(&a, &b);
}

/* 0044e658: sign comes from the signed word, the magnitude is normalized with
 * its leading bit at 60, and __pack_d supplies the raw 64-bit return value. */
uint64_t litodp(int32_t value)
{
    P4FpD fp;
    uint32_t magnitude = value < 0 ? 0u - (uint32_t)value : (uint32_t)value;

    fp.cls = magnitude ? P4_FP_NUMBER : P4_FP_ZERO;
    fp.sign = value < 0;
    fp.normal_exp = 60;
    fp.frac = magnitude;
    if (magnitude) {
        while (fp.frac <= UINT64_C(0x0fffffffffffffff)) {
            fp.frac <<= 1;
            fp.normal_exp--;
        }
    }
    return func_0044dd18(&fp);
}

/* 0044e708: NaN/zero/sub-unit inputs yield zero; infinity and exponent >=31
 * saturate by sign. A finite in-range value truncates its fraction toward zero. */
int32_t dptoli(uint64_t bits)
{
    P4FpD fp;
    uint32_t word;
    int32_t value;

    unpack_d_bits(bits, &fp);
    if ((uint32_t)fp.cls <= P4_FP_ZERO)
        return 0;
    if (fp.cls == P4_FP_INF || fp.normal_exp >= 31)
        return fp.sign ? INT32_MIN : INT32_MAX;
    if (fp.normal_exp < 0)
        return 0;
    word = (uint32_t)(fp.frac >> (60 - fp.normal_exp));
    if (fp.sign)
        word = 0u - word;
    memcpy(&value, &word, sizeof value);
    return value;
}

/* dpsub: double bit pattern a - b (a + (-b)), returned in $v0. */
uint64_t dpsub(uint64_t a_bits, uint64_t b_bits)
{
    P4FpD a, b, out;

    unpack_d_bits(a_bits, &a);
    unpack_d_bits(b_bits, &b);
    b.sign ^= 1;
    return func_0044dd18(func_0044de80(&a, &b, &out));
}

/* Double bit pattern a * b, returned in $v0. */
uint64_t func_0044e168(uint64_t a_bits, uint64_t b_bits)
{
    P4FpD a, b, out;

    unpack_d_bits(a_bits, &a);
    unpack_d_bits(b_bits, &b);
    return func_0044dd18(func_0044e1c0(&a, &b, &out));
}

/* dptofp: double bit pattern -> float. The bits below the float's fraction are folded into bit 0
 * as a sticky bit. Returned in $f0. */
float dptofp(uint64_t bits)
{
    P4FpD d;
    uint32_t frac;

    unpack_d_bits(bits, &d);
    frac = (uint32_t)(d.frac >> 30);
    if (d.frac & 0x3FFFFFFFu)
        frac |= 1;
    return func_0044dca8(d.cls, d.sign, d.normal_exp, frac);
}

/* __floatdidf (0044ba00) deliberately uses the same split-word operations as
 * retail. In particular, the final addition retains the EE adder's rounding. */
uint64_t func_0044ba00(int64_t value)
{
    uint64_t high = litodp((int32_t)(value >> 32));
    uint32_t low_bits = (uint32_t)value;
    int32_t low;
    memcpy(&low, &low_bits, sizeof low);
    high = func_0044e168(high, UINT64_C(0x40f0000000000000));
    high = func_0044e168(high, UINT64_C(0x40f0000000000000));
    uint64_t tail = litodp(low);
    if (low < 0)
        tail = func_0044e0a8(tail, UINT64_C(0x41f0000000000000));
    return func_0044e0a8(high, tail);
}

/* __fixunsdfsi (0044e830): zero/NaN/negative values yield zero; positive
 * infinity or exponent >=32 yields all ones. Exponent-zero inputs flush. */
uint32_t func_0044e830(uint64_t bits)
{
    P4FpD fp;
    unpack_d_bits(bits, &fp);
    if ((uint32_t)fp.cls <= P4_FP_ZERO || fp.sign)
        return 0;
    if (fp.cls == P4_FP_INF || fp.normal_exp >= 32)
        return UINT32_MAX;
    if (fp.normal_exp < 0)
        return 0;
    return (uint32_t)(fp.frac >> (60 - fp.normal_exp));
}

/* __fixunsdfdi (0044bb60) reconstructs two unsigned words. This is not a
 * saturating mathematical uint64 cast: its out-of-range/special results follow
 * the original word converter and wrapping doubleword arithmetic. */
uint64_t func_0044bb60(uint64_t bits)
{
    if (dpcmp(bits, 0) < 0)
        return 0;
    uint64_t high = (uint64_t)func_0044e830(
        func_0044e168(bits, UINT64_C(0x3df0000000000000))) << 32;
    uint64_t high_double;
    if (high >> 63) {
        uint64_t half = (high >> 1) | (high & 1);
        high_double = func_0044ba00((int64_t)half);
        high_double = func_0044e0a8(high_double, high_double);
    } else {
        high_double = func_0044ba00((int64_t)high);
    }
    uint64_t tail = dpsub(bits, high_double);
    if (dpcmp(tail, 0) < 0)
        return high - func_0044e830(dpsub(0, tail));
    return high + func_0044e830(tail);
}

/* __fixdfdi (0044bc68) applies the sign outside __fixunsdfdi, including its
 * unusual infinity/overflow behavior. Keep the negation unsigned. */
int64_t func_0044bc68(uint64_t bits)
{
    uint64_t result = dpcmp(bits, 0) < 0
        ? UINT64_C(0) - func_0044bb60(dpsub(0, bits))
        : func_0044bb60(bits);
    int64_t signed_result;
    memcpy(&signed_result, &result, sizeof signed_result);
    return signed_result;
}
