/* The EE float mode on the host.
 *
 * The EE's float unit rounds toward zero (PCSX2: "Chop"), has no denormals and no Inf/NaN. Game code (decomp mirror, overrides, port
 * natives) is built with SSE single precision (-mfpmath=sse), so setting MXCSR to round-toward-zero, flush-to-zero and denormals-are-zero
 * on the process's thread gives it the EE's multiplication (and the VU0 helpers' truncation) without touching any operation by hand. Addition,
 * subtraction, division and square root are not plain truncations on the EE: the decomp mirror is compiled through tools/ee_float_cc.py, which routes them
 * to include/p4pc/eefloat.h and the x87 unit. P4PC_EE_ROUNDING=near turns the MXCSR mode off.
 *
 * Host libm is the exception: glibc's sinf/acosf/... under round-toward-zero return values off by more than an ulp (NaN for some inputs),
 * so the libm calls in port and override code (the decomp mirror reaches them through the bound functions in src/platform/ps2_libc.c,
 * which are port code) run in round-to-nearest for the call. Everything else is deliberately left in the EE mode, Mesa's llvmpipe
 * included: its worker threads inherit the creating thread's MXCSR, and with them truncating, the software rasteriser's colour and
 * coordinate conversions match the GS's truncating pipeline better than nearest does (every scene gate's MAD drops, docs/renderer_port.md).
 * No-ops on targets without SSE2. */
#ifndef P4PC_HOSTMATH_H
#define P4PC_HOSTMATH_H

#include <math.h>
#include <stdlib.h>
#include <string.h>


#if defined(__SSE2__) || defined(__aarch64__)
#if defined(__SSE2__)
#include <xmmintrin.h>

#define P4PC_MXCSR_RC_MASK 0x6000u
#define P4PC_MXCSR_EE (0x6000u | 0x8000u | 0x0040u) /* round toward zero, flush-to-zero, denormals-are-zero */

/* The thread's MXCSR before a host call; the matching leave restores it. */
static inline unsigned p4pc_fpu_host_enter(void)
{
    const unsigned csr = _mm_getcsr();

    if ((csr & P4PC_MXCSR_RC_MASK) != 0u) {
        _mm_setcsr(csr & ~P4PC_MXCSR_RC_MASK);
    }
    return csr;
}

static inline void p4pc_fpu_host_leave(unsigned csr)
{
    if ((csr & P4PC_MXCSR_RC_MASK) != 0u) {
        _mm_setcsr(csr);
    }
}

static inline int p4pc_fpu_ee_mode(void)
{
    const char *mode = getenv("P4PC_EE_ROUNDING");

    if (mode != NULL && strcmp(mode, "near") == 0) {
        return 0;
    }
    _mm_setcsr(_mm_getcsr() | P4PC_MXCSR_EE);
    {
        /* x87: extended precision, round to nearest (the Windows CRT starts with 53-bit precision): the nearest division and square root of
         * tools/ee_float_cc.py run on the x87 unit. */
        const unsigned short cw = 0x037F;

        __asm__ volatile("fldcw %0" : : "m"(cw));
    }
    return 1;
}

#else /* __aarch64__ */
/* AArch64: the same mode in FPCR (RMode = 0b11 round toward zero, FZ = flush denormal inputs and results to zero), per thread. The nearest-rounded division and
 * square root need nothing from the unit (include/p4pc/eefloat.h computes them in integers). */
#define P4PC_FPCR_RC_MASK (3ul << 22)
#define P4PC_FPCR_EE ((3ul << 22) | (1ul << 24))

static inline unsigned long p4pc_fpcr_get(void)
{
    unsigned long v;

    __asm__ volatile("mrs %0, fpcr" : "=r"(v));
    return v;
}

static inline void p4pc_fpcr_set(unsigned long v) { __asm__ volatile("msr fpcr, %0" : : "r"(v)); }

static inline unsigned p4pc_fpu_host_enter(void)
{
    const unsigned long csr = p4pc_fpcr_get();

    if ((csr & P4PC_FPCR_RC_MASK) != 0ul) {
        p4pc_fpcr_set(csr & ~P4PC_FPCR_RC_MASK);
    }
    return (unsigned)csr;
}

