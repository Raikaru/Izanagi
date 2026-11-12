/* VU0 macro-mode model for the PC port.
 *
 * Replaces the PS2 `__asm__` VU0/MMI/COP2 bridges in the decomp with C helpers
 * that operate on one shared register file (p4pc_vu0). Semantics follow the
 * EE/VU0 behaviour the decomp relies on, with these documented deviations:
 *   - Results are truncated toward zero as the EE FPU and VU0 do: the helpers compute in double (a product of two floats is exact there) and
 *     narrow to float, which truncates because MXCSR rounds toward zero (include/p4pc/hostmath.h), so add, sub and mul are one truncation of the
 *     exact result; madd/msub truncate the product first and then the sum (the golden traces, docs/porting.md). They need that mode: call p4pc_fpu_ee_mode() before using them outside the game.
 *   - Results are clamped to +-FLT_MAX, NaN becomes 0, and denormals flush to
 *     +-0, matching the VU's finite-only arithmetic.
 *   - vf0 is hardwired to (0,0,0,1); writes to it are ignored.
 * The PC build must use -mfpmath=sse -msse2 -ffp-contract=off so x87 extended
 * precision and FMA contraction do not change results.
 */
#ifndef P4PC_VU0_H
#define P4PC_VU0_H

#include <stdint.h>

#include "p4pc/eefloat.h"

/* Destination/field masks, using the VU bit layout (x=8, y=4, z=2, w=1). */
#define P4_VU_X    8u
#define P4_VU_Y    4u
#define P4_VU_Z    2u
#define P4_VU_W    1u
#define P4_VU_XYZ  (P4_VU_X | P4_VU_Y | P4_VU_Z)
#define P4_VU_XYZW (P4_VU_XYZ | P4_VU_W)

/* Broadcast lane selectors for the `.x/.y/.z/.w` operand suffixes. */
#define P4_VU_BC_X 0
#define P4_VU_BC_Y 1
#define P4_VU_BC_Z 2
#define P4_VU_BC_W 3
#define P4_VU_NOBC (-1)

/* Register index used as a destination for ACC writes. */
#define P4_VU_ACC 32

typedef union P4Vu0Reg {
    float f[4];
    int32_t i[4];
    uint32_t u[4];
} P4Vu0Reg;

typedef struct P4Vu0State {
    P4Vu0Reg vf[32];
    P4Vu0Reg acc;
    float q;
    int32_t vi[32];
} P4Vu0State;

extern P4Vu0State p4pc_vu0;

/* 128-bit MMI value; used by the pack/extend helpers below. */
typedef union P4Mmi128 {
    uint8_t b[16];
    uint16_t h[8];
    uint32_t w[4];
} P4Mmi128;

static inline float p4_vu0_fixup(float x)
{
    if (x != x) {
        return 0.0f;
    }
    if (x > 3.4028234663852886e38f) {
        return 3.4028234663852886e38f;
    }
    if (x < -3.4028234663852886e38f) {
        return -3.4028234663852886e38f;
    }
    if (x != 0.0f && x < 1.17549435e-38f && x > -1.17549435e-38f) {
        return 0.0f;
    }
    return x;
}

/* A float operand as the EE sees it: denormals read as zero. */
static inline double p4_vu0_in(float x)
{
    union {
        float f;
        uint32_t u;
    } c;

    c.f = x;
    if (((c.u >> 23) & 255u) == 0) {
        return (c.u & 0x80000000u) ? -0.0 : 0.0;
    }
    return (double)x;
}

/* Write a lane result under a destination mask. vf0 is never written. */
static inline void p4_vu0_store(int dst, unsigned mask, const float r[4])
{
    P4Vu0Reg *reg;
    int lane;

    if (dst == 0) {
        return;
    }
    reg = (dst == P4_VU_ACC) ? &p4pc_vu0.acc : &p4pc_vu0.vf[dst];
    for (lane = 0; lane < 4; lane++) {
        if (mask & (8u >> lane)) {
            reg->f[lane] = p4_vu0_fixup(r[lane]);
        }
    }
}

static inline float p4_vu0_t(int t, int lane, int bc)
{
    return p4pc_vu0.vf[t].f[bc < 0 ? lane : bc];
}

/* Pipeline ops. `s` and `t` are vf indices; `dst` is a vf index or P4_VU_ACC.
 * The *_madd/_msub forms take the accumulator as the addend. */
