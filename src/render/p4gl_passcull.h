/* Alpha-test pass culling: the decision, as a pure function (docs/platform/performance.md; the draw in p4gl_atomic.c supplies the bounds).
 *
 * A draw under the GS alpha test whose failure mode still writes colour (AFAIL = FB_ONLY or RGB_ONLY) is two draws: fragments that pass the test write colour
 * and depth, fragments that fail write colour only. The fragment alpha is vertex alpha x material alpha x texture alpha; each factor is interpolated or filtered,
 * so it stays inside the range of its source values and so does the product. When the bounds show that no fragment can fail, the second draw covers no pixel;
 * when none can pass, the first covers none. The bounds carry a margin larger than the fragment shader's float error and its snap to whole GS alpha values
 * (p4GsAlpha rounds within 1/256), so a pass is dropped only when the comparison has the same result for every possible fragment and the picture is unchanged.
 *
 * func is the shader's alpha function (2 LESS, 3 LEQUAL, 4 EQUAL, 5 GEQUAL, 6 GREATER, 7 NOTEQUAL, 8 NEVER), ref the GS alpha reference (0..255, 0x80 is 1.0).
 * vlo..vhi are the smallest and largest vertex alpha byte, matAlpha the material's alpha (0..1), tlo..thi the texture's alpha bytes (255, 255 when untextured).
 * Returns bit 0: the first (passing) draw is empty; bit 1: the second (failing) draw is empty. */
#ifndef P4GL_PASSCULL_H
#define P4GL_PASSCULL_H

#include <stdint.h>

static inline unsigned p4gl_alpha_pass_cull(int32_t func, float ref, unsigned vlo, unsigned vhi, float matAlpha, unsigned tlo, unsigned thi)
{
    float lo, hi;

    if (func == 8) {
        return 1u; /* NEVER: the first pass discards every fragment */
    }
    if ((func != 2 && func != 3 && func != 5 && func != 6) || vlo > vhi || tlo > thi || vhi > 255u || thi > 255u || !(matAlpha >= 0.0f && matAlpha <= 1.0f)) {
        return 0u; /* EQUAL, NOTEQUAL and anything unrecognised are not bounded by a range */
    }
    lo = 128.0f * ((float)vlo / 255.0f) * matAlpha * ((float)tlo / 255.0f) * 0.9999f - 0.02f;
    hi = 128.0f * ((float)vhi / 255.0f) * matAlpha * ((float)thi / 255.0f) * 1.0001f + 0.02f;
    if (func == 5) {
        return lo >= ref ? 2u : hi < ref ? 1u : 0u;
    }
    if (func == 6) {
        return lo > ref ? 2u : hi <= ref ? 1u : 0u;
    }
    if (func == 2) {
        return hi < ref ? 2u : lo >= ref ? 1u : 0u;
    }
    return hi <= ref ? 2u : lo > ref ? 1u : 0u;
}

#endif
