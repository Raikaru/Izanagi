#include "p4pc/vu0.h"

/* p4pc/vu0.h (force-included into every unit through compat.h) redirects the helpers' names to the packed versions below; in this file they keep their own
 * names: the scalar helpers are the P4PC_VU0_SIMD=0 path, and the madd below is the older x86 one. */
#undef p4_vu0_mul
#undef p4_vu0_add
#undef p4_vu0_sub
#undef p4_vu0_madd
#undef p4_vu0_msub
#undef p4_vu0_mulq
#undef p4_vu0_move
#undef p4_vu0_opmula
#undef p4_vu0_opmsub
#undef p4_vu0_itof
#undef p4_vu0_ftoi

#if defined(__SSE2__)
#include <emmintrin.h>
#endif

/* vf0 is hardwired to (0,0,0,1) on the VU. Writes to it are ignored by the
 * helpers in vu0.h; the initializer sets the constant value. */
P4Vu0State p4pc_vu0 = {
    .vf = {
        [0] = { .f = { 0.0f, 0.0f, 0.0f, 1.0f } },
    },
};

#if defined(__SSE2__)
void p4_vu0_madd(int dst, unsigned mask, int s, int t, int bc)
{
    P4Vu0Reg *out;
    __m128 a, b, acc, lo, hi, result, lanes, previous;
    const __m128 limit = _mm_set1_ps(3.4028234663852886e38f);

    if (dst == 0) {
        return;
    }
    a = _mm_loadu_ps(p4pc_vu0.vf[s].f);
    b = bc < 0 ? _mm_loadu_ps(p4pc_vu0.vf[t].f) : _mm_set1_ps(p4pc_vu0.vf[t].f[bc]);
    acc = _mm_loadu_ps(p4pc_vu0.acc.f);
    /* EE mode has DAZ/FTZ enabled. Conversion of each input therefore has exactly
     * p4_vu0_in's signed-zero treatment. Keep both truncations and the intervening
     * double addition: neither a fused multiply-add nor an EE scalar add is valid. */
    lo = _mm_cvtpd_ps(_mm_mul_pd(_mm_cvtps_pd(a), _mm_cvtps_pd(b)));
    hi = _mm_cvtpd_ps(_mm_mul_pd(_mm_cvtps_pd(_mm_movehl_ps(a, a)),
                               _mm_cvtps_pd(_mm_movehl_ps(b, b))));
    lo = _mm_cvtpd_ps(_mm_add_pd(_mm_cvtps_pd(acc), _mm_cvtps_pd(lo)));
    hi = _mm_cvtpd_ps(_mm_add_pd(_mm_cvtps_pd(_mm_movehl_ps(acc, acc)), _mm_cvtps_pd(hi)));
    result = _mm_movelh_ps(lo, hi);
    /* The conversions already flush tiny results. Match store's NaN -> +0 and
     * infinity clamp, preserving signed zeros and every unselected destination bit. */
    result = _mm_and_ps(result, _mm_cmpord_ps(result, result));
    result = _mm_min_ps(limit, _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), limit), result));
    lanes = _mm_castsi128_ps(_mm_set_epi32(-(int)(mask & 1u), -(int)((mask >> 1) & 1u),
                                         -(int)((mask >> 2) & 1u), -(int)((mask >> 3) & 1u)));
    out = dst == P4_VU_ACC ? &p4pc_vu0.acc : &p4pc_vu0.vf[dst];
    previous = _mm_loadu_ps(out->f);
    _mm_storeu_ps(out->f, _mm_or_ps(_mm_and_ps(lanes, result), _mm_andnot_ps(lanes, previous)));
}
#endif

/* ---- Packed single-precision VU0 operations (P4PC_VU0_SIMD) ------------------------------------------------------------------------------------
 *
 * The inline helpers of vu0.h compute each lane in double and narrow, then fix up each lane with a chain of compares and branches. In the EE float mode
 * (round toward zero, flush-to-zero, denormals-are-zero: MXCSR on x86, FPCR on AArch64) that is the same arithmetic as one packed single-precision
 * operation:
 *   - a float product is exact in a double, so the narrowing truncates it once: the same bits as a packed multiply under RZ;
 *   - (double)a + (double)b truncated to double and then to float is the float sum truncated once (truncating to a finer grid and then to a coarser one is
 *     one truncation), as is the difference; an overflow truncates to FLT_MAX in both, and Inf/NaN operands give the same Inf/NaN, which the fix-up
 *     clamps or zeroes afterwards;
 *   - madd/msub truncate the product first, as a packed multiply and then a packed add (never fused: this file is compiled with contraction off, and each
 *     result goes through its own statement);
 *   - DAZ/FTZ read a denormal operand as a signed zero and flush a tiny result to a signed zero, as p4_vu0_in and the fix-up do (a denormal word that
 *     reaches the fix-up without an operation, p4_vu0_move for one, becomes +0 there, as before).
 * The fix-up is four lanes of integer compares on the bits (NaN -> +0, +-Inf -> +-FLT_MAX, a nonzero denormal -> +0; +-0 is kept) and the destination
 * mask is a bit blend, so every unselected bit (NaN payloads included) is preserved. tests/vu0_helpers_test.c compares every operation with the scalar
 * helpers bit for bit on random and edge patterns, all lane masks, broadcasts, destinations and aliases. P4PC_VU0_SIMD=0 (or P4PC_EE_ROUNDING=near,
 * where DAZ/FTZ are off) keeps the scalar helpers. */
