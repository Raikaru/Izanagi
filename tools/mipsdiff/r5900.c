/* R5900 interpreter over the process's guest memory (identity-mapped on i386, the guest block on a 64-bit host); see r5900.h. */
#define _GNU_SOURCE
#include "r5900.h"
#include "p4pc/eefloat.h"
#include "p4pc/guest.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/* Standalone IOP arithmetic witnesses can execute an unchanged IRX at its
 * linked low address. The EE harness retains its strict default RAM floor. */
#ifndef P4_MIPSDIFF_RAM_LO
#define P4_MIPSDIFF_RAM_LO 0x00100000u
#endif
#define RAM_LO P4_MIPSDIFF_RAM_LO
#define RAM_HI 0x02000000u

/* ---------------------------------------------------------------- EE floats */

static inline uint32_t f2b(float f) { uint32_t b; memcpy(&b, &f, 4); return b; }
static inline float b2f(uint32_t b) { float f; memcpy(&f, &b, 4); return f; }

/* The EE FPU has no NaN, Inf or denormals: exponent 255 is an ordinary (huge) number clamped to Fmax, and an
 * exponent-0 input is zero. */
int r5900_ieee;
static unsigned long r5900_stat_local[2];
unsigned long *r5900_stat = r5900_stat_local;   /* [0] EE results saturated to +-Fmax, [1] float to int conversions saturated; ovdiff points it at shared memory (every function runs in its own process) */

float r5900_ee_in(uint32_t b) {
    if (r5900_ieee) return b2f(b);
    uint32_t e = (b >> 23) & 255;
    if (e == 0) b &= 0x80000000u;
    else if (e == 255) b = (b & 0x80000000u) | 0x7F7FFFFFu;
    return b2f(b);
}

/* Results are truncated toward zero, clamped to +-Fmax, and denormal results flush to zero. */
uint32_t r5900_ee_out(long double v) {
    if (r5900_ieee) {
        /* host-float mode: IEEE specials, and the rounding the port's natives run in (MXCSR round toward zero, include/p4pc/hostmath.h) */
        float f = (float)v;

        if (f == f && !isinf(f) && fabsl((long double)f) > fabsl(v)) f = nextafterf(f, 0.0f);
        return f2b(f);
    }
    if (v != v) return 0x7F7FFFFFu;
    uint32_t sign = signbit(v) ? 0x80000000u : 0;
    long double a = fabsl(v);
    if (a >= 3.4028234663852886e38L) { if (a > 3.4028234663852886e38L) r5900_stat[0]++; return sign | 0x7F7FFFFFu; }
    float f = (float)a;
    if ((long double)f > a) f = nextafterf(f, 0.0f);
    uint32_t b = f2b(f);
    if (((b >> 23) & 255) == 0) return sign;   /* denormal or zero */
    return sign | b;
}

static uint32_t ee_out_nearest(long double v);
/* add.s / sub.s / div.s / sqrt.s: the EE operations of include/p4pc/eefloat.h, which the natives call too (tools/ee_float_cc.py), so the host-float proof pass
 * differs from the EE model only in multiplication specials, float to int conversion and NaN handling. */
static inline uint32_t ee_add(uint32_t a, uint32_t b) { return p4pc_ee_add_bits(a, b); }
static inline uint32_t ee_sub(uint32_t a, uint32_t b) { return p4pc_ee_sub_bits(a, b); }
static inline uint32_t ee_mul(uint32_t a, uint32_t b) { return r5900_ee_out((long double)r5900_ee_in(a) * (long double)r5900_ee_in(b)); }
/* div.s / sqrt.s / rsqrt.s round to nearest on the EE (the golden traces, docs/porting.md "EE float mode"); add/sub/mul/madd truncate. */
static uint32_t ee_out_nearest(long double v) {
    if (v != v) return 0x7F7FFFFFu;
    uint32_t sign = signbit(v) ? 0x80000000u : 0;
    long double a = fabsl(v);
    if (a >= 3.4028234663852886e38L) return sign | 0x7F7FFFFFu;
    uint32_t b = f2b((float)a);
    if (((b >> 23) & 255) == 0) return sign;   /* denormal or zero */
    return sign | b;
}
static uint32_t ee_div(uint32_t a, uint32_t b) {
    return p4pc_ee_div_bits(a, b);
}
static uint32_t ee_sqrt(uint32_t b) { return p4pc_ee_sqrt_bits(b); }
static uint32_t ee_rsqrt(uint32_t a, uint32_t b) {
    if (r5900_ieee) return p4pc_ee_div_bits(a, p4pc_ee_sqrt_bits(b));   /* the natives compute it as a divide by a square root */
    return p4pc_ee_rsqrt_bits(a, b);
}

/* fs * ft as the next add sees it: truncated to float first, which is what separate C operations do (the golden traces show the EE
 * doing the same: a fused single truncation matches fewer recorded calls). */
static inline long double fprod(float a, float b) {
    if (r5900_ieee) { volatile float p = a * b; return (long double)p; }
    /* madd.s / msub.s truncate the product before the add (golden traces, PCSX2's recompiler: mul then add, each truncated) */
    return (long double)b2f(r5900_ee_out((long double)a * (long double)b));
}

/* cvt.w.s / vftoi: saturates on the EE, 0x80000000 for NaN and out of range on x86. */
static inline uint32_t cvt_w(float v) {
    if (r5900_ieee) {
        if (v != v || v >= 2147483648.0f || v < -2147483648.0f) return 0x80000000u;
        return (uint32_t)(int32_t)v;
    }
    if (v >= 2147483648.0f || v <= -2147483648.0f) r5900_stat[1]++;
    return v >= 2147483648.0f ? 0x7FFFFFFFu : v <= -2147483648.0f ? 0x80000000u : (uint32_t)(int32_t)v;
}

/* ---------------------------------------------------------------- helpers */

#define SX32(v) ((uint64_t)(int64_t)(int32_t)(uint32_t)(v))

static inline void note_float(R5900 *c, uint32_t b) {
    uint32_t e = (b >> 23) & 255;
    if (e == 255 || (e == 0 && (b & 0x7FFFFF))) c->special_in = 1;
}

static void fault(R5900 *c, const char *why, uint32_t a) {
    c->fault_why = why;
    c->fault_addr = a;
    c->fault_src = 0;
    if (c->pc_now >= RAM_LO && c->pc_now < RAM_HI && !(c->pc_now & 3)) {
        uint32_t ins = *(uint32_t *)P4_G2H(c->pc_now), op = ins >> 26;
        if ((op >= 32 && op <= 47) || op == 49 || op == 54 || op == 55 || op == 57 || op == 62 || op == 63 || op == 30 || op == 31)
            c->fault_src = c->src[(ins >> 21) & 31];
    }
    longjmp(c->jb, R5900_FAULT);
}

/* LQ, SQ, LQC2 and SQC2 ignore the low four address bits on the EE, and the quadword the game means is the aligned one: no retail
 * path reaches them with a misaligned address, but a random case does. The port's 16-byte loads and stores (u_long128, SSE at -O2)
 * fault or read other bytes there, so such a case compares nothing the game can do: it is invalid (mp faults on a misaligned address). */
static inline void *mp(R5900 *c, uint32_t a, uint32_t size, uint32_t align) {
    if (a & (align - 1)) fault(c, "unaligned access", a);
    if (a < RAM_LO || a > RAM_HI - size) fault(c, "address outside RAM", a);
    return P4_G2H(a);
}

static inline uint8_t ld8(R5900 *c, uint32_t a) { return *(uint8_t *)mp(c, a, 1, 1); }
static inline uint16_t ld16(R5900 *c, uint32_t a) { return *(uint16_t *)mp(c, a, 2, 2); }
static inline uint32_t ld32(R5900 *c, uint32_t a) { return *(uint32_t *)mp(c, a, 4, 4); }
static inline uint64_t ld64(R5900 *c, uint32_t a) { return *(uint64_t *)mp(c, a, 8, 8); }
static inline void st8(R5900 *c, uint32_t a, uint8_t v) { *(uint8_t *)mp(c, a, 1, 1) = v; }
static inline void st16(R5900 *c, uint32_t a, uint16_t v) { *(uint16_t *)mp(c, a, 2, 2) = v; }
static inline void st32(R5900 *c, uint32_t a, uint32_t v) { *(uint32_t *)mp(c, a, 4, 4) = v; }
static inline void st64(R5900 *c, uint32_t a, uint64_t v) { *(uint64_t *)mp(c, a, 8, 8) = v; }

