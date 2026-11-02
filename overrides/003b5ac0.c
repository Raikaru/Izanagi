/* func_003b5ac0 (296 bytes): writes four corner positions per record from a
 * record that holds half-extent terms and a position. arg0: +0 destination base, +4 slot
 * stride S. arg3 is the record count; arg4: +0x0C record base, +0x10 record stride. arg1 and
 * arg2 are unused. For a record r (floats): a = 0.5*r[0], b = 0.5*r[4], c = 0.5*r[2],
 * d = 0.5*r[1], e = 0.5*r[5], f = 0.5*r[6] (float indices), position P = r[12..14]. With
 * D = (a - b, d - e, c - f) and Q = (a + b, d + e, c + f):
 *
 *   dst + 0*S : P + D     dst + 1*S : P - Q
 *   dst + 2*S : P - D     dst + 3*S : P + Q
 *
 * then dst advances by 4*S and the record by its stride. Does nothing when arg3 <= 0.
 * Leaf function. */

void func_003b5ac0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *rec;
    u32 stride;
    u32 recStride;
    s32 i;

    (void)arg1;
    (void)arg2;
    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    rec = *(u8 **)(arg4 + 0xC);
    recStride = *(u32 *)(arg4 + 0x10);

    for (i = 0; i < arg3; i++) {
        f32 *r = (f32 *)rec;
        f32 *p = (f32 *)(rec + 0x30);
        f32 *q0 = (f32 *)dst;
        f32 *q1 = (f32 *)(dst + stride);
        f32 *q2 = (f32 *)(dst + 2 * stride);
        f32 *q3 = (f32 *)(dst + 3 * stride);
        f32 a = 0.5f * r[0];
        f32 b = 0.5f * r[4];
        f32 c = 0.5f * r[2];
        f32 d = 0.5f * r[1];
        f32 e = 0.5f * r[5];
        f32 f = 0.5f * r[6];
        f32 qx = a + b;
        f32 dx = a - b;
        f32 qy = d + e;
        f32 dy = d - e;
        f32 qz = c + f;
        f32 dz = c - f;

        rec += recStride;
        q0[0] = p[0] + dx;
        q0[1] = p[1] + dy;
        q0[2] = p[2] + dz;
        q1[0] = p[0] - qx;
        q1[1] = p[1] - qy;
        q1[2] = p[2] - qz;
        q2[0] = p[0] - dx;
        q2[1] = p[1] - dy;
        q2[2] = p[2] - dz;
        q3[0] = p[0] + qx;
        q3[1] = p[1] + qy;
        q3[2] = p[2] + qz;
        dst += 4 * stride;
    }
}