static inline void p4_vu0_mul(int dst, unsigned mask, int s, int t, int bc)
{
    float r[4];
    int l;

    for (l = 0; l < 4; l++) {
        r[l] = (float)(p4_vu0_in(p4pc_vu0.vf[s].f[l]) * p4_vu0_in(p4_vu0_t(t, l, bc)));
    }
    p4_vu0_store(dst, mask, r);
}

static inline void p4_vu0_add(int dst, unsigned mask, int s, int t, int bc)
{
    float r[4];
    int l;

    for (l = 0; l < 4; l++) {
        r[l] = (float)(p4_vu0_in(p4pc_vu0.vf[s].f[l]) + p4_vu0_in(p4_vu0_t(t, l, bc)));
    }
    p4_vu0_store(dst, mask, r);
}

static inline void p4_vu0_sub(int dst, unsigned mask, int s, int t, int bc)
{
    float r[4];
    int l;

    for (l = 0; l < 4; l++) {
        r[l] = (float)(p4_vu0_in(p4pc_vu0.vf[s].f[l]) - p4_vu0_in(p4_vu0_t(t, l, bc)));
    }
    p4_vu0_store(dst, mask, r);
}

#if defined(__SSE2__)
/* Keep intrinsics out of the mirrored headers. The paired implementation preserves
 * the double product -> float -> double sum -> float sequence. */
void p4_vu0_madd(int dst, unsigned mask, int s, int t, int bc);
#else
static inline void p4_vu0_madd(int dst, unsigned mask, int s, int t, int bc)
{
    float r[4];

    for (int l = 0; l < 4; l++) {
        r[l] = (float)(p4_vu0_in(p4pc_vu0.acc.f[l]) + (double)(float)(p4_vu0_in(p4pc_vu0.vf[s].f[l]) * p4_vu0_in(p4_vu0_t(t, l, bc))));
    }
    p4_vu0_store(dst, mask, r);
}
#endif

static inline void p4_vu0_msub(int dst, unsigned mask, int s, int t, int bc)
{
    float r[4];
    int l;

    for (l = 0; l < 4; l++) {
        r[l] = (float)(p4_vu0_in(p4pc_vu0.acc.f[l]) - (double)(float)(p4_vu0_in(p4pc_vu0.vf[s].f[l]) * p4_vu0_in(p4_vu0_t(t, l, bc))));
    }
    p4_vu0_store(dst, mask, r);
}

/* vmulq / vmaddq style: the operand is Q broadcast across all lanes. */
static inline void p4_vu0_mulq(int dst, unsigned mask, int s)
{
    float r[4];
    int l;

    for (l = 0; l < 4; l++) {
        r[l] = (float)(p4_vu0_in(p4pc_vu0.vf[s].f[l]) * p4_vu0_in(p4pc_vu0.q));
    }
    p4_vu0_store(dst, mask, r);
}

/* vmove: copy a vf register under a mask. */
static inline void p4_vu0_move(int dst, unsigned mask, int s)
{
    float r[4];
    int l;

    for (l = 0; l < 4; l++) {
        r[l] = p4pc_vu0.vf[s].f[l];
    }
    p4_vu0_store(dst, mask, r);
}

/* vopmula.xyz: ACC.xyz = vs x vt (cross product). */
static inline void p4_vu0_opmula(int s, int t)
{
    float r[4];
    const P4Vu0Reg *a = &p4pc_vu0.vf[s];
    const P4Vu0Reg *b = &p4pc_vu0.vf[t];

    r[0] = a->f[1] * b->f[2] - a->f[2] * b->f[1];
    r[1] = a->f[2] * b->f[0] - a->f[0] * b->f[2];
    r[2] = a->f[0] * b->f[1] - a->f[1] * b->f[0];
    r[3] = p4pc_vu0.acc.f[3];
    p4_vu0_store(P4_VU_ACC, P4_VU_XYZ, r);
}

