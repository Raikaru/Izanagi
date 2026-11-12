/* EE add.s, sub.s, div.s and sqrt.s for the decomp mirror: tools/ee_float_cc.py rewrites every float fadd, fsub, fdiv and sqrt there to these
 * (include/p4pc/eefloat.h). Built at -O2 like the rest of the tree; the helpers are bit exact and independent of MXCSR. */
#include <string.h>

#include "p4pc/eefloat.h"

static uint32_t bits_of(float f)
{
    uint32_t u;

    memcpy(&u, &f, sizeof u);
    return u;
}

static float float_of(uint32_t u)
{
    float f;

    memcpy(&f, &u, sizeof f);
    return f;
}

float p4pc_ee_add(float a, float b) { return float_of(p4pc_ee_add_bits(bits_of(a), bits_of(b))); }

float p4pc_ee_sub(float a, float b) { return float_of(p4pc_ee_sub_bits(bits_of(a), bits_of(b))); }

float p4pc_ee_div(float a, float b) { return float_of(p4pc_ee_div_bits(bits_of(a), bits_of(b))); }

float p4pc_ee_sqrt(float a) { return float_of(p4pc_ee_sqrt_bits(bits_of(a))); }

float p4pc_ee_rsqrt(float a, float b) { return float_of(p4pc_ee_rsqrt_bits(bits_of(a), bits_of(b))); }
