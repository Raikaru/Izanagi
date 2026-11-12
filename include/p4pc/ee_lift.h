/* Helpers of the code tools/mips2c.py generates (src/platform/ee_libm.c): register arithmetic that wraps to 32 bits and the EE's float to int conversion. */
#ifndef P4PC_EE_LIFT_H
#define P4PC_EE_LIFT_H

#include <stdint.h>
#include <string.h>

#define W(x) ((intptr_t)(int32_t)(x))

static inline uint32_t bits_of(float f)
{
    uint32_t u;

    memcpy(&u, &f, sizeof u);
    return u;
}

static inline float float_of(uint32_t u)
{
    float f;

    memcpy(&f, &u, sizeof f);
    return f;
}

/* trunc.w.s: toward zero, saturating (x86 gives 0x80000000 for every out-of-range value). */
static inline int32_t ee_cvt_w(float v)
{
    if (v >= 2147483648.0f) {
        return 0x7FFFFFFF;
    }
    if (v <= -2147483648.0f) {
        return (int32_t)0x80000000u;
    }
    return (int32_t)v;
}

#endif /* P4PC_EE_LIFT_H */
