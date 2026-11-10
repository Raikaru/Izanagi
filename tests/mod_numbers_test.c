#include "p4pc/mod_numbers.h"
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#ifdef __SSE2__
#include <xmmintrin.h>
#endif

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_numbers: line%d: %s\n", __LINE__, #x); failures++; } } while (0)
static uint64_t bits_of(double number)
{
    uint64_t bits;
    memcpy(&bits, &number, sizeof bits);
    return bits;
}
static double number_of(uint64_t bits)
{
    double number;
    memcpy(&number, &bits, sizeof number);
    return number;
}
static void vectors(void)
{
    static const struct { double number; uint32_t bits; } examples[] = {
        {0.0, 0}, {-0.0, UINT32_C(0x80000000)}, {1.0, UINT32_C(0x3f800000)},
        {0x1p-149, 1}, {-0x1p-149, UINT32_C(0x80000001)},
        {0x1p-150, 0}, {-0x1p-150, UINT32_C(0x80000000)},
        {0x1.0000000000001p-150, 1},
        {0x1.000001p0, UINT32_C(0x3f800000)},
        {0x1.000003p0, UINT32_C(0x3f800002)},
        {0x1.0000010000001p0, UINT32_C(0x3f800001)},
        {0x1.fffffep127, UINT32_C(0x7f7fffff)},
        {0x1.fffffefffffffp127, UINT32_C(0x7f7fffff)},
        {0x1.ffffffp127, UINT32_C(0x7f800000)},
        {-0x1p128, UINT32_C(0xff800000)},
    };
    for (size_t i = 0; i < sizeof examples / sizeof examples[0]; i++)
        CHECK(p4_mod_f32_encode(examples[i].number) == examples[i].bits);
    CHECK(bits_of(p4_mod_f32_decode(1)) == bits_of(0x1p-149));
    CHECK(bits_of(p4_mod_f32_decode(UINT32_C(0x80000000))) == UINT64_C(0x8000000000000000));
    CHECK(bits_of(p4_mod_f32_decode(UINT32_C(0x7f800001))) == UINT64_C(0x7ff8000020000000));
    CHECK(p4_mod_f32_encode(number_of(UINT64_C(0xfff0000000000001))) == UINT32_C(0xffc00000));
    CHECK(bits_of(p4_mod_f64_quiet(number_of(UINT64_C(0xfff0000000000123)))) == UINT64_C(0xfff8000000000123));
    uint32_t state = 7;
    for (unsigned i = 0; i < 100000; i++) {
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        uint32_t expected = state;
        if ((state & UINT32_C(0x7f800000)) == UINT32_C(0x7f800000) &&
            (state & UINT32_C(0x007fffff))) expected |= UINT32_C(0x00400000);
        CHECK(p4_mod_f32_encode(p4_mod_f32_decode(state)) == expected);
    }
}
int main(void)
{
    fenv_t original;
    CHECK(fegetenv(&original) == 0);
    CHECK(fesetenv(FE_DFL_ENV) == 0);
    vectors();
    CHECK(fesetround(FE_TOWARDZERO) == 0);
#ifdef __SSE2__
    _mm_setcsr((_mm_getcsr() & ~0x6000u) | 0xe040u); /* EE RTZ + FTZ + DAZ. */
#endif
    vectors();
    CHECK(fesetenv(&original) == 0);
    printf("mod_numbers: %s (nearest-even boundaries, subnormals, signed zero, quiet NaN payloads, EE mode)\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
