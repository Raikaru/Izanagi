#include "p4pc/rtanim_half.h"

/* Retail 00415ad0 (RtAnimKeyFrameApply, uncompressed keyframe): builds the rotation matrix of the
 * keyframe's quaternion (floats at +8..+0x14: x, y, z, w) into `matrix`, with the keyframe's
 * translation (+0x18..+0x20) in the position row, and sets the matrix type word to 3. The padding
 * words of the rows are left as they were. */
void func_00415ad0(unsigned char *matrix, unsigned char *frame)
{
    float *m = (float *)matrix;
    const float *q = (const float *)(frame + 8);
    float x = q[0], y = q[1], z = q[2], w = q[3];
    unsigned int type = 3;

    m[0] = 1.0f - (y * y + z * z) * 2.0f;
    m[1] = (x * y + w * z) * 2.0f;
    m[2] = (z * x - w * y) * 2.0f;
    m[4] = (x * y - w * z) * 2.0f;
    m[5] = 1.0f - (x * x + z * z) * 2.0f;
    m[6] = (y * z + w * x) * 2.0f;
    m[8] = (z * x + w * y) * 2.0f;
    m[9] = (y * z - w * x) * 2.0f;
    m[10] = 1.0f - (x * x + y * y) * 2.0f;
    m[12] = *(float *)(frame + 0x18);
    m[13] = *(float *)(frame + 0x1c);
    m[14] = *(float *)(frame + 0x20);
    memcpy(&m[3], &type, sizeof type);
}