static void unsupported(R5900 *c, uint32_t ins) {
    c->bad_word = ins;
    c->bad_pc = c->pc_now;
    longjmp(c->jb, R5900_UNSUPPORTED);
}

/* ---------------------------------------------------------------- MMI lanes */

typedef struct { uint64_t lo, hi; } Q;
static inline Q getq(const R5900 *c, int n) { Q q = { c->r[n], c->rh[n] }; return q; }
static inline void putq(R5900 *c, int n, Q q) { if (n) { c->r[n] = q.lo; c->rh[n] = q.hi; } }

#define LANES(T, N, EXPR)                                                                              \
    do {                                                                                               \
        T a[N], b[N], d[N];                                                                            \
        memcpy(a, &A, 16); memcpy(b, &B, 16);                                                          \
        for (int i = 0; i < N; i++) { T x = a[i], y = b[i]; (void)x; (void)y; d[i] = (T)(EXPR); }       \
        memcpy(&D, d, 16);                                                                             \
    } while (0)

static inline int64_t clampi(int64_t v, int64_t lo, int64_t hi) { return v < lo ? lo : v > hi ? hi : v; }

static void mmi_op(R5900 *c, uint32_t ins, int table) {
    int rs = (ins >> 21) & 31, rt = (ins >> 16) & 31, rd = (ins >> 11) & 31, sa = (ins >> 6) & 31;
    Q A = getq(c, rs), B = getq(c, rt), D = { 0, 0 };
    uint8_t ab[16], bb[16], db[16];
    uint16_t ah[8], bh[8], dh[8];
    uint32_t aw[4], bw[4], dw[4];
    memcpy(ab, &A, 16); memcpy(bb, &B, 16); memcpy(ah, &A, 16); memcpy(bh, &B, 16); memcpy(aw, &A, 16); memcpy(bw, &B, 16);
    memset(db, 0, 16); memset(dh, 0, 16); memset(dw, 0, 16);
    int use_w = 0, use_h = 0, use_b = 0, use_q = 0;
    switch (table * 32 + sa) {
    /* MMI0 */
    case 0 * 32 + 0: for (int i = 0; i < 4; i++) dw[i] = aw[i] + bw[i]; use_w = 1; break;                  /* PADDW */
    case 0 * 32 + 1: for (int i = 0; i < 4; i++) dw[i] = aw[i] - bw[i]; use_w = 1; break;                  /* PSUBW */
    case 0 * 32 + 2: for (int i = 0; i < 4; i++) dw[i] = (int32_t)aw[i] > (int32_t)bw[i] ? ~0u : 0; use_w = 1; break;
    case 0 * 32 + 3: for (int i = 0; i < 4; i++) dw[i] = (int32_t)aw[i] > (int32_t)bw[i] ? aw[i] : bw[i]; use_w = 1; break;
    case 0 * 32 + 4: for (int i = 0; i < 8; i++) dh[i] = ah[i] + bh[i]; use_h = 1; break;                  /* PADDH */
    case 0 * 32 + 5: for (int i = 0; i < 8; i++) dh[i] = ah[i] - bh[i]; use_h = 1; break;
    case 0 * 32 + 6: for (int i = 0; i < 8; i++) dh[i] = (int16_t)ah[i] > (int16_t)bh[i] ? 0xFFFF : 0; use_h = 1; break;
    case 0 * 32 + 7: for (int i = 0; i < 8; i++) dh[i] = (int16_t)ah[i] > (int16_t)bh[i] ? ah[i] : bh[i]; use_h = 1; break;
    case 0 * 32 + 8: for (int i = 0; i < 16; i++) db[i] = ab[i] + bb[i]; use_b = 1; break;                  /* PADDB */
    case 0 * 32 + 9: for (int i = 0; i < 16; i++) db[i] = ab[i] - bb[i]; use_b = 1; break;
    case 0 * 32 + 10: for (int i = 0; i < 16; i++) db[i] = (int8_t)ab[i] > (int8_t)bb[i] ? 0xFF : 0; use_b = 1; break;
    case 0 * 32 + 16: for (int i = 0; i < 4; i++) dw[i] = (uint32_t)clampi((int64_t)(int32_t)aw[i] + (int32_t)bw[i], INT32_MIN, INT32_MAX); use_w = 1; break;
    case 0 * 32 + 17: for (int i = 0; i < 4; i++) dw[i] = (uint32_t)clampi((int64_t)(int32_t)aw[i] - (int32_t)bw[i], INT32_MIN, INT32_MAX); use_w = 1; break;
    case 0 * 32 + 18: dw[0] = bw[0]; dw[1] = aw[0]; dw[2] = bw[1]; dw[3] = aw[1]; use_w = 1; break;        /* PEXTLW */
    case 0 * 32 + 19: dw[0] = bw[0]; dw[1] = bw[2]; dw[2] = aw[0]; dw[3] = aw[2]; use_w = 1; break;        /* PPACW */
    case 0 * 32 + 20: for (int i = 0; i < 8; i++) dh[i] = (uint16_t)clampi((int64_t)(int16_t)ah[i] + (int16_t)bh[i], INT16_MIN, INT16_MAX); use_h = 1; break;
    case 0 * 32 + 21: for (int i = 0; i < 8; i++) dh[i] = (uint16_t)clampi((int64_t)(int16_t)ah[i] - (int16_t)bh[i], INT16_MIN, INT16_MAX); use_h = 1; break;
    case 0 * 32 + 22: for (int i = 0; i < 4; i++) { dh[2 * i] = bh[i]; dh[2 * i + 1] = ah[i]; } use_h = 1; break;   /* PEXTLH */
    case 0 * 32 + 23: for (int i = 0; i < 4; i++) { dh[i] = bh[2 * i]; dh[4 + i] = ah[2 * i]; } use_h = 1; break; /* PPACH */
    case 0 * 32 + 24: for (int i = 0; i < 16; i++) db[i] = (uint8_t)clampi((int64_t)(int8_t)ab[i] + (int8_t)bb[i], INT8_MIN, INT8_MAX); use_b = 1; break;
    case 0 * 32 + 25: for (int i = 0; i < 16; i++) db[i] = (uint8_t)clampi((int64_t)(int8_t)ab[i] - (int8_t)bb[i], INT8_MIN, INT8_MAX); use_b = 1; break;
    case 0 * 32 + 26: for (int i = 0; i < 8; i++) { db[2 * i] = bb[i]; db[2 * i + 1] = ab[i]; } use_b = 1; break;   /* PEXTLB */
    case 0 * 32 + 27: for (int i = 0; i < 8; i++) { db[i] = bb[2 * i]; db[8 + i] = ab[2 * i]; } use_b = 1; break; /* PPACB */
    /* MMI1 */
    case 1 * 32 + 1: for (int i = 0; i < 4; i++) { int32_t v = (int32_t)bw[i]; dw[i] = v == INT32_MIN ? INT32_MAX : (uint32_t)(v < 0 ? -v : v); } use_w = 1; break;
    case 1 * 32 + 2: for (int i = 0; i < 4; i++) dw[i] = aw[i] == bw[i] ? ~0u : 0; use_w = 1; break;        /* PCEQW */
    case 1 * 32 + 3: for (int i = 0; i < 4; i++) dw[i] = (int32_t)aw[i] < (int32_t)bw[i] ? aw[i] : bw[i]; use_w = 1; break;
    case 1 * 32 + 5: for (int i = 0; i < 8; i++) { int16_t v = (int16_t)bh[i]; dh[i] = v == INT16_MIN ? INT16_MAX : (uint16_t)(v < 0 ? -v : v); } use_h = 1; break;
    case 1 * 32 + 6: for (int i = 0; i < 8; i++) dh[i] = ah[i] == bh[i] ? 0xFFFF : 0; use_h = 1; break;
    case 1 * 32 + 7: for (int i = 0; i < 8; i++) dh[i] = (int16_t)ah[i] < (int16_t)bh[i] ? ah[i] : bh[i]; use_h = 1; break;
    case 1 * 32 + 10: for (int i = 0; i < 16; i++) db[i] = ab[i] == bb[i] ? 0xFF : 0; use_b = 1; break;
    case 1 * 32 + 16: for (int i = 0; i < 4; i++) { uint64_t s = (uint64_t)aw[i] + bw[i]; dw[i] = s > 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)s; } use_w = 1; break;
    case 1 * 32 + 17: for (int i = 0; i < 4; i++) dw[i] = aw[i] > bw[i] ? aw[i] - bw[i] : 0; use_w = 1; break;
    case 1 * 32 + 18: dw[0] = bw[2]; dw[1] = aw[2]; dw[2] = bw[3]; dw[3] = aw[3]; use_w = 1; break;        /* PEXTUW */
    case 1 * 32 + 20: for (int i = 0; i < 8; i++) { uint32_t s = (uint32_t)ah[i] + bh[i]; dh[i] = s > 0xFFFF ? 0xFFFF : (uint16_t)s; } use_h = 1; break;
    case 1 * 32 + 21: for (int i = 0; i < 8; i++) dh[i] = ah[i] > bh[i] ? ah[i] - bh[i] : 0; use_h = 1; break;
    case 1 * 32 + 22: for (int i = 0; i < 4; i++) { dh[2 * i] = bh[4 + i]; dh[2 * i + 1] = ah[4 + i]; } use_h = 1; break; /* PEXTUH */
    case 1 * 32 + 24: for (int i = 0; i < 16; i++) { uint32_t s = (uint32_t)ab[i] + bb[i]; db[i] = s > 255 ? 255 : (uint8_t)s; } use_b = 1; break; /* PADDUB */
    case 1 * 32 + 25: for (int i = 0; i < 16; i++) db[i] = ab[i] > bb[i] ? ab[i] - bb[i] : 0; use_b = 1; break;
    case 1 * 32 + 26: for (int i = 0; i < 8; i++) { db[2 * i] = bb[8 + i]; db[2 * i + 1] = ab[8 + i]; } use_b = 1; break; /* PEXTUB */
    case 1 * 32 + 27: {                                                                                   /* QFSRV */
        uint32_t shift = (c->sa & 0xF) << 3;
        /* rd = ({rs, rt} >> shift)[127:0] */
        uint64_t x[2] = { A.lo, A.hi }, y[2] = { B.lo, B.hi }, r[2];
        if (shift == 0) { r[0] = y[0]; r[1] = y[1]; }
        else if (shift >= 64) {
            uint32_t s2 = shift - 64;
            r[0] = s2 ? (y[1] >> s2) | (x[0] << (64 - s2)) : y[1];
            r[1] = s2 ? (x[0] >> s2) | (x[1] << (64 - s2)) : x[0];
        } else {
            r[0] = (y[0] >> shift) | (y[1] << (64 - shift));
            r[1] = (y[1] >> shift) | (x[0] << (64 - shift));
        }
        D.lo = r[0]; D.hi = r[1]; use_q = 1;
        break;
    }
    /* MMI2 */
    case 2 * 32 + 8: D.lo = c->hi; D.hi = c->hi1; use_q = 1; break;                                       /* PMFHI: both banks of the one 128-bit HI */
    case 2 * 32 + 9: D.lo = c->lo; D.hi = c->lo1; use_q = 1; break;                                       /* PMFLO */
    case 2 * 32 + 10: for (int i = 0; i < 4; i++) { dh[2 * i] = bh[i]; dh[2 * i + 1] = ah[4 + i]; } use_h = 1; break; /* PINTH */
    case 2 * 32 + 14: D.lo = B.lo; D.hi = A.lo; use_q = 1; break;                                          /* PCPYLD */
    case 2 * 32 + 17: for (int i = 0; i < 4; i++) dw[i] = (uint32_t)((int32_t)(int16_t)ah[2 * i] * (int16_t)bh[2 * i] + (int32_t)(int16_t)ah[2 * i + 1] * (int16_t)bh[2 * i + 1]); use_w = 1; break;  /* PHMADH (rd only) */
    case 2 * 32 + 18: D.lo = A.lo & B.lo; D.hi = A.hi & B.hi; use_q = 1; break;                            /* PAND */
    case 2 * 32 + 19: D.lo = A.lo ^ B.lo; D.hi = A.hi ^ B.hi; use_q = 1; break;                            /* PXOR */
    case 2 * 32 + 26: { static const int e[8] = { 2, 1, 0, 3, 6, 5, 4, 7 }; for (int i = 0; i < 8; i++) dh[i] = bh[e[i]]; use_h = 1; break; } /* PEXEH */
    case 2 * 32 + 27: for (int i = 0; i < 4; i++) { dh[i] = bh[3 - i]; dh[4 + i] = bh[7 - i]; } use_h = 1; break; /* PREVH */
    case 2 * 32 + 30: dw[0] = bw[2]; dw[1] = bw[1]; dw[2] = bw[0]; dw[3] = bw[3]; use_w = 1; break;        /* PEXEW */
    case 2 * 32 + 31: dw[0] = bw[1]; dw[1] = bw[2]; dw[2] = bw[0]; dw[3] = bw[3]; use_w = 1; break;        /* PROT3W */
    /* MMI3 */
    case 3 * 32 + 10: for (int i = 0; i < 4; i++) { dh[2 * i] = bh[2 * i]; dh[2 * i + 1] = ah[2 * i]; } use_h = 1; break; /* PINTEH */
    case 3 * 32 + 14: D.lo = A.hi; D.hi = B.hi; use_q = 1; break;                                          /* PCPYUD: rs upper first, unlike PCPYLD */
    case 3 * 32 + 18: D.lo = A.lo | B.lo; D.hi = A.hi | B.hi; use_q = 1; break;                            /* POR */
    case 3 * 32 + 19: D.lo = ~(A.lo | B.lo); D.hi = ~(A.hi | B.hi); use_q = 1; break;                       /* PNOR */
    case 3 * 32 + 26: { static const int e[8] = { 0, 2, 1, 3, 4, 6, 5, 7 }; for (int i = 0; i < 8; i++) dh[i] = bh[e[i]]; use_h = 1; break; } /* PEXCH */
    case 3 * 32 + 27: for (int i = 0; i < 4; i++) { dh[i] = bh[0]; dh[4 + i] = bh[4]; } use_h = 1; break;  /* PCPYH */
    case 3 * 32 + 30: dw[0] = bw[0]; dw[1] = bw[2]; dw[2] = bw[1]; dw[3] = bw[3]; use_w = 1; break;        /* PEXCW */
    default: unsupported(c, ins);
    }
    if (use_w) memcpy(&D, dw, 16);
    else if (use_h) memcpy(&D, dh, 16);
    else if (use_b) memcpy(&D, db, 16);
    (void)use_q; (void)rt;
    putq(c, rd, D);
}

