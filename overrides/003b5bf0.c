/* func_003b5bf0 (304 bytes): like func_003b5ac0, with a transform offset. arg0: +0
 * destination base, +4 slot stride S. arg3 is the record count; arg4: +0x0C record base,
 * +0x10 record stride, +0x68 and +0x6C floats c68, c6C. arg1 and arg2 are unused. For a
 * record r (float indices): a0 = 0.5*(r[0]-r[4]), b0 = 0.5*(r[0]+r[4]),
 * a1 = 0.5*(r[1]-r[5]), b1 = 0.5*(r[1]+r[5]), a2 = 0.5*(r[2]-r[6]), c = 0.5*(r[2]+r[6]),
 * and the offset position per component k in {0, 1, 2} is
 *
 *   o_k = ((0.0 + r[12+k]) + r[k]*c68) + r[4+k]*c6C
 *
 * (the retail code accumulates with adda.s/madda.s/madd.s). The corners, three floats each:
 *
 *   dst + 0*S : (o0 + a0, o1 + a1, o2 + a2)     dst + 1*S : (o0 - b0, o1 - b1, o2 - c)
 *   dst + 2*S : (o0 - a0, o1 - a1, o2 - a2)     dst + 3*S : (o0 + b0, o1 + b1, o2 + c)
 *
 * then dst advances by 4*S and the record by its stride. Does nothing when arg3 <= 0.
 * Leaf function. */

void func_003b5bf0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *rec;
    u32 stride;
    u32 recStride;
    f32 c68;
    f32 c6C;
    s32 i;

    (void)arg1;
    (void)arg2;
    c68 = *(f32 *)(arg4 + 0x68);
    rec = *(u8 **)(arg4 + 0xC);
    c6C = *(f32 *)(arg4 + 0x6C);
    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    recStride = *(u32 *)(arg4 + 0x10);

    for (i = 0; i < arg3; i++) {
        f32 *r = (f32 *)rec;
        f32 *q0 = (f32 *)dst;
        f32 *q1 = (f32 *)(dst + stride);
        f32 *q2 = (f32 *)(dst + 2 * stride);
        f32 *q3 = (f32 *)(dst + 3 * stride);
        f32 a0 = 0.5f * (r[0] - r[4]);
        f32 b1 = 0.5f * (r[1] + r[5]);
        f32 c = 0.5f * (r[2] + r[6]);
        f32 a1 = 0.5f * (r[1] - r[5]);
        f32 b0 = 0.5f * (r[0] + r[4]);
        f32 a2 = 0.5f * (r[2] - r[6]);
        f32 o0 = ((0.0f + r[12]) + r[0] * c68) + r[4] * c6C;
        f32 o1 = ((0.0f + r[13]) + r[1] * c68) + r[5] * c6C;
        f32 o2 = ((0.0f + r[14]) + r[2] * c68) + r[6] * c6C;

        rec += recStride;
        q0[0] = o0 + a0;
        q0[1] = o1 + a1;
        q0[2] = o2 + a2;
        q1[0] = o0 - b0;
        q1[1] = o1 - b1;
        q1[2] = o2 - c;
        q2[0] = o0 - a0;
        q2[1] = o1 - a1;
        q2[2] = o2 - a2;
        q3[0] = o0 + b0;
        q3[1] = o1 + b1;
        q3[2] = o2 + c;
        dst += 4 * stride;
    }
}
