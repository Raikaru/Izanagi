#ifndef P4PC_MOD_NUMBERS_H
#define P4PC_MOD_NUMBERS_H
#include <stdint.h>
#include <string.h>

/* Integer-only IEEE conversions, independent of the game's RTZ/FTZ/DAZ mode.
 * Nearest-even f32 rounding; preserve signed zero and quiet NaNs with sign and
 * representable payload preserved. Raw memory buffers preserve signaling bits. */
uint32_t p4_mod_f32_encode(double value);
double p4_mod_f32_decode(uint32_t bits);
double p4_mod_f64_quiet(double value);
static inline double p4_mod_f32_number(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return p4_mod_f32_decode(bits);
}
static inline float p4_mod_f32_value(double value)
{
    uint32_t bits = p4_mod_f32_encode(value);
    float result;
    memcpy(&result, &bits, sizeof result);
    return result;
}
#endif
