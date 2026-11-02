/* Host checks for the retail fp-bit helpers (src/platform/ps2_fpbit.c).
 *
 * Expected values are worked by hand from the retail asm (0x0044DB98, 0x0044DC50,
 * 0x0044DDF0, 0x0044E508), not computed from the C under test. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ps2_fpbit.h"

static int failures;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                   \
        }                                                                 \
    } while (0)

static uint32_t bits_from_float(float f)
{
    uint32_t u;
    memcpy(&u, &f, sizeof u);
    return u;
}

/* Class and sign are always defined. Exponent and fraction only for NUMBER results. */
static void expect_unpack_f(uint32_t w, int cls, int sign, int exp, uint32_t frac)
{
    P4FpF fp;

    memset(&fp, 0x5a, sizeof fp);
    float f;
    memcpy(&f, &w, sizeof f);
    func_0044dc50(&f, &fp);
    CHECK(fp.cls == cls);
    CHECK(fp.sign == sign);
    if (cls == P4_FP_NUMBER) {
        CHECK(fp.normal_exp == exp);
        CHECK(fp.frac == frac);
    }
}

static void expect_pack_f(int cls, int sign, int exp, uint32_t frac, uint32_t want)
{
    P4FpF fp = { cls, sign, exp, frac };

    CHECK(bits_from_float(func_0044db98(&fp)) == want);
}

static void expect_unpack_d(uint64_t w, int cls, int sign, int exp, uint64_t frac)
{
    P4FpD fp;

    memset(&fp, 0x5a, sizeof fp);
    double d;
    memcpy(&d, &w, sizeof d);
    func_0044ddf0(&d, &fp);
    CHECK(fp.cls == cls);
    CHECK(fp.sign == sign);
    if (cls == P4_FP_NUMBER || cls == P4_FP_QNAN || cls == P4_FP_SNAN) {
        if (cls == P4_FP_NUMBER) {
            CHECK(fp.normal_exp == exp);
        }
        CHECK(fp.frac == frac);
    }
}

static P4FpD mkd(int cls, int sign, int exp, uint64_t frac)
{
    P4FpD fp = { cls, sign, exp, frac };
    return fp;
}

static void expect_cmp(P4FpD a, P4FpD b, int want)
{
    CHECK(func_0044e508(&a, &b) == want);
}

/* Checks an unpacked result. Exponent and fraction are only checked for NUMBER results. */
static void expect_d_fields(P4FpD got, int cls, int sign, int exp, uint64_t frac)
{
    CHECK(got.cls == cls);
    CHECK(got.sign == sign);
    if (cls == P4_FP_NUMBER) {
        CHECK(got.normal_exp == exp);
        CHECK(got.frac == frac);
    }
}

static void expect_pack_d(int cls, int sign, int exp, uint64_t frac, uint64_t want)
{
    P4FpD fp = { cls, sign, exp, frac };

    CHECK(func_0044dd18(&fp) == want);
}

static void expect_add(P4FpD a, P4FpD b, int cls, int sign, int exp, uint64_t frac)
{
    P4FpD out;

    expect_d_fields(*func_0044de80(&a, &b, &out), cls, sign, exp, frac);
}

static void expect_mul(P4FpD a, P4FpD b, int cls, int sign, int exp, uint64_t frac)
{
    P4FpD out;

    expect_d_fields(*func_0044e1c0(&a, &b, &out), cls, sign, exp, frac);
}

static void expect_div(P4FpD a, P4FpD b, int cls, int sign, int exp, uint64_t frac)
{
    expect_d_fields(*func_0044e410(&a, &b), cls, sign, exp, frac);
}

