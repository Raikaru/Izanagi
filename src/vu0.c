#include "p4pc/vu0.h"

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