/* ---------------------------------------------------------------- VU0 (COP2 macro mode) */

static inline uint32_t vu_in_bits(uint32_t b) { return f2b(r5900_ee_in(b)); }

static uint32_t vu_mulbits(uint32_t a, uint32_t b) { return ee_mul(a, b); }

static int32_t vu_ftoi(uint32_t bits, int fixed) {
    return (int32_t)cvt_w((float)((long double)r5900_ee_in(bits) * (long double)(1u << fixed)));
}

static uint32_t vu_itof(int32_t v, int fixed) { return r5900_ee_out((long double)v / (long double)(1u << fixed)); }

static void vu_op_inner(R5900 *c, uint32_t ins) {
    const uint32_t fn = ins & 0x3F;
    const int dest = (ins >> 21) & 15;              /* bit 3 = x ... bit 0 = w */
    const int ft = (ins >> 16) & 31, fs = (ins >> 11) & 31, fd = (ins >> 6) & 31;
    const uint32_t I = c->vi_;
    uint32_t *S = c->vf[fs], *T = c->vf[ft], *Dd = c->vf[fd];
    uint32_t tmp[4];
    uint32_t bcv[4];
    /* The operand broadcast by the instruction: x,y,z,w, I, Q. */
    int bc = -1;
    uint32_t *acc = c->vacc;
    enum { ADD, SUB, MUL, MAX, MINI, MADD, MSUB, NONE } kind = NONE;
    int to_acc = 0, src_kind = 0; /* 0 = vector T, 1 = broadcast */
    uint32_t bval = 0;

    if (fn < 0x3C) {
        if (fn < 0x04) { kind = ADD; bc = fn & 3; }
        else if (fn < 0x08) { kind = SUB; bc = fn & 3; }
        else if (fn < 0x0C) { kind = MADD; bc = fn & 3; }
        else if (fn < 0x10) { kind = MSUB; bc = fn & 3; }
        else if (fn < 0x14) { kind = MAX; bc = fn & 3; }
        else if (fn < 0x18) { kind = MINI; bc = fn & 3; }
        else if (fn < 0x1C) { kind = MUL; bc = fn & 3; }
        else switch (fn) {
        case 0x1C: kind = MUL; bc = 5; break;     /* MULq */
        case 0x1D: kind = MAX; bc = 4; break;     /* MAXi */
        case 0x1E: kind = MUL; bc = 4; break;     /* MULi */
        case 0x1F: kind = MINI; bc = 4; break;    /* MINIi */
        case 0x20: kind = ADD; bc = 5; break;
        case 0x21: kind = MADD; bc = 5; break;
        case 0x22: kind = ADD; bc = 4; break;
        case 0x23: kind = MADD; bc = 4; break;
        case 0x24: kind = SUB; bc = 5; break;
        case 0x25: kind = MSUB; bc = 5; break;
        case 0x26: kind = SUB; bc = 4; break;
        case 0x27: kind = MSUB; bc = 4; break;
        case 0x28: kind = ADD; bc = -1; break;
        case 0x29: kind = MADD; bc = -1; break;
        case 0x2A: kind = MUL; bc = -1; break;
        case 0x2B: kind = MAX; bc = -1; break;
        case 0x2C: kind = SUB; bc = -1; break;
        case 0x2D: kind = MSUB; bc = -1; break;
        case 0x2E:                                 /* OPMSUB */
            {
                uint32_t r[3];
                r[0] = r5900_ee_out((long double)r5900_ee_in(acc[0]) - (long double)r5900_ee_in(S[1]) * r5900_ee_in(T[2]));
                r[1] = r5900_ee_out((long double)r5900_ee_in(acc[1]) - (long double)r5900_ee_in(S[2]) * r5900_ee_in(T[0]));
                r[2] = r5900_ee_out((long double)r5900_ee_in(acc[2]) - (long double)r5900_ee_in(S[0]) * r5900_ee_in(T[1]));
                if (dest & 8) Dd[0] = r[0];
                if (dest & 4) Dd[1] = r[1];
                if (dest & 2) Dd[2] = r[2];
                return;
            }
        case 0x2F: kind = MINI; bc = -1; break;
        case 0x30: if (fd) c->vi[fd & 15] = (c->vi[ft & 15] + c->vi[fs & 15]) & 0xFFFF; /* note: vi dest is fd */ return;
        case 0x31: c->vi[fd & 15] = (c->vi[fs & 15] - c->vi[ft & 15]) & 0xFFFF; return;   /* VISUB (fd) */
        case 0x32: c->vi[ft & 15] = (c->vi[fs & 15] + (int32_t)((int32_t)(ins >> 6 & 31) << 27 >> 27)) & 0xFFFF; return; /* VIADDI */
        case 0x34: c->vi[fd & 15] = c->vi[fs & 15] & c->vi[ft & 15]; return;
        case 0x35: c->vi[fd & 15] = c->vi[fs & 15] | c->vi[ft & 15]; return;
        default: unsupported(c, ins);
        }
        switch (bc) {
        case 0: case 1: case 2: case 3: bval = T[bc]; src_kind = 1; break;
        case 4: bval = I; src_kind = 1; break;
        case 5: bval = c->vq; src_kind = 1; break;
        default: src_kind = 0;
        }
        for (int k = 0; k < 4; k++) {
            if (!(dest & (8 >> k))) continue;
            uint32_t t = src_kind ? bval : T[k];
            uint32_t r;
            long double a = r5900_ee_in(S[k]), b = r5900_ee_in(t);
            switch (kind) {
            case ADD: r = r5900_ee_out(a + b); break;
            case SUB: r = r5900_ee_out(a - b); break;
            case MUL: r = r5900_ee_out(a * b); break;
            case MAX: r = (float)a > (float)b ? f2b((float)a) : f2b((float)b); break;
            case MINI: r = (float)a < (float)b ? f2b((float)a) : f2b((float)b); break;
            case MADD: r = r5900_ee_out((long double)r5900_ee_in(acc[k]) + fprod((float)a, (float)b)); break;
            default: r = r5900_ee_out((long double)r5900_ee_in(acc[k]) - fprod((float)a, (float)b)); break;
            }
            (void)to_acc;
            Dd[k] = r;
        }
        (void)vu_mulbits; (void)bcv; (void)tmp;
        return;
    }
    /* fn 0x3C..0x3F: sub-table indexed by (fd << 2 | fn & 3) */
    const int idx = (fd << 2) | (fn & 3);
    int bci = -1;
    int akind = -1;   /* 0 ADDA, 1 SUBA, 2 MADDA, 3 MSUBA, 4 MULA */
    if (idx < 0x04) { akind = 0; bci = idx & 3; }
    else if (idx < 0x08) { akind = 1; bci = idx & 3; }
    else if (idx < 0x0C) { akind = 2; bci = idx & 3; }
    else if (idx < 0x10) { akind = 3; bci = idx & 3; }
    else if (idx < 0x14) {                                                          /* VITOF0/4/12/15 */
        static const int fx[4] = { 0, 4, 12, 15 };
        for (int k = 0; k < 4; k++) if (dest & (8 >> k)) T[k] = vu_itof((int32_t)S[k], fx[idx & 3]);
        return;
    } else if (idx < 0x18) {                                                        /* VFTOI0/4/12/15 */
        static const int fx[4] = { 0, 4, 12, 15 };
        for (int k = 0; k < 4; k++) if (dest & (8 >> k)) T[k] = (uint32_t)vu_ftoi(S[k], fx[idx & 3]);
        return;
    } else if (idx < 0x1C) { akind = 4; bci = idx & 3; }
    else switch (idx) {
    case 0x1C: akind = 4; bci = 5; break;
    case 0x1D: for (int k = 0; k < 4; k++) if (dest & (8 >> k)) T[k] = S[k] & 0x7FFFFFFFu; return;   /* VABS */
    case 0x1E: akind = 4; bci = 4; break;
    case 0x1F: {                                                                                   /* VCLIPw */
        float w = fabsf(r5900_ee_in(T[3]));
        float x = r5900_ee_in(S[0]), y = r5900_ee_in(S[1]), z = r5900_ee_in(S[2]);
        uint32_t f = 0;
        f |= x > w ? 1u : 0; f |= x < -w ? 2u : 0; f |= y > w ? 4u : 0; f |= y < -w ? 8u : 0; f |= z > w ? 16u : 0; f |= z < -w ? 32u : 0;
        c->vcli = ((c->vcli << 6) | f) & 0xFFFFFF;
        return;
    }
    case 0x20: akind = 0; bci = 5; break;
    case 0x21: akind = 2; bci = 5; break;
    case 0x22: akind = 0; bci = 4; break;
    case 0x23: akind = 2; bci = 4; break;
    case 0x24: akind = 1; bci = 5; break;
    case 0x25: akind = 3; bci = 5; break;
    case 0x26: akind = 1; bci = 4; break;
    case 0x27: akind = 3; bci = 4; break;
    case 0x28: akind = 0; bci = -1; break;
    case 0x29: akind = 2; bci = -1; break;
    case 0x2A: akind = 4; bci = -1; break;
    case 0x2C: akind = 1; bci = -1; break;
    case 0x2D: akind = 3; bci = -1; break;
    case 0x2E:                                                                                      /* OPMULA */
        if (dest & 8) acc[0] = ee_mul(S[1], T[2]);
        if (dest & 4) acc[1] = ee_mul(S[2], T[0]);
        if (dest & 2) acc[2] = ee_mul(S[0], T[1]);
        return;
    case 0x2F: return;                                                                              /* VNOP */
    case 0x30: for (int k = 0; k < 4; k++) if (dest & (8 >> k)) T[k] = S[k]; return;                /* VMOVE */
    case 0x31: {                                                                                    /* VMR32 */
        uint32_t t4[4] = { S[1], S[2], S[3], S[0] };
        for (int k = 0; k < 4; k++) if (dest & (8 >> k)) T[k] = t4[k];
        return;
    }
    case 0x34: {                                                                                    /* VLQI */
        uint32_t a = (c->vi[fs & 15] & 0xFFFF) * 16;
        uint32_t *p = (uint32_t *)mp(c, a, 16, 16);
        for (int k = 0; k < 4; k++) if (dest & (8 >> k)) T[k] = p[k];
        c->vi[fs & 15] = (c->vi[fs & 15] + 1) & 0xFFFF;
        return;
    }
    case 0x35: {                                                                                    /* VSQI */
        uint32_t a = (c->vi[ft & 15] & 0xFFFF) * 16;
        uint32_t *p = (uint32_t *)mp(c, a, 16, 16);
        for (int k = 0; k < 4; k++) if (dest & (8 >> k)) p[k] = S[k];
        c->vi[ft & 15] = (c->vi[ft & 15] + 1) & 0xFFFF;
        return;
    }
    case 0x38: {                                                                                    /* VDIV */
        uint32_t a = S[(ins >> 21) & 3], b = T[(ins >> 23) & 3];
        c->vq = ee_div(a, b);
        return;
    }
    case 0x39: c->vq = ee_sqrt(T[(ins >> 23) & 3]); return;                                         /* VSQRT */
    case 0x3A: c->vq = ee_rsqrt(S[(ins >> 21) & 3], T[(ins >> 23) & 3]); return;                    /* VRSQRT */
    case 0x3B: return;                                                                              /* VWAITQ */
    case 0x3C: if ((ft & 15) != 0) c->vi[ft & 15] = S[(ins >> 21) & 3] & 0xFFFF; return;            /* VMTIR */
    case 0x3D: { int32_t v = (int16_t)c->vi[fs & 15]; for (int k = 0; k < 4; k++) if (dest & (8 >> k)) T[k] = (uint32_t)v; return; } /* VMFIR */
    case 0x3E: {                                                                                    /* VILWR */
        uint32_t a = (c->vi[fs & 15] & 0xFFFF) * 16;
        uint32_t *p = (uint32_t *)mp(c, a, 16, 16);
        for (int k = 0; k < 4; k++) if (dest & (8 >> k)) { if (ft & 15) c->vi[ft & 15] = p[k] & 0xFFFF; break; }
        return;
    }
    case 0x3F: {                                                                                    /* VISWR */
        uint32_t a = (c->vi[fs & 15] & 0xFFFF) * 16;
        uint32_t *p = (uint32_t *)mp(c, a, 16, 16);
        for (int k = 0; k < 4; k++) if (dest & (8 >> k)) { p[k] = c->vi[ft & 15] & 0xFFFF; }
        return;
    }
    default: unsupported(c, ins);
    }
    switch (bci) {
    case 0: case 1: case 2: case 3: bval = T[bci]; src_kind = 1; break;
    case 4: bval = I; src_kind = 1; break;
    case 5: bval = c->vq; src_kind = 1; break;
    default: src_kind = 0;
    }
    for (int k = 0; k < 4; k++) {
        if (!(dest & (8 >> k))) continue;
        long double a = r5900_ee_in(S[k]), b = r5900_ee_in(src_kind ? bval : T[k]);
        long double ac = r5900_ee_in(acc[k]);
        switch (akind) {
        case 0: acc[k] = r5900_ee_out(a + b); break;
        case 1: acc[k] = r5900_ee_out(a - b); break;
        case 2: acc[k] = r5900_ee_out(ac + fprod((float)a, (float)b)); break;
        case 3: acc[k] = r5900_ee_out(ac - fprod((float)a, (float)b)); break;
        default: acc[k] = r5900_ee_out(a * b); break;
        }
    }
}