static inline void p4pc_fpu_host_leave(unsigned csr)
{
    if ((csr & P4PC_FPCR_RC_MASK) != 0ul) {
        p4pc_fpcr_set((p4pc_fpcr_get() & ~P4PC_FPCR_RC_MASK) | (csr & P4PC_FPCR_RC_MASK));
    }
}

static inline int p4pc_fpu_ee_mode(void)
{
    const char *mode = getenv("P4PC_EE_ROUNDING");

    if (mode != NULL && strcmp(mode, "near") == 0) {
        return 0;
    }
    p4pc_fpcr_set(p4pc_fpcr_get() | P4PC_FPCR_EE);
    return 1;
}
#endif

#define P4PC_HOSTMATH_F1(name) \
    static inline float p4pc_host_##name(float x) { const unsigned c = p4pc_fpu_host_enter(); const float r = name(x); p4pc_fpu_host_leave(c); return r; }
#define P4PC_HOSTMATH_F2(name) \
    static inline float p4pc_host_##name(float x, float y) { const unsigned c = p4pc_fpu_host_enter(); const float r = name(x, y); p4pc_fpu_host_leave(c); return r; }
#define P4PC_HOSTMATH_D1(name) \
    static inline double p4pc_host_##name(double x) { const unsigned c = p4pc_fpu_host_enter(); const double r = name(x); p4pc_fpu_host_leave(c); return r; }
#define P4PC_HOSTMATH_D2(name) \
    static inline double p4pc_host_##name(double x, double y) { const unsigned c = p4pc_fpu_host_enter(); const double r = name(x, y); p4pc_fpu_host_leave(c); return r; }

P4PC_HOSTMATH_F1(sinf)
P4PC_HOSTMATH_F1(cosf)
P4PC_HOSTMATH_F1(tanf)
P4PC_HOSTMATH_F1(asinf)
P4PC_HOSTMATH_F1(acosf)
P4PC_HOSTMATH_F1(atanf)
P4PC_HOSTMATH_F1(expf)
P4PC_HOSTMATH_F1(logf)
P4PC_HOSTMATH_F1(log10f)
P4PC_HOSTMATH_F2(atan2f)
P4PC_HOSTMATH_F2(powf)
P4PC_HOSTMATH_F2(fmodf)
P4PC_HOSTMATH_D1(sin)
P4PC_HOSTMATH_D1(cos)
P4PC_HOSTMATH_D1(tan)
P4PC_HOSTMATH_D1(asin)
P4PC_HOSTMATH_D1(acos)
P4PC_HOSTMATH_D1(atan)
P4PC_HOSTMATH_D1(exp)
P4PC_HOSTMATH_D1(log)
P4PC_HOSTMATH_D1(log10)
P4PC_HOSTMATH_D2(atan2)
P4PC_HOSTMATH_D2(pow)
P4PC_HOSTMATH_D2(fmod)

#define sinf(x) p4pc_host_sinf(x)
#define cosf(x) p4pc_host_cosf(x)
#define tanf(x) p4pc_host_tanf(x)
#define asinf(x) p4pc_host_asinf(x)
#define acosf(x) p4pc_host_acosf(x)
#define atanf(x) p4pc_host_atanf(x)
#define expf(x) p4pc_host_expf(x)
#define logf(x) p4pc_host_logf(x)
#define log10f(x) p4pc_host_log10f(x)
#define atan2f(y, x) p4pc_host_atan2f(y, x)
#define powf(x, y) p4pc_host_powf(x, y)
#define fmodf(x, y) p4pc_host_fmodf(x, y)
#define sin(x) p4pc_host_sin(x)
#define cos(x) p4pc_host_cos(x)
#define tan(x) p4pc_host_tan(x)
#define asin(x) p4pc_host_asin(x)
#define acos(x) p4pc_host_acos(x)
#define atan(x) p4pc_host_atan(x)
#define exp(x) p4pc_host_exp(x)
#define log(x) p4pc_host_log(x)
#define log10(x) p4pc_host_log10(x)
#define atan2(y, x) p4pc_host_atan2(y, x)
#define pow(x, y) p4pc_host_pow(x, y)
#define fmod(x, y) p4pc_host_fmod(x, y)
#else
static inline unsigned p4pc_fpu_host_enter(void) { return 0; }
static inline void p4pc_fpu_host_leave(unsigned csr) { (void)csr; }
static inline int p4pc_fpu_ee_mode(void) { return 0; }
#endif

#endif /* P4PC_HOSTMATH_H */
