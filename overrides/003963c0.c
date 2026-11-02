/* func_003963c0 (352 bytes): keyframe subtract for the animation blender. arg0 and arg1 point at key frames whose
 * rotation quaternion (a, b, c, d) sits at +8, +0xC, +0x10, +0x14 and whose translation sits at +0x18, +0x1C, +0x20.
 * arg0 becomes inverse(arg1.q) * arg0.q (quaternion product, the vector part computed as
 * cross(i, p) + p * i.d + i * p.d, the scalar part as i.d * p.d - dot(i, p), where p = arg0.q and i is arg1.q
 * conjugated and divided by its squared length) and arg0's translation minus arg1's. A squared length of 0 or
 * less makes retail use whatever floats lie in its uninitialised stack words as the inverse; here the inverse is the
 * identity (0, 0, 0, 1). Returns nothing. */

void func_003963c0(f32 *arg0, f32 *arg1)
{
    f32 qa = arg1[2];
    f32 qb = arg1[3];
    f32 qc = arg1[4];
    f32 qd = arg1[5];
    f32 pa;
    f32 pb;
    f32 pc;
    f32 pd;
    f32 ia;
    f32 ib;
    f32 ic;
    f32 iw;
    f32 norm;
    f32 inv;
    f32 acc;
    f32 t;

    acc = qb * qb;
    acc = acc + qa * qa;
    acc = acc + qc * qc;
    norm = acc + qd * qd;
    if (norm <= 0.0f) {
        ia = 0.0f;
        ib = 0.0f;
        ic = 0.0f;
        iw = 1.0f;
    } else {
        inv = 1.0f / norm;
        iw = qd * inv;
        inv = -inv;
        ia = qa * inv;
        ib = qb * inv;
        ic = qc * inv;
    }
    pa = arg0[2];
    pb = arg0[3];
    pc = arg0[4];
    pd = arg0[5];

    acc = ib * pb;
    acc = acc + ia * pa;
    acc = acc + ic * pc;
    arg0[5] = iw * pd - acc;

    acc = ib * pc;
    arg0[2] = acc - ic * pb;
    acc = ic * pa;
    arg0[3] = acc - ia * pc;
    acc = ia * pb;
    arg0[4] = acc - ib * pa;

    t = 0.0f + arg0[2];
    arg0[2] = t + pa * iw;
    t = 0.0f + arg0[3];
    arg0[3] = t + pb * iw;
    t = 0.0f + arg0[4];
    arg0[4] = t + pc * iw;
    t = 0.0f + arg0[2];
    arg0[2] = t + ia * pd;
    t = 0.0f + arg0[3];
    arg0[3] = t + ib * pd;
    t = 0.0f + arg0[4];
    arg0[4] = t + ic * pd;

    arg0[6] = arg0[6] - arg1[6];
    arg0[7] = arg0[7] - arg1[7];
    arg0[8] = arg0[8] - arg1[8];
}
