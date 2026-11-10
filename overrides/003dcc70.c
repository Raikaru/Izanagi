#include "p4pc/rtanim_half.h"

/* Retail 003dcc70 (code1_003d.c): prepares a quaternion slerp between q1 and q2 into the 10-float
 * record `out`: out[0..3] = q1; out[4..7] = q2, negated when the dot product is negative (the shorter
 * arc); out[8] = the angle acos(|dot|) between them (|dot| limited to 1); the integer word out[9] = 1 when the two are
 * nearly parallel (|dot| >= 0.99999) and 0 otherwise. For the not-parallel case both quaternions
 * in the record are divided by sin(angle), so the slerp evaluator only needs sin(t * angle) and
 * sin((1 - t) * angle). Retail inlines its acos and sin polynomials, the ones of rtanim_half.h.
 * The dot product is retail's accumulator chain (mula y, madda x, madda z, madd w): the EE adder is
 * not associative, and summing in another order moved the angle by up to an ulp of the dot, which the
 * acos near 1 magnifies (the battle opening camera's third curve key, btlMainCurveSetThreePoses). */
void func_003dcc70(float *q1, float *q2, float *out)
{
    float dot, c;
    int i;

    out[0] = q1[0];
    out[1] = q1[1];
    out[2] = q1[2];
    out[3] = q1[3];
    dot = q1[1] * q2[1] + q1[0] * q2[0] + q1[2] * q2[2] + q1[3] * q2[3];
    if (dot < 0.0f) {
        c = dot < -1.0f ? 1.0f : -dot;
        out[7] = -q2[3];
        out[4] = -q2[0];
        out[5] = -q2[1];
        out[6] = -q2[2];
    } else {
        c = dot > 1.0f ? 1.0f : dot;
        out[4] = q2[0];
        out[5] = q2[1];
        out[6] = q2[2];
        out[7] = q2[3];
    }
    out[8] = p4_anim_acos(c);
    *(int *)&out[9] = 0.99999f <= c ? 1 : 0;    /* retail stores the integer flag (li 1 / move zero; sw), not a float */
    if (*(int *)&out[9] == 0) {
        float inv = 1.0f / p4_anim_sin(out[8]);

        for (i = 0; i < 8; i++) {
            out[i] = out[i] * inv;
        }
    }
}
