/* The alpha-test pass culling decision (src/render/p4gl_passcull.h) against the fragment shader it stands for. For a grid of alpha functions, references,
 * vertex/material/texture alpha ranges, every fragment alpha the ranges allow (the range's ends and interior points, with the shader's own float round trip of
 * the vertex colour through clip W) must compare the same way the decision claims: a dropped second pass means every fragment passes, a dropped first pass
 * means every fragment fails. A decision that is too eager would change the picture; this fails first. */
#include <math.h>
#include <stdio.h>

#include "p4gl_passcull.h"

/* The shader's p4GsAlpha and alphaPasses (p4gl_gl.c kGlslGsAlpha, p4gl_atomic.c kAtomicFragmentSource). */
static float gs_alpha(float a)
{
    const float t = a * 128.0f, n = floorf(t + 0.5f);

    return fabsf(t - n) < 1.0f / 256.0f ? n : t;
}

static int passes(int func, float ref, float a)
{
    const float t = gs_alpha(a);

    switch (func) {
    case 8: return 0;
    case 2: return t < ref;
    case 3: return t <= ref;
    case 4: return t == ref;
    case 5: return t >= ref;
    case 6: return t > ref;
    case 7: return t != ref;
    default: return 1;
    }
}

static int bad;
static unsigned decided;

static void check(int func, float ref, unsigned vlo, unsigned vhi, float mat, unsigned tlo, unsigned thi)
{
    static const float kW[] = {0.37f, 1.0f, 3.1f, 7.3f, 120.0f};
    const unsigned skip = p4gl_alpha_pass_cull(func, ref, vlo, vhi, mat, tlo, thi);
    unsigned a, b, w;

    decided += skip != 0;
    for (a = 0; a <= 8; a++) {
        const unsigned v = vlo + (vhi - vlo) * a / 8u;

        for (b = 0; b <= 8; b++) {
            const unsigned t = tlo + (thi - tlo) * b / 8u;

            for (w = 0; w < sizeof kW / sizeof kW[0]; w++) {
                /* vCol = aCol * W, then (vCol / vFogZ) * uMatColor * texture, in float, as the shader does */
                const float vcol = ((float)v / 255.0f) * kW[w];
                const float frag = (vcol / kW[w]) * mat * ((float)t / 255.0f);
                const int p = passes(func, ref, frag);

                if (((skip & 2u) && !p) || ((skip & 1u) && p)) {
                    printf("alpha_pass_cull_test: func %d ref %g v %u..%u mat %g t %u..%u: skip %u but the fragment (v %u t %u W %g) %s\n", func, (double)ref, vlo, vhi,
                           (double)mat, tlo, thi, skip, v, t, (double)kW[w], p ? "passes" : "fails");
                    bad = 1;
                    return;
                }
            }
        }
    }
}

int main(void)
{
    static const int kFuncs[] = {2, 3, 4, 5, 6, 7, 8};
    static const float kRefs[] = {0, 1, 31, 63, 64, 95, 126, 127, 128};
    static const unsigned kRanges[][2] = {{255, 255}, {128, 255}, {0, 255}, {0, 0}, {100, 200}, {64, 64}, {250, 255}, {1, 2}};
    static const float kMats[] = {1.0f, 0.5f, 0.25f, 0.0f, 128.0f / 255.0f};
    unsigned f, r, v, m, t;

    for (f = 0; f < sizeof kFuncs / sizeof kFuncs[0]; f++) {
        for (r = 0; r < sizeof kRefs / sizeof kRefs[0]; r++) {
            for (v = 0; v < sizeof kRanges / sizeof kRanges[0]; v++) {
                for (m = 0; m < sizeof kMats / sizeof kMats[0]; m++) {
                    for (t = 0; t < sizeof kRanges / sizeof kRanges[0]; t++) {
                        check(kFuncs[f], kRefs[r], kRanges[v][0], kRanges[v][1], kMats[m], kRanges[t][0], kRanges[t][1]);
                    }
                }
            }
        }
    }
    /* the cases the optimisation is for */
    if (p4gl_alpha_pass_cull(5, 127.0f, 255, 255, 1.0f, 255, 255) != 2u) { puts("alpha_pass_cull_test: opaque GEQUAL 127 must drop the second pass"); bad = 1; }
    if (p4gl_alpha_pass_cull(5, 127.0f, 255, 255, 0.5f, 255, 255) != 1u) { puts("alpha_pass_cull_test: half-alpha GEQUAL 127 must drop the first pass"); bad = 1; }
    if (p4gl_alpha_pass_cull(5, 127.0f, 255, 255, 1.0f, 0, 255) != 0u) { puts("alpha_pass_cull_test: a texture with transparent texels must keep both passes"); bad = 1; }
    if (p4gl_alpha_pass_cull(5, 128.0f, 255, 255, 1.0f, 255, 255) != 0u) { puts("alpha_pass_cull_test: GEQUAL 128 on exactly 1.0 is inside the margin and must keep both"); bad = 1; }
    if (p4gl_alpha_pass_cull(8, 0.0f, 0, 255, 1.0f, 0, 255) != 1u) { puts("alpha_pass_cull_test: NEVER drops the first pass"); bad = 1; }
    if (p4gl_alpha_pass_cull(4, 128.0f, 255, 255, 1.0f, 255, 255) != 0u || p4gl_alpha_pass_cull(7, 128.0f, 255, 255, 1.0f, 255, 255) != 0u) { puts("alpha_pass_cull_test: EQUAL/NOTEQUAL are never bounded"); bad = 1; }
    if (decided < 100) {
        printf("alpha_pass_cull_test: only %u of the grid cases were decided; the decision has become too timid\n", decided);
        bad = 1;
    }
    if (!bad) {
        printf("alpha_pass_cull_test: ok (%u decided cases)\n", decided);
    }
    return bad;
}
