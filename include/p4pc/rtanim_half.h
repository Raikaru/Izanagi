/* RtAnim compressed keyframes: the 16-bit float the retail RtAnim code stores quaternion and
 * translation components in (1 sign bit, 4 exponent bits, 11 mantissa bits; retail 0x00415960 encodes,
 * the inline decode in 0x00415bc0 and 0x00416e10 reads). Exponent 0 is zero, otherwise the value is
 * (-1)^s * 2^(e - 16) * (1 + m / 2048), so the largest magnitude is just under 1. */
#ifndef P4PC_RTANIM_HALF_H
#define P4PC_RTANIM_HALF_H

#include <math.h>

static inline unsigned int p4_anim_bits(float f)
{
    unsigned int u;

    __builtin_memcpy(&u, &f, sizeof u);
    return u;
}

static inline float p4_anim_float(unsigned int u)
{
    float f;

    __builtin_memcpy(&f, &u, sizeof f);
    return f;
}

/* The decode in retail 0x00415bc0: sign, then (when the magnitude bits are not all zero) exponent and
 * mantissa moved to the float fields with the bias 0x38000000. */
static inline float p4_anim_half_to_float(unsigned short h)
{
    unsigned int bits = (unsigned int)(h & 0x8000) << 16;

    if ((h & 0x7fff) != 0) {
        bits |= ((unsigned int)(h & 0x7800) * 0x1000u + 0x38000000u) | ((unsigned int)(h & 0x7ff) << 12);
    }
    return p4_anim_float(bits);
}

/* Retail 0x00415960: round to nearest at the 12th mantissa bit, saturate magnitudes of 2 and above to
 * the largest value, flush magnitudes below 2^-15 to zero, then pack. */
static inline unsigned short p4_anim_float_to_half(float x)
{
    unsigned int u = p4_anim_bits(x);
    unsigned int exp;
    float f = x;
    unsigned int v;
    unsigned int h;

    exp = (u & 0x7f800000u) >> 23;
    if ((u & 0xfffu) > 0x7ffu && exp > 0xcu && (exp < 0xfeu || (u & 0x7ff000u) != 0x7ff000u)) {
        f = x + p4_anim_float((u & 0x80000000u) | ((exp - 0xcu) * 0x800000u));
    }
    v = p4_anim_bits(f) & 0xfffff000u;
    if (0x3f800000u < (p4_anim_bits(f) & 0x7f800000u)) {
        v = (p4_anim_bits(f) & 0x80000000u) | 0x7ffff000u;
    }
    if ((v & 0x7f800000u) < 0x38000000u) {
        v &= 0x80000000u;
    }
    h = (unsigned int)((int)v >> 16) & 0x8000u;
    if ((v & 0x7fffffffu) != 0) {
        h |= ((((v >> 23) & 0xffu) - 0x70u) * 0x800u) & 0xffffu;
        h |= (v >> 12) & 0x7ffu;
    }
    return (unsigned short)h;
}

/* The retail acos (inlined in 0x00415bc0 at 0x416088..0x416580 and in 0x004165a0): a rational approximation
 * R(z) = z * P(z) / Q(z) of asin, evaluated by the EE's adda/madd chains (acc + a * b: with -ffp-contract=off the
 * same products and sums in the same order). |x| < 0.5 uses z = x * x, larger magnitudes z = (1 -+ x) / 2 and a
 * square root whose high 20 bits are split off for the correction term. */
static inline float p4_anim_asin_ratio(float z)
{
    float f19 = p4_anim_float(0x3a4f7f04u) + p4_anim_float(0x3811ef08u) * z;
    float f17 = p4_anim_float(0xbd241146u) + z * f19;
    float f16 = p4_anim_float(0x3e4e0aa8u) + z * f17;
    float f15 = p4_anim_float(0xbea6b090u) + z * f16;
    float f14 = p4_anim_float(0x3e2aaaabu) + z * f15;
    float q12 = p4_anim_float(0xbf303361u) + p4_anim_float(0x3d9dc62eu) * z;
    float q11 = p4_anim_float(0x4001572du) + z * q12;
    float q3 = p4_anim_float(0xc019d139u) + z * q11;
    float q2 = 1.0f + z * q3;

    return (z * f14) / q2;
}

static inline float p4_anim_acos(float x)
{
    unsigned int xb = p4_anim_bits(x);
    unsigned int ab = xb & 0x7fffffffu;
    const float c = p4_anim_float(0x33a22168u);

    if (ab >= 0x3f800000u) {
        return (int)xb <= 0 ? p4_anim_float(0x40490fdbu) : 0.0f;
    }
    if (ab >= 0x3f000000u) {
        float z, s, shi, corr, r;

        if ((int)xb < 0) {
            z = 0.5f * (1.0f + x);
            r = p4_anim_asin_ratio(z);
            s = sqrtf(z);
            return p4_anim_float(0x40490fdau) - 2.0f * (s + (r * s - c));
        }
        z = 0.5f * (1.0f - x);
        r = p4_anim_asin_ratio(z);
        s = sqrtf(z);
        shi = p4_anim_float(p4_anim_bits(s) & 0xfffff000u);
        corr = (z - shi * shi) / (s + shi);
        return 2.0f * (shi + (corr + r * s));
    }
    if (ab < 0x23000001u) {
        return p4_anim_float(0x3fc90fdbu);
    }
    {
        float r = p4_anim_asin_ratio(x * x);
        float t = c - x * r;

        return p4_anim_float(0x3fc90fdau) - (x - t);
    }
}

/* The retail sin the slerp inlines: w + w^3 * P(w^2) (odd polynomial, no range reduction). */
static inline float p4_anim_sin(float w)
{
    float z = w * w;
    float p = p4_anim_float(0xb2d72f34u) + p4_anim_float(0x2f2ec9d3u) * z;

    p = p4_anim_float(0x3638ef1bu) + z * p;
    p = p4_anim_float(0xb9500d01u) + z * p;
    p = p4_anim_float(0x3c088889u) + z * p;
    p = p4_anim_float(0xbe2aaaabu) + z * p;
    return w + (z * w) * p;
}

/* The slerp the retail blend and interpolate callbacks inline (their own acos and sin polynomials above). q1 and q2
 * are 4 floats (x, y, z, w); `t` is the blend factor. A dot below 0x3f7fbe77 (just under 0.99924) slerps, anything
 * larger lerps. q2 and dot are the caller's: the caller has already negated q2 when dot was negative. */
static inline void p4_anim_slerp(float *out, const float *q1, const float *q2, float dot, float t)
{
    float s0 = 1.0f - t;
    float s1 = t;

    if (dot < p4_anim_float(0x3f7fbe77u)) {
        float omega = p4_anim_acos(dot);
        float inv = 1.0f / p4_anim_sin(omega);

        s0 = p4_anim_sin(s0 * omega) * inv;
        s1 = p4_anim_sin(s1 * omega) * inv;
    }
    out[0] = s1 * q2[0] + s0 * q1[0];
    out[1] = s1 * q2[1] + s0 * q1[1];
    out[2] = s1 * q2[2] + s0 * q1[2];
    out[3] = s1 * q2[3] + s0 * q1[3];
}

#endif /* P4PC_RTANIM_HALF_H */
