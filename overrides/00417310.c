/* Retail 00417310 (RtAnimKeyFrameAdd, uncompressed keyframes): out = in1 * in2 as quaternions (floats
 * +8..+0x14: x, y, z, w) and the translations added (+0x18..+0x20). The scalar part subtracts retail's
 * accumulator chain y, x, z (0x417334..0x41733c), in that order: the EE adder is not associative. */
void func_00417310(unsigned char *out, unsigned char *in1, unsigned char *in2)
{
    const float *a = (const float *)(in1 + 8);
    const float *b = (const float *)(in2 + 8);
    float ax = a[0], ay = a[1], az = a[2], aw = a[3];
    float bx = b[0], by = b[1], bz = b[2], bw = b[3];
    float *o = (float *)(out + 8);

    o[3] = aw * bw - (ay * by + ax * bx + az * bz);
    o[0] = ay * bz - az * by;
    o[1] = az * bx - ax * bz;
    o[2] = ax * by - ay * bx;
    o[0] = bx * aw + o[0];
    o[1] = by * aw + o[1];
    o[2] = bz * aw + o[2];
    o[0] = ax * bw + o[0];
    o[1] = ay * bw + o[1];
    o[2] = az * bw + o[2];
    *(float *)(out + 0x18) = *(float *)(in1 + 0x18) + *(float *)(in2 + 0x18);
    *(float *)(out + 0x1c) = *(float *)(in1 + 0x1c) + *(float *)(in2 + 0x1c);
    *(float *)(out + 0x20) = *(float *)(in1 + 0x20) + *(float *)(in2 + 0x20);
}