/* The port's VU0 helpers (include/p4pc/vu0.h) truncate the exact sum like the VU (the guard-bit adder of the FPU, include/p4pc/eefloat.h, matches fewer
 * golden calls for VU0 code: 2600 against 2605 of 3355), also in the host-float proof pass: VU ops always run with EE semantics. */
static void vu_op(R5900 *c, uint32_t ins) {
    const int ieee = r5900_ieee;
    r5900_ieee = 0;
    vu_op_inner(c, ins);
    r5900_ieee = ieee;
}

/* ---------------------------------------------------------------- main interpreter */

typedef struct {
    int kind;            /* 0 none, 1 branch/jump */
    int taken, likely, call;
    uint32_t target;
} Br;

static int exec(R5900 *c, uint32_t ins, uint32_t pc, Br *br);

static inline int32_t imm16(uint32_t ins) { return (int16_t)(ins & 0xFFFF); }

#define RS ((ins >> 21) & 31)
#define RT ((ins >> 16) & 31)
#define RD ((ins >> 11) & 31)
#define SA ((ins >> 6) & 31)
#define FN (ins & 63)
#define W(n, v) do { int n_ = (n); if (n_) { c->r[n_] = (uint64_t)(v); c->src[n_] = 0; } } while (0)
#define R(n) (c->r[n])
#define R32(n) ((uint32_t)c->r[n])
#define RSS(n) ((int64_t)c->r[n])

