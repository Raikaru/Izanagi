#include "p4pc/rtanim_half.h"

/* Retail 00416e10 (RtAnimKeyFrameMulRecip, compressed keyframes): `frame` = frame * reciprocal(start),
 * in place. The reciprocal of the start quaternion is its conjugate over its squared length (a zero
 * length gives retail's uninitialised stack; the port uses the identity). Quaternion components and
 * translation are 16-bit floats (+8..+0xf and +0x10..+0x15); the translation becomes
 * frame.t - start.t. Every sum is retail's accumulator chain, in its order (the EE adder is not
 * associative): the squared length y, x, z, w (0x416f54), each vector component the cross term, then
 * f * i.w, then i * f.w (0x4170b0..0x4170e0), the scalar i.w * f.w less y, x, z (0x417110..0x417124). */
void func_00416e10(unsigned char *frame, unsigned char *start)
{
    float s[4], f[4];
    float ix = 0.0f, iy = 0.0f, iz = 0.0f, iw = 1.0f;
    float len2;
    int i;

    for (i = 0; i < 4; i++) {
        s[i] = p4_anim_half_to_float(*(unsigned short *)(start + 8 + i * 2));
        f[i] = p4_anim_half_to_float(*(unsigned short *)(frame + 8 + i * 2));
    }
    len2 = s[1] * s[1] + s[0] * s[0] + s[2] * s[2] + s[3] * s[3];
    if (0.0f < len2) {
        float r = 1.0f / len2;

        iw = s[3] * r;
        r = -r;
        ix = s[0] * r;
        iy = s[1] * r;
        iz = s[2] * r;
    }
    {
        float fx = f[0], fy = f[1], fz = f[2], fw = f[3];
        float rx = (iy * fz - iz * fy) + fx * iw + ix * fw;
        float ry = (iz * fx - ix * fz) + fy * iw + iy * fw;
        float rz = (ix * fy - iy * fx) + fz * iw + iz * fw;
        float rw = iw * fw - (iy * fy + ix * fx + iz * fz);

        *(unsigned short *)(frame + 8) = p4_anim_float_to_half(rx);
        *(unsigned short *)(frame + 10) = p4_anim_float_to_half(ry);
        *(unsigned short *)(frame + 12) = p4_anim_float_to_half(rz);
        *(unsigned short *)(frame + 14) = p4_anim_float_to_half(rw);
    }
    for (i = 0; i < 3; i++) {
        float ft = p4_anim_half_to_float(*(unsigned short *)(frame + 0x10 + i * 2));
        float st = p4_anim_half_to_float(*(unsigned short *)(start + 0x10 + i * 2));

        *(unsigned short *)(frame + 0x10 + i * 2) = p4_anim_float_to_half(ft - st);
    }
}