int main(void)
{
    /* __unpack_f. Implicit bit at 0x40000000; frac = (mantissa << 7) | 0x40000000. */
    expect_unpack_f(0x3f800000u, P4_FP_NUMBER, 0, 0, 0x40000000u);       /* 1.0 */
    expect_unpack_f(0xc0000000u, P4_FP_NUMBER, 1, 1, 0x40000000u);       /* -2.0 */
    expect_unpack_f(0x3fc00000u, P4_FP_NUMBER, 0, 0, 0x60000000u);       /* 1.5: mant 0x400000 */
    expect_unpack_f(0x40490fdbu, P4_FP_NUMBER, 0, 1,
                    (0x490fdbu << 7) | 0x40000000u);                     /* pi */
    expect_unpack_f(0x80000000u, P4_FP_ZERO, 1, 0, 0);                   /* -0.0 */
    expect_unpack_f(0x00000001u, P4_FP_ZERO, 0, 0, 0);                   /* denormal flushed */
    expect_unpack_f(0x7f800000u, P4_FP_NUMBER, 0, 128, 0x40000000u);     /* inf is a number */
    expect_unpack_f(0x7fc00000u, P4_FP_NUMBER, 0, 128, 0x60000000u);     /* NaN is a number */

    /* __pack_f. Exponent 255 and mantissa 0x7fffff for INF, NaN and overflow. */
    expect_pack_f(P4_FP_NUMBER, 0, 0, 0x40000000u, 0x3f800000u);
    expect_pack_f(P4_FP_NUMBER, 0, 0, 0x60000000u, 0x3fc00000u);
    expect_pack_f(P4_FP_NUMBER, 1, 0, 0x60000000u, 0xbfc00000u);
    expect_pack_f(P4_FP_NUMBER, 0, 1, (0x490fdbu << 7) | 0x40000000u, 0x40490fdbu);
    expect_pack_f(P4_FP_NUMBER, 0, -126, 0x40000000u, 0x00800000u);      /* smallest normal */
    expect_pack_f(P4_FP_NUMBER, 0, -127, 0x40000000u, 0);                /* underflow: zero */
    expect_pack_f(P4_FP_NUMBER, 0, 5, 0, 0);                             /* zero fraction */
    expect_pack_f(P4_FP_NUMBER, 0, 128, 0x40000000u, 0x7f800000u);      /* biased 255: mant from frac, 0 */
    expect_pack_f(P4_FP_NUMBER, 1, 128, 0x40000000u, 0xff800000u);
    expect_pack_f(P4_FP_NUMBER, 0, 129, 0x40000000u, 0x7fffffffu);      /* overflow: biased 256 */
    expect_pack_f(P4_FP_NUMBER, 1, 129, 0x40000000u, 0xffffffffu);
    expect_pack_f(P4_FP_INF, 0, 0, 0, 0x7fffffffu);
    expect_pack_f(P4_FP_INF, 1, 0, 0, 0xffffffffu);
    expect_pack_f(P4_FP_QNAN, 0, 0, 0, 0x7fffffffu);
    expect_pack_f(P4_FP_SNAN, 0, 0, 0, 0x7fffffffu);
    expect_pack_f(P4_FP_ZERO, 1, 0, 0, 0x80000000u);
    expect_pack_f(P4_FP_ZERO, 0, 0, 0, 0);

    /* __unpack_d. Implicit bit at 1 << 60; frac = (mantissa << 8) | (1 << 60). */
    expect_unpack_d(0x3ff0000000000000ull, P4_FP_NUMBER, 0, 0, 0x1000000000000000ull); /* 1.0 */
    expect_unpack_d(0xc000000000000000ull, P4_FP_NUMBER, 1, 1, 0x1000000000000000ull); /* -2.0 */
    expect_unpack_d(0x3ff8000000000000ull, P4_FP_NUMBER, 0, 0, 0x1800000000000000ull); /* 1.5 */
    expect_unpack_d(0x7fefffffffffffffull, P4_FP_NUMBER, 0, 1023, 0x1fffffffffffff00ull);
    expect_unpack_d(0x0000000000000000ull, P4_FP_ZERO, 0, 0, 0);
    expect_unpack_d(0x8000000000000000ull, P4_FP_ZERO, 1, 0, 0);
    expect_unpack_d(0x0000000000000001ull, P4_FP_ZERO, 0, 0, 0);        /* denormal flushed */
    expect_unpack_d(0x7ff0000000000000ull, P4_FP_INF, 0, 0, 0);
    expect_unpack_d(0xfff0000000000000ull, P4_FP_INF, 1, 0, 0);
    expect_unpack_d(0x7ff8000000000000ull, P4_FP_QNAN, 0, 0, 0x0008000000000000ull);
    expect_unpack_d(0x7ff0000000000001ull, P4_FP_SNAN, 0, 0, 1);

    /* __fpcmp_parts_d. */
    P4FpD nan = mkd(P4_FP_QNAN, 0, 0, 0);
    P4FpD pinf = mkd(P4_FP_INF, 0, 0, 0);
    P4FpD ninf = mkd(P4_FP_INF, 1, 0, 0);
    P4FpD pzero = mkd(P4_FP_ZERO, 0, 0, 0);
    P4FpD nzero = mkd(P4_FP_ZERO, 1, 0, 0);
    P4FpD one = mkd(P4_FP_NUMBER, 0, 0, 0x1000000000000000ull);
    P4FpD none = mkd(P4_FP_NUMBER, 1, 0, 0x1000000000000000ull);
    P4FpD two = mkd(P4_FP_NUMBER, 0, 1, 0x1000000000000000ull);
    P4FpD ntwo = mkd(P4_FP_NUMBER, 1, 1, 0x1000000000000000ull);
    P4FpD onefive = mkd(P4_FP_NUMBER, 0, 0, 0x1800000000000000ull);
    P4FpD nonefive = mkd(P4_FP_NUMBER, 1, 0, 0x1800000000000000ull);
    P4FpD five = mkd(P4_FP_NUMBER, 0, 2, 0x1400000000000000ull);

    expect_cmp(nan, one, 1);
    expect_cmp(one, nan, 1);
    expect_cmp(pinf, pinf, 0);
    expect_cmp(ninf, pinf, -1);
    expect_cmp(pinf, ninf, 1);
    expect_cmp(pinf, five, 1);
    expect_cmp(ninf, five, -1);
    expect_cmp(five, pinf, -1);
    expect_cmp(five, ninf, 1);
    expect_cmp(pzero, nzero, 0);
    expect_cmp(pzero, one, -1);
    expect_cmp(pzero, none, 1);
    expect_cmp(none, pzero, -1);
    expect_cmp(one, pzero, 1);
    expect_cmp(one, two, -1);
    expect_cmp(two, one, 1);
    expect_cmp(none, ntwo, 1);
    expect_cmp(ntwo, none, -1);
    expect_cmp(one, none, 1);
    expect_cmp(none, one, -1);
    expect_cmp(onefive, onefive, 0);
    expect_cmp(onefive, one, 1);
    expect_cmp(one, onefive, -1);
    expect_cmp(nonefive, none, -1);

    /* __pack_d. Normal values, signed zero and DBL_MAX, as in the unpack_d cases above. */
    expect_pack_d(P4_FP_NUMBER, 0, 0, 0x1000000000000000ull, 0x3ff0000000000000ull);     /* 1.0 */
    expect_pack_d(P4_FP_NUMBER, 1, 1, 0x1000000000000000ull, 0xc000000000000000ull);     /* -2.0 */
    expect_pack_d(P4_FP_NUMBER, 0, 0, 0x1800000000000000ull, 0x3ff8000000000000ull);     /* 1.5 */
    expect_pack_d(P4_FP_NUMBER, 0, 1023, 0x1fffffffffffff00ull, 0x7fefffffffffffffull);  /* DBL_MAX */
    expect_pack_d(P4_FP_ZERO, 1, 0, 0, 0x8000000000000000ull);                          /* -0.0 */
    expect_pack_d(P4_FP_ZERO, 0, 0, 0, 0);
    /* __pack_d special classes, worked from 0x0044DD18. cls < 2 (QNAN, SNAN): exponent 2047 and
     * mantissa = (frac | 1 << 51) & 0xfffffffffffff, so the quiet bit is forced on. INF and
     * exponent overflow (normal_exp >= 1024): exponent 2047, mantissa 0. Zero fraction,
     * normal_exp < -1022 and ZERO: exponent 0, mantissa 0, sign kept. */
    expect_pack_d(P4_FP_INF, 0, 0, 0, 0x7ff0000000000000ull);
    expect_pack_d(P4_FP_INF, 1, 0, 0, 0xfff0000000000000ull);
    expect_pack_d(P4_FP_QNAN, 0, 0, 0, 0x7ff8000000000000ull);
    expect_pack_d(P4_FP_QNAN, 1, 0, 0x0008000000000000ull, 0xfff8000000000000ull);
    expect_pack_d(P4_FP_SNAN, 0, 0, 1, 0x7ff8000000000001ull);
    expect_pack_d(P4_FP_SNAN, 0, 0, 0xffffffffffffffffull, 0x7fffffffffffffffull);  /* bits above 51 masked */
    expect_pack_d(P4_FP_NUMBER, 0, 1024, 0x1000000000000000ull, 0x7ff0000000000000ull);   /* overflow */
    expect_pack_d(P4_FP_NUMBER, 1, 2000, 0x1000000000000000ull, 0xfff0000000000000ull);
    expect_pack_d(P4_FP_NUMBER, 0, -1022, 0x1000000000000000ull, 0x0010000000000000ull);  /* smallest normal */
    expect_pack_d(P4_FP_NUMBER, 0, -1023, 0x1000000000000000ull, 0);                      /* underflow */
    expect_pack_d(P4_FP_NUMBER, 1, -1023, 0x1000000000000000ull, 0x8000000000000000ull);
    expect_pack_d(P4_FP_NUMBER, 0, 5, 0, 0);                                              /* zero fraction */

    /* __add_core on unpacked operands. Results worked by hand from the values above. */
    expect_add(one, one, P4_FP_NUMBER, 0, 1, 0x1000000000000000ull);     /* 1 + 1 = 2 */
    expect_add(one, two, P4_FP_NUMBER, 0, 1, 0x1800000000000000ull);     /* 1 + 2 = 3 */
    expect_add(two, none, P4_FP_NUMBER, 0, 0, 0x1000000000000000ull);    /* 2 + -1 = 1 */
    expect_add(none, two, P4_FP_NUMBER, 0, 0, 0x1000000000000000ull);    /* -1 + 2 = 1 */
    expect_add(ntwo, one, P4_FP_NUMBER, 1, 0, 0x1000000000000000ull);    /* -2 + 1 = -1 */
    expect_add(pzero, nzero, P4_FP_ZERO, 0, 0, 0);                       /* +0 + -0 = +0 */
    expect_add(nzero, nzero, P4_FP_ZERO, 1, 0, 0);                       /* -0 + -0 = -0 */
    {
        P4FpD out, ca = pinf, cb = ninf;
        CHECK(func_0044de80(&ca, &cb, &out) == P4_FP_NAN_OBJECT);        /* INF + -INF */
    }

    /* __mul_core on unpacked operands. */
    expect_mul(onefive, two, P4_FP_NUMBER, 0, 1, 0x1800000000000000ull);  /* 1.5 * 2 = 3 */
    expect_mul(onefive, ntwo, P4_FP_NUMBER, 1, 1, 0x1800000000000000ull); /* 1.5 * -2 = -3 */
    expect_mul(nzero, two, P4_FP_ZERO, 1, 0, 0);                          /* -0 * 2 = -0 */
    expect_mul(none, pzero, P4_FP_ZERO, 1, 0, 0);                         /* -1 * +0 = -0 */
    {
        P4FpD out, ca = pinf, cb = pzero;
        CHECK(func_0044e1c0(&ca, &cb, &out) == P4_FP_NAN_OBJECT);        /* INF * 0 */
    }

    /* __mul_core right-shift path (0x0044E314 loop): taken when the high 64 bits of the product
     * exceed 0x1fffffffffffffff, which needs a fraction that is not normalised to 1 << 60.
     * Each shift halves hi and adds one to the exponent. Exponent starts at a + b + 4.
     *   0x7000000000000000 ^ 2 = 0x31 << 120, hi = 0x3100000000000000, one shift. */
    expect_mul(mkd(P4_FP_NUMBER, 1, 3, 0x7000000000000000ull),
               mkd(P4_FP_NUMBER, 0, -2, 0x7000000000000000ull),
               P4_FP_NUMBER, 1, 6, 0x1880000000000000ull);
    /* (2^64 - 1)^2 = 2^128 - 2^65 + 1, hi = 0xfffffffffffffffe, three shifts to 0x1fff... */
    expect_mul(mkd(P4_FP_NUMBER, 0, 0, 0xffffffffffffffffull),
               mkd(P4_FP_NUMBER, 0, 0, 0xffffffffffffffffull),
               P4_FP_NUMBER, 0, 7, 0x1fffffffffffffffull);
    /* __div_core on unpacked operands (the result is stored into the first operand). */
    expect_div(onefive, two, P4_FP_NUMBER, 0, -1, 0x1800000000000000ull);   /* 1.5 / 2 = 0.75 */
    expect_div(onefive, ntwo, P4_FP_NUMBER, 1, -1, 0x1800000000000000ull);  /* 1.5 / -2 = -0.75 */
    expect_div(none, pzero, P4_FP_INF, 1, 0, 0);        /* -1 / 0 = -INF */
    expect_div(pzero, two, P4_FP_ZERO, 0, 0, 0);                              /* 0 / 2 = 0 */
    {
        P4FpD ca = pzero, cb = pzero;
        CHECK(func_0044e410(&ca, &cb) == P4_FP_NAN_OBJECT);                   /* 0 / 0 */
        ca = pinf;
        cb = pinf;
        CHECK(func_0044e410(&ca, &cb) == P4_FP_NAN_OBJECT);                   /* INF / INF */
    }

    /* fptodp, dptofp, dpsub and the double multiply wrapper on exact IEEE values. */
    CHECK(fptodp(1.5f) == 0x3ff8000000000000ull);
    CHECK(fptodp(-2.0f) == 0xc000000000000000ull);
    CHECK(fptodp(0.0f) == 0);
    CHECK(bits_from_float(dptofp(0x3ff8000000000000ull)) == 0x3fc00000u);
    CHECK(bits_from_float(dptofp(0xc000000000000000ull)) == 0xc0000000u);
    CHECK(dpsub(0x4000000000000000ull, 0x3ff0000000000000ull) == 0x3ff0000000000000ull);  /* 2 - 1 = 1 */
    CHECK(dpsub(0x3ff0000000000000ull, 0x4000000000000000ull) == 0xbff0000000000000ull);  /* 1 - 2 = -1 */
    CHECK(func_0044e168(0x3ff8000000000000ull, 0x4000000000000000ull) == 0x4008000000000000ull);  /* 1.5 * 2 = 3 */
    CHECK(func_0044e168(0x3ff8000000000000ull, 0xc000000000000000ull) == 0xc008000000000000ull);  /* 1.5 * -2 = -3 */


    if (failures) {
        fprintf(stderr, "fpbit_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("fpbit_test: all checks passed\n");
    return 0;
}
