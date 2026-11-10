#include "p4pc/mod_numbers.h"
#include <float.h>
#pragma STDC FENV_ACCESS ON

_Static_assert(sizeof(float) == 4 && FLT_MANT_DIG == 24 && FLT_MAX_EXP == 128,
               "guest f32 API requires IEEE binary32");
_Static_assert(sizeof(double) == 8 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024,
               "Luau numbers require IEEE binary64");

static double from_bits(uint64_t bits)
{
    double value;
    memcpy(&value, &bits, sizeof value);
    return value;
}
static uint64_t round_shift(uint64_t value, unsigned shift)
{
    uint64_t result = value >> shift;
    uint64_t remainder = value & ((UINT64_C(1) << shift) - 1);
    uint64_t half = UINT64_C(1) << (shift - 1);
    return result + (remainder > half || (remainder == half && (result & 1)));
}
uint32_t p4_mod_f32_encode(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    uint32_t sign = (uint32_t)(bits >> 63) << 31;
    unsigned exponent = (unsigned)((bits >> 52) & 2047u);
    uint64_t fraction = bits & UINT64_C(0x000fffffffffffff);
    if (exponent == 2047) {
        uint32_t payload = fraction ? (uint32_t)(fraction >> 29) | UINT32_C(0x00400000) : 0;
        return sign | UINT32_C(0x7f800000) | payload;
    }
    if (!exponent) return sign; /* Binary64 subnormals are below half an f32 ulp. */
    int power = (int)exponent - 1023;
    if (power > 127) return sign | UINT32_C(0x7f800000);
    if (power < -150) return sign;
    uint64_t mantissa = fraction | (UINT64_C(1) << 52);
    if (power < -126)
        return sign | (uint32_t)round_shift(mantissa, (unsigned)(29 - 126 - power));
    uint32_t rounded = (uint32_t)round_shift(mantissa, 29);
    if (rounded == UINT32_C(0x01000000)) { rounded >>= 1; power++; }
    if (power > 127) return sign | UINT32_C(0x7f800000);
    return sign | (uint32_t)(power + 127) << 23 | (rounded & UINT32_C(0x007fffff));
}
double p4_mod_f32_decode(uint32_t bits)
{
    uint64_t sign = (uint64_t)(bits >> 31) << 63;
    unsigned exponent = (bits >> 23) & 255u;
    uint32_t fraction = bits & UINT32_C(0x007fffff);
    if (exponent == 255) {
        uint64_t payload = fraction ? (uint64_t)(fraction | UINT32_C(0x00400000)) << 29 : 0;
        return from_bits(sign | UINT64_C(0x7ff0000000000000) | payload);
    }
    if (exponent)
        return from_bits(sign | (uint64_t)(exponent + 896u) << 52 | (uint64_t)fraction << 29);
    if (!fraction) return from_bits(sign);
    unsigned top = 31u - (unsigned)__builtin_clz(fraction);
    uint64_t significand = (uint64_t)(fraction ^ (UINT32_C(1) << top)) << (52u - top);
    return from_bits(sign | (uint64_t)(top + 874u) << 52 | significand);
}
double p4_mod_f64_quiet(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    if ((bits & UINT64_C(0x7ff0000000000000)) == UINT64_C(0x7ff0000000000000) &&
        (bits & UINT64_C(0x000fffffffffffff))) bits |= UINT64_C(0x0008000000000000);
    return from_bits(bits);
}
