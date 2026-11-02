#include "p4pc/vu0.h"

void mdlEffectAdvanceRibbon(u8 *track, u16 trackIndex, u8 *point, f32 width)
{
    extern void mdlEffectAxisAngleBasisVU(f32 angle);
    extern u_long128 D_00713D00;
    extern f32 fGpffff8094;
    extern f32 fGpffff80bc;
    extern f32 fGpffff80c0;
    extern f32 fGpffff80c8;
    extern f32 fGpffff80cc;
    u_long128 headTangent;
    u_long128 headSide;
    f32 lane[4];
    u8 *followingCenter;
    u8 *previousCenter;
    s32 vertexCount;
    s32 shiftedVertex;
    s32 rowCount;
    u32 lastRow;
    u8 *countSlot;
    u32 activeRows;
    u8 *row;
    u8 *firstRow;
    u8 *nxt;
    f32 fraction;
    f32 fractionStep;
    f32 headWidth;
    f32 rotation;
    f32 dot;
    u32 rowIndex;
    u32 lastActiveRow;
    u8 *cap;
    f32 capLength;
    f32 capWidth;
    f32 wide;
    f32 inner;
    f32 blendWide;
    f32 blendInner;
    f32 endpoint;

    lane[3] = 0.0f;
    {
        u8 *t10 = *(u8 **)(track + 0x10);
        vertexCount = *(u16 *)(t10 + 8);
        {
            u8 *t1 = *(u8 **)(t10 + 0x10);
            u8 *t2 = *(u8 **)(t1 + 0x18);
            u8 *t3 = *(u8 **)(t2 + 0x5C);
            u8 *tab = *(u8 **)(t3 + 0x14);
            row = tab + (u32)vertexCount * mdlEffectTrackIndex(trackIndex) * 12;
        }
        previousCenter = row + (vertexCount - 5) * 12;
        followingCenter = row + (vertexCount - 2) * 12;
    }
    /* Shift the centre history one step toward the tail. */
    for (shiftedVertex = 3; shiftedVertex < vertexCount; shiftedVertex += 3) {
        *(TrackVertex *)(followingCenter) = *(const TrackVertex *)(previousCenter);
        previousCenter -= 0x24;
        followingCenter -= 0x24;
    }
    *(TrackVertex *)(row + 0xc) = *(const TrackVertex *)(point);
    rowCount = (s32)vertexCount / 3 - 1;
    lastRow = (u16)rowCount;
    countSlot = *(u8 **)(track + 0x18) + (mdlEffectTrackIndex(trackIndex) * 2);
    activeRows = *(u16 *)countSlot;
    if ((s32)activeRows <= (s32)lastRow) {
        activeRows = (u16)(activeRows + 1);
        *(u16 *)countSlot = (u16)activeRows;
    }
    firstRow = row;
    headWidth = 0.0f;
    p4_vu0_sqc2(&headTangent, 0);
    p4_vu0_sqc2(&headSide, 0);
    rowIndex = 0;
    if ((s32)activeRows > 1) {
        fraction = (f32)activeRows / (f32)lastRow;
        fractionStep = fraction / (f32)activeRows;
        headWidth = width * fraction;
        lastActiveRow = activeRows - 1;
        p4_vu0_lqc2(10, &D_00713D00);
        p4_vu0_move(24, P4_VU_XYZW, 10);
        p4_vu0_lqc2(11, track);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 11, P4_VU_NOBC);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
        dot = p4pc_vu0.vf[2].f[0];
        rotation = fGpffff8094 * dot;

        /* Row 0 also latches the head tangent and side vectors used by the cap. */
        for (rowIndex = 0; rowIndex < lastActiveRow; rowIndex++) {
            lane[0] = *(f32 *)(row + 12);
            lane[1] = *(f32 *)(row + 16);
            lane[2] = *(f32 *)(row + 20);
            p4_vu0_lqc2(12, lane);
            lane[0] = *(f32 *)(row + 48);
            lane[1] = *(f32 *)(row + 52);
            lane[2] = *(f32 *)(row + 56);
            p4_vu0_lqc2(10, lane);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
            p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            if (rowIndex == 0) {
                p4_vu0_sqc2(&headTangent, 10);
            }
            mdlEffectAxisAngleBasisVU(rotation);
            p4_vu0_move(10, P4_VU_XYZW, 24);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            if (rowIndex == 0) {
                p4_vu0_sqc2(&headSide, 10);
            }
            p4_vu0_set_xf(2, width * fraction);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
            p4_vu0_add(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
            p4_vu0_sqc2(lane, 12);
            *(f32 *)(row) = lane[0];
            *(f32 *)(row + 4) = lane[1];
            *(f32 *)(row + 8) = lane[2];
            p4_vu0_sub(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
            p4_vu0_sub(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
            p4_vu0_sqc2(lane, 12);
            *(f32 *)(row + 24) = lane[0];
            *(f32 *)(row + 28) = lane[1];
            *(f32 *)(row + 32) = lane[2];
            fraction = fraction - fractionStep;
            row += 0x24;
        }
    }

    /* Collapse: repeat the last row's tail vertices through the remaining rows. */
    *(TrackVertex *)(row) = *(const TrackVertex *)(row + 0xc);
    *(TrackVertex *)(row + 0x18) = *(const TrackVertex *)(row + 0xc);
    nxt = row + 0x24;
    while (rowIndex < lastRow) {
        *(TrackVertex *)(nxt) = *(const TrackVertex *)(row);
        *(TrackVertex *)(nxt + 0xc) = *(const TrackVertex *)(row + 0xc);
        *(TrackVertex *)(nxt + 0x18) = *(const TrackVertex *)(row + 0x18);
        nxt += 0x24;
        rowIndex++;
    }

    /* Cap: mirror the first row's vertices, then offset along the head side vector. */
    {
        u8 *t = *(u8 **)(track + 0x14);
        u32 c2 = *(u16 *)(t + 8);
        u8 *t1 = *(u8 **)(t + 0x10);
        u8 *t2 = *(u8 **)(t1 + 0x18);
        u8 *t3 = *(u8 **)(t2 + 0x5C);
        u8 *tab2 = *(u8 **)(t3 + 0x14);
        cap = tab2 + (u32)c2 * mdlEffectTrackIndex(trackIndex) * 12;
        *(TrackVertex *)cap = *(const TrackVertex *)(firstRow + 12);
        *(TrackVertex *)(cap + 0xc) = *(const TrackVertex *)(firstRow);
        *(TrackVertex *)(cap + 0x54) = *(const TrackVertex *)(firstRow + 0x18);
    }
    p4_vu0_lqc2(11, &headTangent);
    p4_vu0_sub(11, P4_VU_XYZ, 0, 11, P4_VU_NOBC);
    capLength = 2.0f * headWidth;
    p4_vu0_set_xf(2, capLength);
    p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    lane[0] = *(f32 *)(firstRow + 12);
    lane[1] = *(f32 *)(firstRow + 16);
    lane[2] = *(f32 *)(firstRow + 20);
    p4_vu0_lqc2(12, lane);

    p4_vu0_lqc2(10, &headSide);
    wide = fGpffff80bc * headWidth;
    capWidth = wide;
    p4_vu0_set_xf(2, capWidth);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    blendWide = fGpffff80c0;
    p4_vu0_set_xf(2, blendWide);
    p4_vu0_sub(3, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    p4_vu0_madd(10, P4_VU_XYZW, 10, 3, P4_VU_BC_W);
    p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
    p4_vu0_sqc2(lane, 10);
    *(f32 *)(cap + 24) = lane[0];
    *(f32 *)(cap + 28) = lane[1];
    *(f32 *)(cap + 32) = lane[2];

    p4_vu0_lqc2(10, &headSide);
    inner = 1.7f * headWidth;
    capWidth = inner;
    p4_vu0_set_xf(2, capWidth);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    blendInner = fGpffff80c8;
    p4_vu0_set_xf(2, blendInner);
    p4_vu0_sub(3, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    p4_vu0_madd(10, P4_VU_XYZW, 10, 3, P4_VU_BC_W);
    p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
    p4_vu0_sqc2(lane, 10);
    *(f32 *)(cap + 36) = lane[0];
    *(f32 *)(cap + 40) = lane[1];
    *(f32 *)(cap + 44) = lane[2];

    p4_vu0_lqc2(10, &headSide);
    p4_vu0_set_xf(2, wide);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
    p4_vu0_set_xf(2, blendWide);
    p4_vu0_sub(3, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    p4_vu0_madd(10, P4_VU_XYZW, 10, 3, P4_VU_BC_W);
    p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
    p4_vu0_sqc2(lane, 10);
    *(f32 *)(cap + 72) = lane[0];
    *(f32 *)(cap + 76) = lane[1];
    *(f32 *)(cap + 80) = lane[2];

    p4_vu0_lqc2(10, &headSide);
    p4_vu0_set_xf(2, inner);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
    p4_vu0_set_xf(2, blendInner);
    p4_vu0_sub(3, P4_VU_W, 0, 2, P4_VU_BC_X);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    p4_vu0_madd(10, P4_VU_XYZW, 10, 3, P4_VU_BC_W);
    p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
    p4_vu0_sqc2(lane, 10);
    *(f32 *)(cap + 60) = lane[0];
    *(f32 *)(cap + 64) = lane[1];
    *(f32 *)(cap + 68) = lane[2];

    endpoint = fGpffff80cc;
    p4_vu0_set_xf(2, endpoint);
    p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    p4_vu0_add(11, P4_VU_XYZW, 11, 12, P4_VU_NOBC);
    p4_vu0_sqc2(lane, 11);
    *(f32 *)(cap + 48) = lane[0];
    *(f32 *)(cap + 52) = lane[1];
    *(f32 *)(cap + 56) = lane[2];
}