static void set_branch(Br *br, int taken, uint32_t target, int likely, int call) {
    br->kind = 1; br->taken = taken; br->target = target; br->likely = likely; br->call = call;
}

static uint32_t fcr_get(R5900 *c, int n) { return n == 31 ? c->fcr31 : n == 0 ? 0x2E00u : 0; }

static int exec(R5900 *c, uint32_t ins, uint32_t pc, Br *br) {
    br->kind = 0;
    const uint32_t op = ins >> 26;
    switch (op) {
    case 0: /* SPECIAL */
        switch (FN) {
        case 0x00: W(RD, SX32(R32(RT) << SA)); break;
        case 0x02: W(RD, SX32(R32(RT) >> SA)); break;
        case 0x03: W(RD, SX32((int32_t)R32(RT) >> SA)); break;
        case 0x04: W(RD, SX32(R32(RT) << (R32(RS) & 31))); break;
        case 0x06: W(RD, SX32(R32(RT) >> (R32(RS) & 31))); break;
        case 0x07: W(RD, SX32((int32_t)R32(RT) >> (R32(RS) & 31))); break;
        case 0x08: set_branch(br, 1, R32(RS), 0, 0); break;                               /* JR */
        case 0x09: { uint32_t t = R32(RS); W(RD, pc + 8); set_branch(br, 1, t, 0, 1); break; } /* JALR */
        case 0x0A: if (R(RT) == 0) W(RD, R(RS)); break;                                     /* MOVZ */
        case 0x0B: if (R(RT) != 0) W(RD, R(RS)); break;                                     /* MOVN */
        case 0x0C:
            if (c->kernel && R(3) == 0x3eu) { /* EE EndOfHeap */
                W(2, c->kernel->heap_end);
                break;
            }
            longjmp(c->jb, R5900_TRAP);
        case 0x0D: longjmp(c->jb, R5900_TRAP);
        case 0x0F: break;                                                                   /* SYNC */
        case 0x10: W(RD, c->hi); break;
        case 0x11: c->hi = R(RS); break;
        case 0x12: W(RD, c->lo); break;
        case 0x13: c->lo = R(RS); break;
        case 0x14: W(RD, R(RT) << (R32(RS) & 63)); break;                                   /* DSLLV */
        case 0x16: W(RD, R(RT) >> (R32(RS) & 63)); break;
        case 0x17: W(RD, (uint64_t)(RSS(RT) >> (R32(RS) & 63))); break;
        case 0x18: case 0x19: {                                                             /* MULT / MULTU (rd form too) */
            int64_t p = FN == 0x18 ? (int64_t)(int32_t)R32(RS) * (int32_t)R32(RT)
                                   : (int64_t)((uint64_t)R32(RS) * R32(RT));
            c->lo = SX32((uint32_t)p);
            c->hi = SX32((uint32_t)((uint64_t)p >> 32));
            W(RD, c->lo);
            break;
        }
        case 0x1A: {                                                                        /* DIV */
            int32_t x = (int32_t)R32(RS), y = (int32_t)R32(RT);
            if (y == 0) { c->lo = x < 0 ? 1 : (uint64_t)-1; c->hi = SX32(x); }
            else if (x == INT32_MIN && y == -1) { c->lo = SX32(INT32_MIN); c->hi = 0; }
            else { c->lo = SX32(x / y); c->hi = SX32(x % y); }
            break;
        }
        case 0x1B: {                                                                        /* DIVU */
            uint32_t x = R32(RS), y = R32(RT);
            if (y == 0) { c->lo = (uint64_t)-1; c->hi = SX32(x); }
            else { c->lo = SX32(x / y); c->hi = SX32(x % y); }
            break;
        }
        case 0x20: case 0x21: { uint32_t so = c->src[RS] ? c->src[RS] : c->src[RT]; W(RD, SX32(R32(RS) + R32(RT))); if (RD) c->src[RD] = so; break; }
        case 0x22: case 0x23: W(RD, SX32(R32(RS) - R32(RT))); break;
        case 0x24: W(RD, R(RS) & R(RT)); break;
        case 0x25: { uint32_t so = c->src[RS] ? c->src[RS] : c->src[RT]; W(RD, R(RS) | R(RT)); if (RD) c->src[RD] = so; break; }
        case 0x26: W(RD, R(RS) ^ R(RT)); break;
        case 0x27: W(RD, ~(R(RS) | R(RT))); break;
        case 0x28: W(RD, c->sa); break;                                                     /* MFSA */
        case 0x29: c->sa = R32(RS) & 0xF; break;                                          /* MTSA: hardware-visible byte selector */
        case 0x2A: W(RD, RSS(RS) < RSS(RT)); break;
        case 0x2B: W(RD, R(RS) < R(RT)); break;
        case 0x2C: case 0x2D: { uint32_t so = c->src[RS] ? c->src[RS] : c->src[RT]; W(RD, R(RS) + R(RT)); if (RD) c->src[RD] = so; break; }
        case 0x2E: case 0x2F: W(RD, R(RS) - R(RT)); break;
        case 0x38: W(RD, R(RT) << SA); break;
        case 0x3A: W(RD, R(RT) >> SA); break;
        case 0x3B: W(RD, (uint64_t)(RSS(RT) >> SA)); break;
        case 0x3C: W(RD, R(RT) << (SA + 32)); break;
        case 0x3E: W(RD, R(RT) >> (SA + 32)); break;
        case 0x3F: W(RD, (uint64_t)(RSS(RT) >> (SA + 32))); break;
        default: unsupported(c, ins);
        }
        break;
    case 1: { /* REGIMM */
        const int rt = RT;
        const uint32_t tgt = pc + 4 + ((uint32_t)imm16(ins) << 2);
        switch (rt) {
        case 0x00: set_branch(br, RSS(RS) < 0, tgt, 0, 0); break;
        case 0x01: set_branch(br, RSS(RS) >= 0, tgt, 0, 0); break;
        case 0x02: set_branch(br, RSS(RS) < 0, tgt, 1, 0); break;
        case 0x03: set_branch(br, RSS(RS) >= 0, tgt, 1, 0); break;
        case 0x10: { int t = RSS(RS) < 0; W(31, pc + 8); set_branch(br, t, tgt, 0, 1); break; }
        case 0x11: { int t = RSS(RS) >= 0; W(31, pc + 8); set_branch(br, t, tgt, 0, 1); break; }
        case 0x18: c->sa = ((R32(RS) & 0xF) ^ ((uint32_t)imm16(ins) & 0xF)); break;               /* MTSAB */
        case 0x19: c->sa = ((R32(RS) & 7) ^ ((uint32_t)imm16(ins) & 7)) << 1; break;               /* MTSAH */
        default: unsupported(c, ins);
        }
        break;
    }
    case 2: set_branch(br, 1, ((pc + 4) & 0xF0000000u) | ((ins & 0x3FFFFFFu) << 2), 0, 0); break;
    case 3: W(31, pc + 8); set_branch(br, 1, ((pc + 4) & 0xF0000000u) | ((ins & 0x3FFFFFFu) << 2), 0, 1); break;
    case 4: set_branch(br, R(RS) == R(RT), pc + 4 + ((uint32_t)imm16(ins) << 2), 0, 0); break;
    case 5: set_branch(br, R(RS) != R(RT), pc + 4 + ((uint32_t)imm16(ins) << 2), 0, 0); break;
    case 6: set_branch(br, RSS(RS) <= 0, pc + 4 + ((uint32_t)imm16(ins) << 2), 0, 0); break;
    case 7: set_branch(br, RSS(RS) > 0, pc + 4 + ((uint32_t)imm16(ins) << 2), 0, 0); break;
    case 8: case 9: { uint32_t so = c->src[RS]; W(RT, SX32(R32(RS) + (uint32_t)imm16(ins))); if (RT) c->src[RT] = so; break; }
    case 10: W(RT, RSS(RS) < (int64_t)imm16(ins)); break;
    case 11: W(RT, R(RS) < (uint64_t)(int64_t)imm16(ins)); break;
    case 12: W(RT, R(RS) & (ins & 0xFFFF)); break;
    case 13: W(RT, R(RS) | (ins & 0xFFFF)); break;
    case 14: W(RT, R(RS) ^ (ins & 0xFFFF)); break;
    case 15: W(RT, SX32((ins & 0xFFFF) << 16)); break;
    case 16: {
        if (!c->kernel) unsupported(c, ins);
        if (RS == 0 && RD == 12 && (ins & 0x7ffu) == 0) {
            W(RT, SX32(c->kernel->status)); /* MFC0 Status */
        } else if (ins == 0x42000038u) { /* EI */
            c->kernel->status |= 0x10000u;
        } else if (ins == 0x42000039u) { /* DI */
            c->kernel->status &= ~0x10000u;
        } else {
            unsupported(c, ins);
        }
        break;
    }
    case 17: { /* COP1 */
        const int fmt = RS, ft = RT, fs = RD, fd = SA;
        uint32_t *f = c->f;
        switch (fmt) {
        case 0: W(RT, SX32(f[fs])); break;
        case 2: W(RT, SX32(fcr_get(c, fs))); break;
        case 4: f[fs] = R32(RT); note_float(c, f[fs]); break;
        case 6: if (fs == 31) { c->fcr31 = R32(RT); c->fcond = (c->fcr31 >> 23) & 1; } break;
        case 8: {
            const uint32_t tgt = pc + 4 + ((uint32_t)imm16(ins) << 2);
            switch (ft) {
            case 0: set_branch(br, !c->fcond, tgt, 0, 0); break;
            case 1: set_branch(br, c->fcond, tgt, 0, 0); break;
            case 2: set_branch(br, !c->fcond, tgt, 1, 0); break;
            case 3: set_branch(br, c->fcond, tgt, 1, 0); break;
            default: unsupported(c, ins);
            }
            break;
        }
        case 16: {
            switch (FN) {
            case 0: f[fd] = ee_add(f[fs], f[ft]); break;
            case 1: f[fd] = ee_sub(f[fs], f[ft]); break;
            case 2: f[fd] = ee_mul(f[fs], f[ft]); break;
            case 3: f[fd] = ee_div(f[fs], f[ft]); break;
            case 4: f[fd] = ee_sqrt(f[ft]); break;
            case 5: f[fd] = f[fs] & 0x7FFFFFFFu; break;
            case 6: f[fd] = f[fs]; break;
            case 7: f[fd] = f[fs] ^ 0x80000000u; break;
            case 22: f[fd] = ee_rsqrt(f[fs], f[ft]); break;
            case 24: c->acc = ee_add(f[fs], f[ft]); break;
            case 25: c->acc = ee_sub(f[fs], f[ft]); break;
            case 26: c->acc = ee_mul(f[fs], f[ft]); break;
            case 28: c->acc = ee_add(c->acc, f2b((float)fprod(r5900_ee_in(f[fs]), r5900_ee_in(f[ft])))); f[fd] = c->acc; break;
            case 29: c->acc = ee_sub(c->acc, f2b((float)fprod(r5900_ee_in(f[fs]), r5900_ee_in(f[ft])))); f[fd] = c->acc; break;
            case 30: c->acc = ee_add(c->acc, f2b((float)fprod(r5900_ee_in(f[fs]), r5900_ee_in(f[ft])))); break;
            case 31: c->acc = ee_sub(c->acc, f2b((float)fprod(r5900_ee_in(f[fs]), r5900_ee_in(f[ft])))); break;
            case 36: f[fd] = cvt_w(r5900_ee_in(f[fs])); break;
            case 40: { float a = r5900_ee_in(f[fs]), b = r5900_ee_in(f[ft]); f[fd] = f2b(a > b ? a : b); break; }
            case 41: { float a = r5900_ee_in(f[fs]), b = r5900_ee_in(f[ft]); f[fd] = f2b(a < b ? a : b); break; }
            case 48: c->fcond = 0; break;
            case 50: c->fcond = r5900_ee_in(f[fs]) == r5900_ee_in(f[ft]); break;
            case 52: c->fcond = r5900_ee_in(f[fs]) < r5900_ee_in(f[ft]); break;
            case 54: c->fcond = r5900_ee_in(f[fs]) <= r5900_ee_in(f[ft]); break;
            default: unsupported(c, ins);
            }
            break;
        }
        case 20:
            if (FN == 32) f[fd] = r5900_ee_out((long double)(int32_t)f[fs]);
            else unsupported(c, ins);
            break;
        default: unsupported(c, ins);
        }
        break;
    }
    case 18: { /* COP2: VU0 macro mode */
        const int fmt = RS;
        if (fmt >= 16) { vu_op(c, ins); break; }
        const int rt = RT, id = RD;
        switch (fmt) {
        case 1: { /* QMFC2 */ Q q = { (uint64_t)c->vf[id][0] | ((uint64_t)c->vf[id][1] << 32), (uint64_t)c->vf[id][2] | ((uint64_t)c->vf[id][3] << 32) }; if (rt) { c->r[rt] = q.lo; c->rh[rt] = q.hi; } break; }
        case 2: /* CFC2 */ W(rt, SX32(id == 22 ? c->vq : id == 21 ? c->vi_ : id == 18 ? c->vcli : id < 16 ? c->vi[id] : 0)); break;
        case 5: { /* QMTC2 */ c->vf[id][0] = (uint32_t)c->r[rt]; c->vf[id][1] = (uint32_t)(c->r[rt] >> 32); c->vf[id][2] = (uint32_t)c->rh[rt]; c->vf[id][3] = (uint32_t)(c->rh[rt] >> 32); break; }
        case 6: /* CTC2 */
            if (id < 16) c->vi[id] = id ? (uint32_t)R(rt) & 0xFFFF : 0;
            else if (id == 21) c->vi_ = (uint32_t)R(rt);
            else if (id == 22) c->vq = (uint32_t)R(rt);
            else if (id == 18) c->vcli = (uint32_t)R(rt);
            break;
        default: unsupported(c, ins);
        }
        break;
    }
    case 20: set_branch(br, R(RS) == R(RT), pc + 4 + ((uint32_t)imm16(ins) << 2), 1, 0); break;
    case 21: set_branch(br, R(RS) != R(RT), pc + 4 + ((uint32_t)imm16(ins) << 2), 1, 0); break;
    case 22: set_branch(br, RSS(RS) <= 0, pc + 4 + ((uint32_t)imm16(ins) << 2), 1, 0); break;
    case 23: set_branch(br, RSS(RS) > 0, pc + 4 + ((uint32_t)imm16(ins) << 2), 1, 0); break;
    case 24: case 25: { uint32_t so = c->src[RS]; W(RT, R(RS) + (uint64_t)(int64_t)imm16(ins)); if (RT) c->src[RT] = so; break; }                    /* DADDI/DADDIU */
    case 26: { /* LDL */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~7u, n = a & 7;
        uint64_t v = R(RT);
        for (uint32_t i = 0; i <= n; i++) { int bi = 7 - n + i; v = (v & ~(0xFFull << (8 * bi))) | ((uint64_t)ld8(c, b + i) << (8 * bi)); }
        W(RT, v); break;
    }
    case 27: { /* LDR */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~7u, n = a & 7;
        uint64_t v = R(RT);
        for (uint32_t i = n; i < 8; i++) { int bi = i - n; v = (v & ~(0xFFull << (8 * bi))) | ((uint64_t)ld8(c, b + i) << (8 * bi)); }
        W(RT, v); break;
    }
    case 28: { /* MMI */
        switch (FN) {
        case 0x00: case 0x01: case 0x20: case 0x21: {                                   /* MADD, MADDU, MADD1, MADDU1 */
            int p1 = FN >= 0x20;
            int sgn = (FN & 1) == 0;
            int64_t prod = sgn ? (int64_t)(int32_t)R32(RS) * (int32_t)R32(RT) : (int64_t)((uint64_t)R32(RS) * R32(RT));
            uint64_t *hi = p1 ? &c->hi1 : &c->hi, *lo = p1 ? &c->lo1 : &c->lo;
            uint64_t acc = ((uint64_t)(uint32_t)*hi << 32) | (uint32_t)*lo;
            acc += (uint64_t)prod;
            *lo = SX32((uint32_t)acc); *hi = SX32((uint32_t)(acc >> 32));
            W(RD, *lo);
            break;
        }
        case 0x04: {                                                                        /* PLZCW: leading bits equal to the sign, minus one */
            uint64_t o = 0;
            for (int h = 0; h < 2; h++) {
                uint32_t v = (uint32_t)(R(RS) >> (32 * h));
                if (v >> 31) v = ~v;
                o |= (uint64_t)(v ? (uint32_t)__builtin_clz(v) - 1 : 31) << (32 * h);
            }
            W(RD, o);
            break;
        }
        case 0x08: mmi_op(c, ins, 0); break;
        case 0x28: mmi_op(c, ins, 1); break;
        case 0x09: mmi_op(c, ins, 2); break;
        case 0x29: mmi_op(c, ins, 3); break;
        case 0x30:                                                                          /* PMFHL.LW / .UW: the words of HI and LO (gcc's soft double multiply reads the 128-bit product this way) */
            if (SA > 1) unsupported(c, ins);
            if (RD) {
                const int up = SA == 1;

                c->r[RD] = (uint64_t)(uint32_t)(up ? c->lo >> 32 : c->lo) | ((uint64_t)(uint32_t)(up ? c->hi >> 32 : c->hi) << 32);
                c->rh[RD] = (uint64_t)(uint32_t)(up ? c->lo1 >> 32 : c->lo1) | ((uint64_t)(uint32_t)(up ? c->hi1 >> 32 : c->hi1) << 32);
                c->src[RD] = 0;
            }
            break;
        case 0x10: W(RD, c->hi1); break;
        case 0x11: c->hi1 = R(RS); break;
        case 0x12: W(RD, c->lo1); break;
        case 0x13: c->lo1 = R(RS); break;
        case 0x18: case 0x19: {                                                          /* MULT1 / MULTU1 */
            int64_t p = FN == 0x18 ? (int64_t)(int32_t)R32(RS) * (int32_t)R32(RT) : (int64_t)((uint64_t)R32(RS) * R32(RT));
            c->lo1 = SX32((uint32_t)p); c->hi1 = SX32((uint32_t)((uint64_t)p >> 32)); W(RD, c->lo1); break;
        }
        case 0x1A: { int32_t x = (int32_t)R32(RS), y = (int32_t)R32(RT);
                     if (y == 0) { c->lo1 = x < 0 ? 1 : (uint64_t)-1; c->hi1 = SX32(x); }
                     else if (x == INT32_MIN && y == -1) { c->lo1 = SX32(INT32_MIN); c->hi1 = 0; }
                     else { c->lo1 = SX32(x / y); c->hi1 = SX32(x % y); } break; }
        case 0x1B: { uint32_t x = R32(RS), y = R32(RT);
                     if (y == 0) { c->lo1 = (uint64_t)-1; c->hi1 = SX32(x); } else { c->lo1 = SX32(x / y); c->hi1 = SX32(x % y); } break; }
        case 0x34: { Q a = getq(c, RT); uint16_t h[8]; memcpy(h, &a, 16); int s = SA & 15; for (int i = 0; i < 8; i++) h[i] = (uint16_t)(h[i] << s); memcpy(&a, h, 16); putq(c, RD, a); break; } /* PSLLH */
        case 0x36: { Q a = getq(c, RT); uint16_t h[8]; memcpy(h, &a, 16); int s = SA & 15; for (int i = 0; i < 8; i++) h[i] = (uint16_t)(h[i] >> s); memcpy(&a, h, 16); putq(c, RD, a); break; } /* PSRLH */
        case 0x37: { Q a = getq(c, RT); int16_t h[8]; memcpy(h, &a, 16); int s = SA & 15; for (int i = 0; i < 8; i++) h[i] = (int16_t)(h[i] >> s); memcpy(&a, h, 16); putq(c, RD, a); break; } /* PSRAH */
        case 0x3C: { Q a = getq(c, RT); uint32_t w[4]; memcpy(w, &a, 16); int s = SA; for (int i = 0; i < 4; i++) w[i] <<= s; memcpy(&a, w, 16); putq(c, RD, a); break; } /* PSLLW */
        case 0x3E: { Q a = getq(c, RT); uint32_t w[4]; memcpy(w, &a, 16); int s = SA; for (int i = 0; i < 4; i++) w[i] >>= s; memcpy(&a, w, 16); putq(c, RD, a); break; } /* PSRLW */
        case 0x3F: { Q a = getq(c, RT); int32_t w[4]; memcpy(w, &a, 16); int s = SA; for (int i = 0; i < 4; i++) w[i] >>= s; memcpy(&a, w, 16); putq(c, RD, a); break; } /* PSRAW */
        default: unsupported(c, ins);
        }
        break;
    }
    case 30: { uint32_t a = R32(RS) + (uint32_t)imm16(ins); uint64_t *p = (uint64_t *)mp(c, a, 16, 16); if (RT) { c->r[RT] = p[0]; c->rh[RT] = p[1]; } break; } /* LQ */
    case 31: { uint32_t a = R32(RS) + (uint32_t)imm16(ins); uint64_t *p = (uint64_t *)mp(c, a, 16, 16); p[0] = c->r[RT]; p[1] = c->rh[RT]; break; }  /* SQ */
    case 32: W(RT, (int64_t)(int8_t)ld8(c, R32(RS) + (uint32_t)imm16(ins))); break;
    case 33: W(RT, (int64_t)(int16_t)ld16(c, R32(RS) + (uint32_t)imm16(ins))); break;
    case 34: { /* LWL */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~3u, n = a & 3;
        uint32_t v = R32(RT);
        for (uint32_t i = 0; i <= n; i++) { int bi = 3 - n + i; v = (v & ~(0xFFu << (8 * bi))) | ((uint32_t)ld8(c, b + i) << (8 * bi)); }
        W(RT, SX32(v)); break;
    }
    case 35: { uint32_t a = R32(RS) + (uint32_t)imm16(ins); W(RT, SX32(ld32(c, a))); if (RT) c->src[RT] = a; break; }
    case 36: W(RT, ld8(c, R32(RS) + (uint32_t)imm16(ins))); break;
    case 37: W(RT, ld16(c, R32(RS) + (uint32_t)imm16(ins))); break;
    case 38: { /* LWR */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~3u, n = a & 3;
        uint32_t v = R32(RT);
        for (uint32_t i = n; i < 4; i++) { int bi = i - n; v = (v & ~(0xFFu << (8 * bi))) | ((uint32_t)ld8(c, b + i) << (8 * bi)); }
        if (n == 0) W(RT, SX32(v)); else W(RT, (R(RT) & 0xFFFFFFFF00000000ull) | v);
        break;
    }
    case 39: { uint32_t a = R32(RS) + (uint32_t)imm16(ins); W(RT, ld32(c, a)); if (RT) c->src[RT] = a; break; }
    case 40: st8(c, R32(RS) + (uint32_t)imm16(ins), (uint8_t)R(RT)); break;
    case 41: st16(c, R32(RS) + (uint32_t)imm16(ins), (uint16_t)R(RT)); break;
    case 42: { /* SWL */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~3u, n = a & 3;
        uint32_t v = R32(RT);
        for (uint32_t i = 0; i <= n; i++) st8(c, b + i, (uint8_t)(v >> (8 * (3 - n + i))));
        break;
    }
    case 43: st32(c, R32(RS) + (uint32_t)imm16(ins), R32(RT)); break;
    case 44: { /* SDL */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~7u, n = a & 7;
        for (uint32_t i = 0; i <= n; i++) st8(c, b + i, (uint8_t)(R(RT) >> (8 * (7 - n + i))));
        break;
    }
    case 45: { /* SDR */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~7u, n = a & 7;
        for (uint32_t i = n; i < 8; i++) st8(c, b + i, (uint8_t)(R(RT) >> (8 * (i - n))));
        break;
    }
    case 46: { /* SWR */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins), b = a & ~3u, n = a & 3;
        for (uint32_t i = n; i < 4; i++) st8(c, b + i, (uint8_t)(R32(RT) >> (8 * (i - n))));
        break;
    }
    case 47: break;                                                                             /* CACHE */
    case 49: c->f[RT] = ld32(c, R32(RS) + (uint32_t)imm16(ins)); note_float(c, c->f[RT]); break;                         /* LWC1 */
    case 51: break;                                                                             /* PREF */
    case 54: { /* LQC2 */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins); uint32_t *p = (uint32_t *)mp(c, a, 16, 16);
        if (RT) memcpy(c->vf[RT], p, 16);
        for (int k = 0; k < 4; k++) note_float(c, p[k]);
        break;
    }
    case 55: { uint32_t a = R32(RS) + (uint32_t)imm16(ins); W(RT, ld64(c, a)); if (RT) c->src[RT] = a; break; }
    case 57: st32(c, R32(RS) + (uint32_t)imm16(ins), c->f[RT]); break;                          /* SWC1 */
    case 62: { /* SQC2 */
        uint32_t a = R32(RS) + (uint32_t)imm16(ins); uint32_t *p = (uint32_t *)mp(c, a, 16, 16);
        memcpy(p, c->vf[RT], 16);
        break;
    }
    case 63: st64(c, R32(RS) + (uint32_t)imm16(ins), R(RT)); break;
    default: unsupported(c, ins);
    }
    c->r[0] = 0;
    c->vf[0][0] = c->vf[0][1] = c->vf[0][2] = 0;
    c->vf[0][3] = 0x3F800000u;
    return br->kind;
}