/* vopmsub.xyz: vd.xyz = ACC.xyz - (vs x vt). */
static inline void p4_vu0_opmsub(int dst, int s, int t)
{
    float r[4];
    const P4Vu0Reg *a = &p4pc_vu0.vf[s];
    const P4Vu0Reg *b = &p4pc_vu0.vf[t];
    float cx = a->f[1] * b->f[2] - a->f[2] * b->f[1];
    float cy = a->f[2] * b->f[0] - a->f[0] * b->f[2];
    float cz = a->f[0] * b->f[1] - a->f[1] * b->f[0];

    r[0] = p4pc_vu0.acc.f[0] - cx;
    r[1] = p4pc_vu0.acc.f[1] - cy;
    r[2] = p4pc_vu0.acc.f[2] - cz;
    r[3] = p4pc_vu0.vf[dst].f[3];
    p4_vu0_store(dst, P4_VU_XYZ, r);
}

/* Store Q and mirror its IEEE bits into vi[22], which cfc2 $vi22 reads
 * after vsqrt/vrsqrt in the Bezier code (0048f5f0). */
static inline void p4_vu0_set_q(float v)
{
    union {
        float f;
        int32_t i;
    } bits;

    bits.f = v;
    p4pc_vu0.q = v;
    p4pc_vu0.vi[22] = bits.i;
}

/* vrsqrt Q, fs.bc, ft.bc: Q = fs[fbc] / sqrt(ft[tbc]). */
static inline void p4_vu0_rsqrt(int fs, int fbc, int ft, int tbc)
{
    p4_vu0_set_q(p4_vu0_fixup(p4pc_ee_rsqrt(p4pc_vu0.vf[fs].f[fbc < 0 ? 0 : fbc], p4pc_vu0.vf[ft].f[tbc < 0 ? 0 : tbc])));
}

/* vsqrt Q, ft.bc: Q = sqrt(ft[bc]). */
static inline void p4_vu0_sqrt(int ft, int bc)
{
    p4_vu0_set_q(p4_vu0_fixup(p4pc_ee_sqrt(p4pc_vu0.vf[ft].f[bc < 0 ? 0 : bc])));
}

/* vitof0: int32 lanes -> float lanes (exponent bias 0). */
static inline void p4_vu0_itof(int dst, unsigned mask, int s)
{
    float r[4];
    int l;

    for (l = 0; l < 4; l++) {
        r[l] = (float)(double)p4pc_vu0.vf[s].i[l];
    }
    p4_vu0_store(dst, mask, r);
}

/* vftoi0: float lanes -> int32 lanes, truncating and saturating. */
static inline void p4_vu0_ftoi(int dst, unsigned mask, int s)
{
    int l;
    int32_t v;

    if (dst == 0) {
        return;
    }
    for (l = 0; l < 4; l++) {
        float x = p4pc_vu0.vf[s].f[l];
        if (x != x) {
            v = 0;
        } else if (x >= 2147483648.0f) {
            v = INT32_MAX;
        } else if (x <= -2147483648.0f) {
            v = INT32_MIN;
        } else {
            v = (int32_t)x;
        }
        if (mask & (8u >> l)) {
            ((dst == P4_VU_ACC) ? &p4pc_vu0.acc : &p4pc_vu0.vf[dst])->i[l] = v;
        }
    }
}

/* Bridges between VU registers and 128-bit memory/GPR values. */
static inline void p4_vu0_lqc2(int dst, const void *addr)
{
    if (dst != 0) {
        __builtin_memcpy(&p4pc_vu0.vf[dst], addr, 16);
    }
}

static inline void p4_vu0_sqc2(void *addr, int src)
{
    __builtin_memcpy(addr, &p4pc_vu0.vf[src], 16);
}

static inline void p4_vu0_qmtc2(int dst, const void *gpr128)
{
    if (dst != 0) {
        __builtin_memcpy(&p4pc_vu0.vf[dst], gpr128, 16);
    }
}

static inline void p4_vu0_qmfc2(void *gpr128, int src)
{
    __builtin_memcpy(gpr128, &p4pc_vu0.vf[src], 16);
}

/* cfc2 reads VI register `vi`. */
static inline int32_t p4_vu0_cfc2(int vi)
{
    return p4pc_vu0.vi[vi];
}

/* 128-bit pack/extend helpers, matching the EE MMI definitions used by the
 * decomp (rs = $zero in every call site that matters for colour packing). */
static inline P4Mmi128 p4_mmi_pextlb(P4Mmi128 rs, P4Mmi128 rt)
{
    P4Mmi128 r;
    int i;

    for (i = 0; i < 8; i++) {
        r.b[2 * i] = rt.b[i];
        r.b[2 * i + 1] = rs.b[i];
    }
    return r;
}

