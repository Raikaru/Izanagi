/* RtAnim keyframe callbacks (overrides 00415ad0, 004165a0, 00415bc0, 00417310, 00416e10) and the
 * 16-bit float of p4pc/rtanim_half.h. Expected values are worked by hand from the retail semantics:
 * identity and exactly representable quaternions, so the checks do not depend on libm rounding. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "p4pc/rtanim_half.h"
#include "guest_test.h"

/* Interpolator records and callback data live on the guest stack. Their pointer
 * words stay four bytes; native callbacks carry their actual register ABI. */
#define GA(p) ((unsigned int)P4_H2G(p))
#define GF(f, sig) ((unsigned int)p4_fn_h2g_sig((f), (sig)))

void func_00415ad0(unsigned char *matrix, unsigned char *frame);
void func_004165a0(unsigned char *out, unsigned char *in1, unsigned char *in2, float alpha);
void func_00415bc0(unsigned char *out, unsigned char *in1, unsigned char *in2, float time, float *custom);
void func_00417310(unsigned char *out, unsigned char *in1, unsigned char *in2);
void func_00416e10(unsigned char *frame, unsigned char *start);
void func_003dcc70(float *q1, float *q2, float *out);
int func_003d59a0(unsigned char *dst, unsigned char *src);
int func_003d5e90(unsigned char *a, unsigned char *b, unsigned char *c, float t);
int func_003d5840(unsigned char *interp, unsigned char *anim);
int func_003d59d0(unsigned char *interp, float dt);
int func_003d5bc0(unsigned char *interp, float dt);

static int checks, failures;
static int blend_calls;
static unsigned char *blend_last_a, *blend_last_b, *blend_last_c;
static float blend_t;

static void blend_cb(unsigned char *a, unsigned char *b, unsigned char *c, float t)
{
    blend_calls++;
    blend_last_a = a;
    blend_last_b = b;
    blend_last_c = c;
    blend_t = t;
}

#define CHECK(cond)                                                      \
    do {                                                                 \
        checks++;                                                        \
        if (!(cond)) {                                                   \
            failures++;                                                  \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
        }                                                                \
    } while (0)

static int near_f(float a, float b)
{
    return fabsf(a - b) < 1e-5f;
}

/* Uncompressed keyframe: 0x24 bytes, time at +4, quaternion +8..+0x14, translation +0x18..+0x20. */
static void set_frame(unsigned char *f, float time, float x, float y, float z, float w, float tx, float ty, float tz)
{
    float *p = (float *)f;

    memset(f, 0, 0x24);
    p[1] = time;
    p[2] = x;
    p[3] = y;
    p[4] = z;
    p[5] = w;
    p[6] = tx;
    p[7] = ty;
    p[8] = tz;
}

static float q(unsigned char *f, int i)
{
    return ((float *)f)[2 + i];
}

/* Compressed keyframe: time float at +4, quaternion halves +8..+0xf, translation halves +0x10..+0x15. */
static void set_cframe(unsigned char *f, float time, float x, float y, float z, float w, float tx, float ty, float tz)
{
    memset(f, 0, 0x18);
    *(float *)(f + 4) = time;
    *(unsigned short *)(f + 8) = p4_anim_float_to_half(x);
    *(unsigned short *)(f + 10) = p4_anim_float_to_half(y);
    *(unsigned short *)(f + 12) = p4_anim_float_to_half(z);
    *(unsigned short *)(f + 14) = p4_anim_float_to_half(w);
    *(unsigned short *)(f + 16) = p4_anim_float_to_half(tx);
    *(unsigned short *)(f + 18) = p4_anim_float_to_half(ty);
    *(unsigned short *)(f + 20) = p4_anim_float_to_half(tz);
}


/* Interpolator (overrides 003d5840 003d59d0 003d5bc0). One node over four keyframes {prev, time, value}
 * at times 0, 1, 2, 3 with values 0, 10, 20, 30; the interpolate callback lerps the value (+8). The
 * animation lasts 3. Interpolator words: [1] time, [2] next frame, [3] callback, [4] its data,
 * [5] callback time, [6] loop callback, [7] its data, [9] interpolated keyframe size, [10] keyframe size,
 * [0xb] node count; the node's interpolated keyframe starts at +0x4c with its value at +0x54. */

static void lerp_cb(unsigned char *out, unsigned char *in1, unsigned char *in2, float time, void *custom)
{
    float t1 = *(float *)(in1 + 4), t2 = *(float *)(in2 + 4);
    float a = *(float *)(in1 + 8), b = *(float *)(in2 + 8);

    (void)custom;
    *(float *)(out + 8) = t2 == t1 ? a : a + (b - a) * (time - t1) / (t2 - t1);
}

