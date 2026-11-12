#include "p4pc/rtanim_half.h"

/* Retail 00415bc0 (RtAnimKeyFrameInterpolate, compressed keyframes): interpolates between two
 * compressed keyframes at `time` into the uncompressed frame `out`. A compressed keyframe holds the
 * time (float, +4), the quaternion as four 16-bit floats (+8..+0xf) and the translation as three
 * (+0x10..+0x15). The translation is lerped, then scaled and offset by `custom` (offset x, y, z at
 * [0..2], scale x, y, z at [3..5]). The quaternion is slerped (second keyframe negated when the dot
 * is negative) into floats at out+8..+0x14. The dot product is retail's accumulator chain y, x, z, w
 * (0x415f4c..0x415f88): the EE adder is not associative. */
void func_00415bc0(unsigned char *out, unsigned char *in1, unsigned char *in2, float time, float *custom)
{
    float q1[4], q2[4], t1[3], t2[3], q[4], tr[3];
    float f, dot;
    int i;

    for (i = 0; i < 4; i++) {
        q1[i] = p4_anim_half_to_float(*(unsigned short *)(in1 + 8 + i * 2));
        q2[i] = p4_anim_half_to_float(*(unsigned short *)(in2 + 8 + i * 2));
    }
    for (i = 0; i < 3; i++) {
        t1[i] = p4_anim_half_to_float(*(unsigned short *)(in1 + 0x10 + i * 2));
        t2[i] = p4_anim_half_to_float(*(unsigned short *)(in2 + 0x10 + i * 2));
    }
    f = (time - *(float *)(in1 + 4)) / (*(float *)(in2 + 4) - *(float *)(in1 + 4));
    dot = q1[1] * q2[1] + q1[0] * q2[0] + q1[2] * q2[2] + q1[3] * q2[3];
    for (i = 0; i < 3; i++) {
        tr[i] = (f * (t2[i] - t1[i]) + t1[i]) * custom[3 + i] + custom[i];
        *(float *)(out + 0x18 + i * 4) = tr[i];
    }
    if (dot < 0.0f) {
        for (i = 0; i < 4; i++) {
            q2[i] = -q2[i];
        }
        dot = -dot;
    }
    p4_anim_slerp(q, q1, q2, dot, f);
    for (i = 0; i < 4; i++) {
        *(float *)(out + 8 + i * 4) = q[i];
    }
}