static uint32_t fetch(R5900 *c, uint32_t pc) {
    if ((pc & 3) || pc < RAM_LO || pc >= RAM_HI) fault(c, "instruction fetch outside RAM", pc);
    return *(uint32_t *)P4_G2H(pc);
}

int r5900_call(R5900 *c, uint32_t entry) {
    int rc = setjmp(c->jb);
    if (rc) return rc;
    c->r[31] = R5900_SENTINEL;
    c->r[0] = 0;
    uint32_t pc = entry;
    for (;;) {
        if (pc == R5900_SENTINEL) return R5900_OK;
        if (++c->steps > c->max_steps) return R5900_STEPS;
        c->pc_now = pc;
        Br br;
        uint32_t ins = fetch(c, pc);
        if (c->trace) fprintf(stderr, "  %08x %08x v0=%08x:%08x:%08x:%08x a0=%08x v1=%08x f12=%08x f0=%08x vf10=%08x,%08x,%08x,%08x\n", pc, ins, (uint32_t)(c->rh[2] >> 32), (uint32_t)c->rh[2], (uint32_t)(c->r[2] >> 32), (uint32_t)c->r[2], (uint32_t)c->r[4], (uint32_t)c->r[3], c->f[12], c->f[0], c->vf[10][0], c->vf[10][1], c->vf[10][2], c->vf[10][3]);
        if (!exec(c, ins, pc, &br)) { pc += 4; continue; }
        if (br.likely && !br.taken) { pc += 8; continue; }
        c->pc_now = pc + 4;
        Br db;
        if (exec(c, fetch(c, pc + 4), pc + 4, &db)) fault(c, "branch in a delay slot", pc + 4);
        c->steps++;
        if (!br.taken) { pc += 8; continue; }
        const uint32_t t = br.target;
        c->indirect = (ins >> 26) == 0 && ((ins & 63) == 9 || ((ins & 63) == 8 && ((ins >> 21) & 31) != 31));
        const int inside = t >= c->lo_pc && t < c->hi_pc;
        if (t == R5900_SENTINEL) { pc = t; continue; }
        if (br.call) {
            if (inside) { pc = t; continue; }
            if (!c->hook || !c->hook(c, t, c->user)) { c->fault_src = c->src[(ins >> 21) & 31]; return R5900_CALL; }
            pc += 8;
            continue;
        }
        if (inside) { pc = t; continue; }
        /* a jump or branch out of the function: a tail call. The callee returns to our caller. */
        if (!c->hook || !c->hook(c, t, c->user)) return R5900_CALL;
        pc = (uint32_t)c->r[31];
    }
}

const char *r5900_name(uint32_t w) {
    static char buf[48];
    uint32_t op = w >> 26;
    if (op == 0) snprintf(buf, sizeof buf, "SPECIAL fn %02x", w & 63);
    else if (op == 1) snprintf(buf, sizeof buf, "REGIMM rt %02x", (w >> 16) & 31);
    else if (op == 17) snprintf(buf, sizeof buf, "COP1 fmt %02x fn %02x", (w >> 21) & 31, w & 63);
    else if (op == 18) snprintf(buf, sizeof buf, ((w >> 21) & 31) >= 16 ? "COP2 vu fn %02x/%02x" : "COP2 fmt %02x fn %02x", ((w >> 21) & 31) >= 16 ? (w & 63) : ((w >> 21) & 31), ((w >> 6) & 31));
    else if (op == 28) snprintf(buf, sizeof buf, "MMI fn %02x sa %02x", w & 63, (w >> 6) & 31);
    else snprintf(buf, sizeof buf, "op %02x", op);
    return buf;
}