static void *count_cb(unsigned char *interp, void *data)
{
    (void)interp;
    (*(int *)data)++;
    return data;
}

static float node_value(unsigned char *interp)
{
    return *(float *)(interp + 0x54);
}

static void test_interpolator(void)
{
    _Alignas(16) unsigned char interp[0x4c + 12 + 16];
    unsigned int frames[4][4] = {{0}};
    unsigned int anim[6] = {0}, info[7] = {0};
    unsigned int *w = (unsigned int *)interp;
    float *f = (float *)interp;
    int i;
    int cb_calls = 0, loop_calls = 0;

    for (i = 0; i < 4; i++) {
        frames[i][0] = GA(i == 0 ? &frames[0] : &frames[i - 1]);
        ((float *)frames[i])[1] = (float)i;
        ((float *)frames[i])[2] = 10.0f * (float)i;
    }
    info[1] = 12;
    info[2] = 16;
    info[5] = GF(lerp_cb, "vpppfp");
    anim[0] = GA(info);
    anim[1] = 4;
    ((float *)anim)[3] = 3.0f;
    anim[4] = GA(frames);
    memset(interp, 0, sizeof interp);
    w[0xb] = 1;

    CHECK(func_003d5840(interp, (unsigned char *)anim) == 1);
    CHECK(w[0] == GA(anim) && f[1] == 0.0f);
    CHECK(w[9] == 12 && w[10] == 16);
    CHECK(near_f(node_value(interp), 0.0f));
    CHECK(w[2] == GA(&frames[2]));

    CHECK(func_003d5bc0(interp, 1.5f) == 1);
    CHECK(near_f(f[1], 1.5f) && near_f(node_value(interp), 15.0f));
    CHECK(w[2] == GA(&frames[3]));
    CHECK(func_003d5bc0(interp, 1.0f) == 1);
    CHECK(near_f(node_value(interp), 25.0f));
    CHECK(func_003d5bc0(interp, -1.0f) == 1 && near_f(f[1], 2.5f));   /* only a positive step does anything */
    CHECK(func_003d59d0(interp, 2.0f) == 1);
    CHECK(near_f(f[1], 0.5f) && near_f(node_value(interp), 5.0f));
    CHECK(w[2] == 0);                                                 /* stepping back unsets next */

    /* The callback fires once when the time crosses its callback time going forward. */
    w[3] = GF(count_cb, "ppp");
    w[4] = GA(&cb_calls);
    f[5] = 1.2f;
    func_003d5bc0(interp, 1.0f);
    CHECK(cb_calls == 1 && near_f(node_value(interp), 15.0f));
    func_003d5bc0(interp, 0.5f);
    CHECK(cb_calls == 1);

    /* Past the duration the time wraps and the loop callback fires. */
    w[6] = GF(count_cb, "ppp");
    w[7] = GA(&loop_calls);
    func_003d5bc0(interp, 2.5f);                                      /* 2.0 + 2.5 = 4.5 -> 1.5 */
    CHECK(loop_calls == 1 && near_f(f[1], 1.5f) && near_f(node_value(interp), 15.0f));
    /* Stepping back past zero wraps the other way. */
    func_003d59d0(interp, 2.0f);                                      /* 1.5 - 2.0 = -0.5 -> 2.5 */
    CHECK(loop_calls == 2 && near_f(f[1], 2.5f) && near_f(node_value(interp), 25.0f));
}

