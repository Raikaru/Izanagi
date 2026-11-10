#include "p4pc/vu0.h"

void effMiscAxisAngleMatrix(f32 angle, const RwV3d* axis, RwMatrix* matrix)
{
    f32 xSquared;
    f32 ySquared;
    f32 zSquared;
    f32 xz;
    const RwV3d* axis_p;
    RwMatrix* matrix_p;
    f32 angle_p;
    RwV4d normalizedAxis;
    f32 cosine;
    f32 sine;
    f32 oneMinusCosine;
    f32 xy;
    f32 zSine;
    f32 x;
    f32 y;
    f32 z;
    f32 axisPadded[4];

    angle_p = angle;
    axis_p = axis;
    matrix_p = matrix;
    cosine = cosf(angle_p);
    sine = sinf(angle_p);

    /* The RwV3d is 12 bytes; the w lane is zero-filled rather than read past it. */
    axisPadded[0] = axis_p->x;
    axisPadded[1] = axis_p->y;
    axisPadded[2] = axis_p->z;
    axisPadded[3] = 0.0f;
    p4_vu0_lqc2(10, axisPadded);
    p4_vu0_mul(2, P4_VU_XYZW, 10, 10, P4_VU_NOBC);
    p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
    p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
    p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
    p4_vu0_mulq(10, P4_VU_XYZ, 10);
    p4_vu0_sqc2(&normalizedAxis, 10);

    x = normalizedAxis.x;
    y = normalizedAxis.y;
    z = normalizedAxis.z;
    xSquared = x * x;
    matrix_p->right.x = xSquared + (1.0f - xSquared) * cosine;
    xy = x * y;
    oneMinusCosine = 1.0f - cosine;
    zSine = z * sine;
    xy = xy * oneMinusCosine;
    matrix_p->right.y = zSine + xy;
    xz = x * z;
    matrix_p->right.z = xz * (1.0f - cosine) - y * sine;
    matrix_p->flags = 0;

    matrix_p->up.x = xy - zSine;
    ySquared = y * y;
    matrix_p->up.y = ySquared + (1.0f - ySquared) * cosine;
    xSquared = y * z;
    ySquared = x * sine;
    matrix_p->up.z = ySquared + xSquared * (1.0f - cosine);
    matrix_p->pad1 = 0;

    matrix_p->at.x = y * sine + x * z * (1.0f - cosine);
    matrix_p->at.y = y * z * (1.0f - cosine) - x * sine;
    zSquared = z * z;
    matrix_p->at.z = zSquared + (1.0f - zSquared) * cosine;
    matrix_p->pad2 = 0;

    p4_vu0_sqc2((unsigned char *)matrix_p + 0x30, 0);
}