static inline P4Mmi128 p4_mmi_pextlh(P4Mmi128 rs, P4Mmi128 rt)
{
    P4Mmi128 r;
    int i;

    for (i = 0; i < 4; i++) {
        r.h[2 * i] = rt.h[i];
        r.h[2 * i + 1] = rs.h[i];
    }
    return r;
}

static inline P4Mmi128 p4_mmi_pextuw(P4Mmi128 rs, P4Mmi128 rt)
{
    P4Mmi128 r;

    r.w[0] = rt.w[2];
    r.w[1] = rs.w[2];
    r.w[2] = rt.w[3];
    r.w[3] = rs.w[3];
    return r;
}

static inline P4Mmi128 p4_mmi_ppach(P4Mmi128 rs, P4Mmi128 rt)
{
    P4Mmi128 r;
    int i;

    for (i = 0; i < 4; i++) {
        r.h[i] = (uint16_t)rt.w[i];
        r.h[i + 4] = (uint16_t)rs.w[i];
    }
    return r;
}

static inline P4Mmi128 p4_mmi_ppacb(P4Mmi128 rs, P4Mmi128 rt)
{
    P4Mmi128 r;
    int i;

    for (i = 0; i < 8; i++) {
        r.b[i] = (uint8_t)rt.h[i];
        r.b[i + 8] = (uint8_t)rs.h[i];
    }
    return r;
}

/* Set lane x of a VU register from a 32-bit GPR value (qmtc2 of a word). */
static inline void p4_vu0_set_x(int vf, uint32_t bits)
{
    P4Mmi128 g;

    __builtin_memset(&g, 0, sizeof g);
    g.w[0] = bits;
    p4_vu0_qmtc2(vf, &g);
}

/* Set lane x of a VU register from a float value (qmtc2 of its bit pattern). */
static inline void p4_vu0_set_xf(int vf, float v)
{
    uint32_t bits;
    __builtin_memcpy(&bits, &v, 4);
    p4_vu0_set_x(vf, bits);
}

/* Unpack the four bytes of a packed colour word into four int32 lanes. */
static inline P4Mmi128 p4_vu0_word_bytes_to_lanes(uint32_t packed)
{
    P4Mmi128 g;
    P4Mmi128 z;

    __builtin_memset(&g, 0, sizeof g);
    __builtin_memset(&z, 0, sizeof z);
    g.w[0] = packed;
    return p4_mmi_pextlh(z, p4_mmi_pextlb(z, g));
}

/* Packs vf10 * 255 (lanes truncated to int32, saturated) into one word with lane x in
 * the lowest byte. Shared tail of the colour pipelines: retail's
 * `vmulx 255.0 / vftoi0 / ppach / ppacb` sequence. */
static inline uint32_t p4_vu0_pack_vf10_255(void)
{
    P4Mmi128 z;
    P4Mmi128 r;

    __builtin_memset(&z, 0, sizeof z);
    p4_vu0_set_x(2, 0x437F0000u); /* 255.0f */
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_ftoi(10, P4_VU_XYZW, 10);
    p4_vu0_qmfc2(&r, 10);
    r = p4_mmi_ppach(z, r);
    r = p4_mmi_ppacb(z, r);
    return r.w[0];
}

/* Blur-filter colour pipeline (two VU passes, then pack the low bytes):
 *   vf11 = itof(a bytes) * scale
 *   vf10 = itof(b bytes) * scale * vf11, then p4_vu0_pack_vf10_255().
 *   result byte i = (int32)vf10[i] & 0xFF, lane x in the lowest byte. */
static inline uint32_t p4_vu0_packed_color_scale2(uint32_t a, uint32_t b, float scale)
{
    uint32_t sbits;
    P4Mmi128 lanes;

    __builtin_memcpy(&sbits, &scale, sizeof sbits);

    lanes = p4_vu0_word_bytes_to_lanes(a);
    p4_vu0_qmtc2(10, &lanes);
    p4_vu0_itof(10, P4_VU_XYZW, 10);
    p4_vu0_set_x(2, sbits);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_move(11, P4_VU_XYZW, 10);

    lanes = p4_vu0_word_bytes_to_lanes(b);
    p4_vu0_qmtc2(10, &lanes);
    p4_vu0_itof(10, P4_VU_XYZW, 10);
    p4_vu0_set_x(2, sbits);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    return p4_vu0_pack_vf10_255();
}

#endif /* P4PC_VU0_H */
