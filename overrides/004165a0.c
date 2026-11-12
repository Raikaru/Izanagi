#include "p4pc/rtanim_half.h"

/* Retail 004165a0 (RtAnimKeyFrameBlend, uncompressed keyframes): out = slerp(in1, in2, alpha) for
 * the quaternion (floats +8..+0x14) and lerp for the translation (+0x18..+0x20). When the two
 * quaternions point away from each other (negative dot) retail negates the second one IN PLACE in
 * `in2` and blends towards that, which this does too. The dot product is retail's accumulator chain
 * y, x, z, w (0x4165c0..0x4165e0): the EE adder is not associative. */
void func_004165a0(unsigned char *out, unsigned char *in1, unsigned char *in2, float alpha)
{
    float *q1 = (float *)(in1 + 8);
    float *q2 = (float *)(in2 + 8);
    float dot = q1[1] * q2[1] + q1[0] * q2[0] + q1[2] * q2[2] + q1[3] * q2[3];
    int i;

    for (i = 0; i < 3; i++) {
        float a = *(float *)(in1 + 0x18 + i * 4);
        float b = *(float *)(in2 + 0x18 + i * 4);

        *(float *)(out + 0x18 + i * 4) = alpha * (b - a) + a;
    }
    if (dot < 0.0f) {
        dot = -dot;
        q2[0] = -q2[0];
        q2[1] = -q2[1];
        q2[2] = -q2[2];
        q2[3] = -q2[3];
    }
    p4_anim_slerp((float *)(out + 8), q1, q2, dot, alpha);
}