#if defined(__SSE2__) || defined(__aarch64__)
#if defined(__clang__)
#pragma clang fp contract(off)
#endif
#if defined(__aarch64__)
#include <arm_neon.h>
#endif
#include "p4pc/hostmath.h"

typedef uint32_t p4v_u __attribute__((vector_size(16)));
typedef int32_t p4v_i __attribute__((vector_size(16)));
typedef float p4v_f __attribute__((vector_size(16)));
typedef uint32_t p4v_mu __attribute__((vector_size(16), aligned(4), __may_alias__));
typedef float p4v_mf __attribute__((vector_size(16), aligned(4), __may_alias__));

int p4pc_vu0_simd = 1;

/* The packed operations are exact only in the EE float mode (round toward zero, flush-to-zero, denormals-are-zero): the calling thread has to be in it. */
static int thread_in_ee_mode(void)
{
#if defined(__aarch64__)
    return (p4pc_fpcr_get() & P4PC_FPCR_EE) == P4PC_FPCR_EE;
#else
    return (_mm_getcsr() & P4PC_MXCSR_EE) == P4PC_MXCSR_EE;
#endif
}

/* src/crt.c calls this on the game thread, after p4pc_fpu_ee_mode(), with P4PC_VU0_SIMD's verdict (0 keeps the scalar helpers). */
void p4pc_vu0_simd_init(int on) { p4pc_vu0_simd = on && thread_in_ee_mode(); }

static inline p4v_f ld_f(const P4Vu0Reg *r) { return *(const p4v_mf *)r->f; }

static inline p4v_f ld_t(int t, int bc)
{
    float s;

    if (bc < 0) {
        return ld_f(&p4pc_vu0.vf[t]);
    }
    s = p4pc_vu0.vf[t].f[bc];
    return (p4v_f){s, s, s, s};
}

static inline p4v_u fixup4(p4v_u u)
{
    const p4v_i a = (p4v_i)(u & 0x7FFFFFFFu);
    const p4v_i nan = a > 0x7F800000;                          /* a is non-negative: the signed compare is the magnitude compare */
    const p4v_i inf = a == 0x7F800000;
    const p4v_i den = (a > 0) & (a < 0x00800000);
    const p4v_u clamp = (u & 0x80000000u) | 0x7F7FFFFFu;
    const p4v_u r = u & ~(p4v_u)(nan | den);

    return (r & ~(p4v_u)inf) | (clamp & (p4v_u)inf);
}

static inline p4v_u lane_mask(unsigned mask)
{
    const p4v_u m = {mask, mask, mask, mask};
    const p4v_u sel = {8u, 4u, 2u, 1u};

    return (p4v_u)((m & sel) != 0u);
}

static inline void store_bits(P4Vu0Reg *reg, unsigned mask, p4v_u v)
{
    if (mask == P4_VU_XYZW) {
        *(p4v_mu *)reg->u = v;
    } else {
        const p4v_u lanes = lane_mask(mask);

        *(p4v_mu *)reg->u = (v & lanes) | (*(const p4v_mu *)reg->u & ~lanes);
    }
}

static inline void store4(int dst, unsigned mask, p4v_f v)
{
    if (dst != 0) {
        store_bits(dst == P4_VU_ACC ? &p4pc_vu0.acc : &p4pc_vu0.vf[dst], mask, fixup4((p4v_u)v));
    }
}

void p4_vu0_mul_s(int dst, unsigned mask, int s, int t, int bc)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_mul(dst, mask, s, t, bc);
        return;
    }
    {
        const p4v_f a = ld_f(&p4pc_vu0.vf[s]);
        const p4v_f b = ld_t(t, bc);

        store4(dst, mask, a * b);
    }
}

void p4_vu0_add_s(int dst, unsigned mask, int s, int t, int bc)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_add(dst, mask, s, t, bc);
        return;
    }
    {
        const p4v_f a = ld_f(&p4pc_vu0.vf[s]);
        const p4v_f b = ld_t(t, bc);

        store4(dst, mask, a + b);
    }
}