static int run_test(int argc, char **argv)
{
    _Alignas(16) unsigned char a[0x24], b[0x24], out[0x24];
    _Alignas(16) float m[16];
    unsigned int type;
    float custom[6] = {10.0f, 20.0f, 30.0f, 2.0f, 2.0f, 2.0f};
    _Alignas(16) unsigned char ca[0x18], cb[0x18];

    (void)argc;
    (void)argv;

    /* 16-bit float: exact values round-trip, 1.0 is the largest exponent, tiny values flush to zero,
     * large values saturate, signs are kept. */
    CHECK(p4_anim_float_to_half(1.0f) == 0x7800);
    CHECK(p4_anim_half_to_float(0x7800) == 1.0f);
    CHECK(p4_anim_float_to_half(0.0f) == 0);
    CHECK(p4_anim_half_to_float(0) == 0.0f);
    CHECK(p4_anim_float_to_half(-0.5f) == (0x8000 | p4_anim_float_to_half(0.5f)));
    CHECK(p4_anim_half_to_float(p4_anim_float_to_half(0.5f)) == 0.5f);
    CHECK(p4_anim_half_to_float(p4_anim_float_to_half(-0.25f)) == -0.25f);
    CHECK(p4_anim_half_to_float(p4_anim_float_to_half(0.75f)) == 0.75f);
    CHECK(p4_anim_float_to_half(1e-6f) == 0);
    CHECK(p4_anim_half_to_float(p4_anim_float_to_half(5.0f)) > 0.99f);
    CHECK(near_f(p4_anim_half_to_float(p4_anim_float_to_half(0.3f)), 0.3f) ||
          fabsf(p4_anim_half_to_float(p4_anim_float_to_half(0.3f)) - 0.3f) < 3e-4f);

    /* Apply: the identity quaternion gives the identity rotation, the position row, and type 3. */
    set_frame(a, 0, 0, 0, 0, 1, 1, 2, 3);
    memset(m, 0xcc, sizeof m);
    func_00415ad0((unsigned char *)m, a);
    CHECK(m[0] == 1 && m[5] == 1 && m[10] == 1);
    CHECK(m[1] == 0 && m[2] == 0 && m[4] == 0 && m[6] == 0 && m[8] == 0 && m[9] == 0);
    CHECK(m[12] == 1 && m[13] == 2 && m[14] == 3);
    memcpy(&type, &m[3], sizeof type);
    CHECK(type == 3);
    /* A 90 degree turn about z, q = (0, 0, s, s) with s = sqrt(1/2): x axis maps to y. */
    set_frame(a, 0, 0, 0, 0.70710678f, 0.70710678f, 0, 0, 0);
    func_00415ad0((unsigned char *)m, a);
    CHECK(near_f(m[0], 0) && near_f(m[1], 1) && near_f(m[4], -1) && near_f(m[5], 0) && near_f(m[10], 1));

    /* Blend: alpha 0 is the first keyframe, alpha 1 the second, translations lerp. */
    set_frame(a, 0, 0, 0, 0, 1, 0, 0, 0);
    set_frame(b, 1, 0, 0, 0.70710678f, 0.70710678f, 4, 8, 12);
    func_004165a0(out, a, b, 0.0f);
    CHECK(near_f(q(out, 2), 0) && near_f(q(out, 3), 1));
    func_004165a0(out, a, b, 1.0f);
    CHECK(near_f(q(out, 2), 0.70710678f) && near_f(q(out, 3), 0.70710678f));
    func_004165a0(out, a, b, 0.5f);
    CHECK(near_f(*(float *)(out + 0x18), 2) && near_f(*(float *)(out + 0x1c), 4) && near_f(*(float *)(out + 0x20), 6));
    CHECK(near_f(q(out, 2), 0.38268343f) && near_f(q(out, 3), 0.92387953f));   /* 45 degrees: sin/cos(22.5) */
    /* A negative dot negates the second quaternion in place and blends towards -q2. */
    set_frame(b, 1, 0, 0, -0.70710678f, -0.70710678f, 0, 0, 0);
    func_004165a0(out, a, b, 1.0f);
    CHECK(near_f(q(out, 2), 0.70710678f) && near_f(q(out, 3), 0.70710678f));
    CHECK(near_f(q(b, 2), 0.70710678f));

    /* Add: identity times q is q; quaternions multiply, translations add. */
    set_frame(a, 0, 0, 0, 0, 1, 1, 1, 1);
    set_frame(b, 0, 0, 0, 0.70710678f, 0.70710678f, 2, 3, 4);
    func_00417310(out, a, b);
    CHECK(near_f(q(out, 2), 0.70710678f) && near_f(q(out, 3), 0.70710678f) && near_f(q(out, 0), 0));
    CHECK(near_f(*(float *)(out + 0x18), 3) && near_f(*(float *)(out + 0x1c), 4) && near_f(*(float *)(out + 0x20), 5));

    /* Interpolate (compressed): halfway in time between identity and a half-size z turn, with the
     * translation scaled by 2 and offset by (10, 20, 30). Half-float quantisation sets the tolerance. */
    set_cframe(ca, 0.0f, 0, 0, 0, 1, 0, 0, 0);
    set_cframe(cb, 2.0f, 0, 0, 0.5f, 0.5f, 0.5f, 0.25f, 0.125f);
    func_00415bc0(out, ca, cb, 1.0f, custom);
    CHECK(fabsf(*(float *)(out + 0x18) - (0.25f * 2.0f + 10.0f)) < 1e-4f);
    CHECK(fabsf(*(float *)(out + 0x1c) - (0.125f * 2.0f + 20.0f)) < 1e-4f);
    CHECK(fabsf(*(float *)(out + 0x20) - (0.0625f * 2.0f + 30.0f)) < 1e-4f);
    CHECK(near_f(q(out, 0), 0) && near_f(q(out, 1), 0));
    CHECK(near_f(q(out, 2), 0.28867513f) && fabsf(q(out, 3) - 0.8660254f) < 1e-3f);
    func_00415bc0(out, ca, cb, 0.0f, custom);
    CHECK(near_f(q(out, 3), 1.0f) && near_f(q(out, 2), 0));

    /* MulRecip (compressed): a frame times the reciprocal of itself is the identity rotation, with a
     * zero translation. */
    set_cframe(ca, 0, 0, 0, 0.5f, 0.5f, 0.5f, 0.25f, 0.125f);
    set_cframe(cb, 0, 0, 0, 0.5f, 0.5f, 0.5f, 0.25f, 0.125f);
    func_00416e10(ca, cb);
    CHECK(near_f(p4_anim_half_to_float(*(unsigned short *)(ca + 8)), 0));
    CHECK(near_f(p4_anim_half_to_float(*(unsigned short *)(ca + 10)), 0));
    CHECK(near_f(p4_anim_half_to_float(*(unsigned short *)(ca + 12)), 0));
    CHECK(near_f(p4_anim_half_to_float(*(unsigned short *)(ca + 14)), 1.0f));
    CHECK(*(unsigned short *)(ca + 16) == 0 && *(unsigned short *)(ca + 18) == 0 && *(unsigned short *)(ca + 20) == 0);

    test_interpolator();

    /* Record copy (003d59a0) and node blend (003d5e90): three nodes of 8 bytes, a callback at +64. */
    {
        _Alignas(16) unsigned char ra[76 + 24] = {0}, rb[76 + 24] = {0}, rc[76 + 24] = {0};
        int i;

        for (i = 0; i < 3; i++) {
            *(int *)(ra + 36) = *(int *)(rb + 36) = *(int *)(rc + 36) = 8;
            *(int *)(ra + 44) = *(int *)(rb + 44) = *(int *)(rc + 44) = 3;
        }
        *(unsigned int *)(ra + 64) = GF(blend_cb, "vpppf");
        for (i = 0; i < 24; i++) {
            rb[76 + i] = (unsigned char)(i + 1);
        }
        CHECK(func_003d59a0(ra, rb) == 1 && memcmp(ra + 76, rb + 76, 24) == 0);
        blend_calls = 0;
        CHECK(func_003d5e90(ra, rb, rc, 0.25f) == 1 && blend_calls == 3);
        CHECK(blend_last_a == ra + 76 + 16 && blend_last_b == rb + 76 + 16 && blend_last_c == rc + 76 + 16);
        CHECK(blend_t == 0.25f);
        *(int *)(ra + 44) = 0;
        blend_calls = 0;
        CHECK(func_003d5e90(ra, rb, rc, 0.5f) == 1 && blend_calls == 0);
    }

    /* Slerp setup (003dcc70): identity to a 90 degree z turn. dot = sqrt(1/2), angle = 45 degrees, not
     * parallel, so the eight quaternion words are scaled by 1 / sin(45 degrees); the turn's second
     * quaternion is negated when the dot is negative. */
    {
        float q1[4] = {0, 0, 0, 1}, q2[4] = {0, 0, 0.70710678f, 0.70710678f}, out[10];
        float neg[4] = {0, 0, -0.70710678f, -0.70710678f};

        func_003dcc70(q1, q2, out);
        CHECK(near_f(out[8], 0.78539816f) && out[9] == 0.0f);
        CHECK(near_f(out[3], 1.41421356f) && near_f(out[6], 1.0f) && near_f(out[7], 1.0f));
        func_003dcc70(q1, neg, out);
        CHECK(near_f(out[8], 0.78539816f) && near_f(out[6], 1.0f) && near_f(out[7], 1.0f));
        func_003dcc70(q1, q1, out);                       /* identical: parallel, nothing scaled */
        CHECK(((int *)out)[9] == 1 && near_f(out[3], 1.0f) && near_f(out[7], 1.0f) && near_f(out[8], 0.0f));
    }

    printf("rtanim_test: %d checks, %d failures\n", checks, failures);
    return failures != 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(run_test, argc, argv);
}
