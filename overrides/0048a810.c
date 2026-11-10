#include "p4pc/vu0.h"

/* Retail 0048a810 forms the two Catmull-Rom tangents (p2 - p0) / 2 and (p3 - p1) / 2 from four
 * quadword control points, then leaves p1*b0 + t1*b1 + t2*b2 + p2*b3 in vf10 for the caller to
 * store. The Hermite basis is built in float exactly as the decomp wrote it. The w lanes of the
 * tangent and basis rows are stack garbage in retail (never initialised), so they are zero here. */
void mdlEffectCatmullRomVU(float param_1, u8 (*param_2)[16])
{
    const float *p0 = (const float *)param_2[0];
    const float *p1 = (const float *)param_2[1];
    const float *p2 = (const float *)param_2[2];
    const float *p3 = (const float *)param_2[3];
    float square, cube, b0, b1, b2, b3;
    float tan1[4], tan2[4];
    float result[4] __attribute__((aligned(16)));
    int i;

    square = param_1 * param_1;
    cube = square * param_1;
    b0 = cube * 2.0f - square * 3.0f + 1.0f;
    b1 = param_1 + (cube - square * 2.0f);
    b2 = cube - square;
    b3 = cube * -2.0f + square * 3.0f;

    for (i = 0; i < 3; i++) {
        tan1[i] = ((p2[i] - p1[i]) + (p1[i] - p0[i])) * 0.5f;
        tan2[i] = ((p3[i] - p2[i]) + (p2[i] - p1[i])) * 0.5f;
    }
    tan1[3] = tan2[3] = 0.0f;

    for (i = 0; i < 3; i++) {
        result[i] = p2[i] * b3 + (tan2[i] * b2 + (tan1[i] * b1 + p1[i] * b0));
    }
    result[3] = 0.0f;
    p4_vu0_lqc2(10, result);
}
