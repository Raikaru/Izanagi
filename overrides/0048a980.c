#include "p4pc/vu0.h"

void effMiscMatrixToQuatVU(f32 *arg0)
{
    extern f32 sqrtf(f32);
    f32 q[4] __attribute__((aligned(16)));
    f32 zz, yy, xx;
    f32 one;
    f32 traceXY, traceXYZ, trace;

    yy = arg0[5];
    xx = arg0[0];
    zz = arg0[10];
    one = 1.0f;
    traceXY = xx + yy;
    traceXYZ = zz + traceXY;
    trace = one + traceXYZ;
    if (!(trace < one)) {
        f32 s = 2.0f * sqrtf(trace);
        q[3] = -(s / 4.0f);
        q[0] = (arg0[6] - arg0[9]) / s;
        q[1] = (arg0[8] - arg0[2]) / s;
        q[2] = (arg0[1] - arg0[4]) / s;
    } else {
        u32 selected = (((xx > yy) ? 1 : 0) ^ 1) & 0xFF;
        s32 i;
        s32 j, k;
        s32 axisCount;
        s32 next;
        s32 oi, oj, ok;
        u8 *ri, *rj, *rk;
        f32 dj, delta, radicand, sum;
        f32 root;
        f32 identity;
        f32 zero;

        if (!(zz <= *(f32 *)((u8 *)arg0 + selected * 16 + selected * 4))) {
            u8 third = 2;
            selected = third;
        }
        i = (u8)selected;
        next = i + 1;
        axisCount = 3;
        j = (u8)(next % axisCount);
        k = (u8)((j + 1) % axisCount);
        oj = j * 4;
        rj = (u8 *)arg0 + j * 16;
        dj = *(f32 *)(rj + oj);
        oi = i * 4;
        ri = (u8 *)arg0 + i * 16;
        delta = *(f32 *)(ri + oi) - dj;
        ok = k * 4;
        rk = (u8 *)arg0 + k * 16;
        radicand = delta - *(f32 *)(rk + ok);
        identity = 1.0f;
        sum = identity + radicand;
        root = sqrtf(sum);
        {
            f32 scaled = 2.0f * root;
            root = scaled;
        }
        zero = 0.0f;
        if (zero != root) {
            *(f32 *)((u8 *)q + oi) = root / 4.0f;
            *(f32 *)((u8 *)q + oj) = (*(f32 *)(ri + oj) + *(f32 *)(rj + oi)) / root;
            *(f32 *)((u8 *)q + ok) = (*(f32 *)(ri + ok) + *(f32 *)(rk + oi)) / root;
            q[3] = -((*(f32 *)(rj + ok) - *(f32 *)(rk + oj)) / root);
        } else {
            *(f32 *)((u8 *)q + oi) = identity;
            *(f32 *)((u8 *)q + oj) = zero;
            *(f32 *)((u8 *)q + ok) = zero;
            q[3] = zero;
        }
    }
    /* The quaternion is returned through the established VU0 vf10 ABI. */
    p4_vu0_lqc2(10, q);
}
