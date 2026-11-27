/* EE add.s, sub.s, div.s and sqrt.s for the decomp mirror: tools/ee_float_cc.py rewrites every float fadd, fsub, fdiv and sqrt there to these
 * (include/p4pc/eefloat.h). Built at -O2 like the rest of the tree; the helpers are bit exact and independent of MXCSR. */
#include <string.h>

#include "p4pc/eefloat.h"
#include "p4pc/hostmath.h"

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

/* The add/sub through the host adder (p4pc_ee_add_hw_bits, include/p4pc/eefloat.h), on the game thread once it is in the EE float mode (src/crt.c reads
 * P4PC_EE_ADD_HW and calls p4pc_ee_float_init, which checks the mode): 0 = the integer adder (every host, any other thread), 1 = the host adder, 2 = the host
 * adder checked against the integer one on every call (P4PC_EE_ADD_HW=check: a mismatch is reported through p4pc_ee_add_report and the integer result is
 * returned). */
int p4pc_ee_add_hw = 0;
void (*p4pc_ee_add_report)(uint32_t a, uint32_t b);   /* set by src/crt.c: the trap that names the operands of a mismatch */

/* The host adder is the EE's only in the EE float mode (round toward zero, flush-to-zero, denormals-are-zero): the calling thread has to be in it. */
static int thread_in_ee_mode(void)
{
#if defined(__aarch64__)
    return (p4pc_fpcr_get() & P4PC_FPCR_EE) == P4PC_FPCR_EE;
#elif defined(__SSE2__)
    return (_mm_getcsr() & P4PC_MXCSR_EE) == P4PC_MXCSR_EE;
#else
    return 0;
#endif
}

void p4pc_ee_float_init(int mode) { p4pc_ee_add_hw = thread_in_ee_mode() ? mode : 0; }

static uint32_t add_checked(uint32_t a, uint32_t b)
{
    const uint32_t want = p4pc_ee_add_bits(a, b);
    const uint32_t got = p4pc_ee_add_hw_bits(a, b);

    if (got != want && p4pc_ee_add_report != NULL) {
        p4pc_ee_add_report(a, b);
    }
    return want;
}

float p4pc_ee_add(float a, float b)
{
    const uint32_t x = bits_of(a);
    const uint32_t y = bits_of(b);

    if (p4pc_ee_add_hw == 1) {
        return float_of(p4pc_ee_add_hw_bits(x, y));
    }
    return float_of(p4pc_ee_add_hw == 0 ? p4pc_ee_add_bits(x, y) : add_checked(x, y));
}

float p4pc_ee_sub(float a, float b)
{
    const uint32_t x = bits_of(a);
    const uint32_t y = bits_of(b) ^ 0x80000000u;

    if (p4pc_ee_add_hw == 1) {
        return float_of(p4pc_ee_add_hw_bits(x, y));
    }
    return float_of(p4pc_ee_add_hw == 0 ? p4pc_ee_add_bits(x, y) : add_checked(x, y));
}

float p4pc_ee_div(float a, float b) { return float_of(p4pc_ee_div_bits(bits_of(a), bits_of(b))); }

float p4pc_ee_sqrt(float a) { return float_of(p4pc_ee_sqrt_bits(bits_of(a))); }

float p4pc_ee_rsqrt(float a, float b) { return float_of(p4pc_ee_rsqrt_bits(bits_of(a), bits_of(b))); }