void p4_vu0_sub_s(int dst, unsigned mask, int s, int t, int bc)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_sub(dst, mask, s, t, bc);
        return;
    }
    {
        const p4v_f a = ld_f(&p4pc_vu0.vf[s]);
        const p4v_f b = ld_t(t, bc);

        store4(dst, mask, a - b);
    }
}

void p4_vu0_madd_s(int dst, unsigned mask, int s, int t, int bc)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_madd(dst, mask, s, t, bc);
        return;
    }
    {
        const p4v_f a = ld_f(&p4pc_vu0.vf[s]);
        const p4v_f b = ld_t(t, bc);
        const p4v_f acc = ld_f(&p4pc_vu0.acc);
        const p4v_f prod = a * b;
        const p4v_f sum = acc + prod;

        store4(dst, mask, sum);
    }
}

void p4_vu0_msub_s(int dst, unsigned mask, int s, int t, int bc)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_msub(dst, mask, s, t, bc);
        return;
    }
    {
        const p4v_f a = ld_f(&p4pc_vu0.vf[s]);
        const p4v_f b = ld_t(t, bc);
        const p4v_f acc = ld_f(&p4pc_vu0.acc);
        const p4v_f prod = a * b;
        const p4v_f diff = acc - prod;

        store4(dst, mask, diff);
    }
}

void p4_vu0_mulq_s(int dst, unsigned mask, int s)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_mulq(dst, mask, s);
        return;
    }
    {
        const float q = p4pc_vu0.q;
        const p4v_f b = {q, q, q, q};

        store4(dst, mask, ld_f(&p4pc_vu0.vf[s]) * b);
    }
}

void p4_vu0_move_s(int dst, unsigned mask, int s)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_move(dst, mask, s);
        return;
    }
    store4(dst, mask, ld_f(&p4pc_vu0.vf[s]));
}

void p4_vu0_opmula_s(int s, int t)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_opmula(s, t);
        return;
    }
    {
        const p4v_f a = ld_f(&p4pc_vu0.vf[s]);
        const p4v_f b = ld_f(&p4pc_vu0.vf[t]);
        const p4v_f x = __builtin_shufflevector(a, a, 1, 2, 0, 3);
        const p4v_f y = __builtin_shufflevector(b, b, 2, 0, 1, 3);

        store4(P4_VU_ACC, P4_VU_XYZ, x * y);
    }
}

void p4_vu0_opmsub_s(int dst, int s, int t)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_opmsub(dst, s, t);
        return;
    }
    {
        const p4v_f a = ld_f(&p4pc_vu0.vf[s]);
        const p4v_f b = ld_f(&p4pc_vu0.vf[t]);
        const p4v_f acc = ld_f(&p4pc_vu0.acc);
        const p4v_f x = __builtin_shufflevector(a, a, 1, 2, 0, 3);
        const p4v_f y = __builtin_shufflevector(b, b, 2, 0, 1, 3);
        const p4v_f prod = x * y;
        const p4v_f diff = acc - prod;

        store4(dst, P4_VU_XYZ, diff);
    }
}

void p4_vu0_itof_s(int dst, unsigned mask, int s)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_itof(dst, mask, s);
        return;
    }
    {
        const p4v_i v = (p4v_i)*(const p4v_mu *)p4pc_vu0.vf[s].u;

        store4(dst, mask, __builtin_convertvector(v, p4v_f));
    }
}

void p4_vu0_ftoi_s(int dst, unsigned mask, int s)
{
    if (!p4pc_vu0_simd) {
        p4_vu0_ftoi(dst, mask, s);
        return;
    }
    if (dst != 0) {
        const p4v_f x = ld_f(&p4pc_vu0.vf[s]);
        p4v_i r;

#if defined(__aarch64__)
        r = (p4v_i)vcvtq_s32_f32((float32x4_t)x);                  /* saturates; NaN gives 0 */
#else
        {
            /* cvttps2dq gives 0x80000000 for NaN and for anything out of range: NaN becomes 0, a positive overflow INT32_MAX */
            const p4v_i raw = (p4v_i)_mm_cvttps_epi32((__m128)x);
            const p4v_i big = x >= 2147483648.0f;
            const p4v_i ord = x == x;

            r = ((raw & ~big) | (0x7FFFFFFF & big)) & ord;
        }
#endif
        store_bits(dst == P4_VU_ACC ? &p4pc_vu0.acc : &p4pc_vu0.vf[dst], mask, (p4v_u)r);
    }
}
#endif /* __SSE2__ || __aarch64__ */
