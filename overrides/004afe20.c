#include "p4pc/vu0.h"

/* VU0 boundary: the colour unpack/scale and the 255-scale pack run on the
 * shared VU0 model (vf10/vf11/vf2). Packing keeps the low byte of each lane
 * after the float-to-int convert, matching the PS2 ftoi0/ppach/ppacb sequence. */
void DistortParticleDrawSky(u8 *arg0, u8 *arg1, void *arg2, void *arg3, void *arg4, void *arg5, s32 arg6, s32 arg7)
{
    extern void func_003f6690(s32 param, void *out);
    extern void RpSkyRenderStateSet(s32 param, s32 value);
    extern f32 cosf(f32 param);
    extern f32 sinf(f32 param);
    extern s32 kwlnGetCamera(void);
    extern void func_003e42a0(DistortVec3 *out, DistortVec3 *in, void *matrix);
    extern void mdlEffectEmitMaskedTagPair(void);
    extern void mdlEffectEmitTagPair(void);
    extern void mdlEffectEmitSlotPacket(void);
    extern s32 (*D_00887310[])(s32, DistortVertex *, s32);
    extern f32 D_008872F8[];
    extern f32 D_008872FC[];
    extern f32 fGpffff8084;
    extern f32 fGpffff80f0;
    extern f32 fGpffff81f4;
    extern f32 D_00714570[];
    extern f32 D_00714574[];
    extern f32 D_00714578[];
    extern f32 D_0071457C[];
    extern f32 D_00714580[];
    extern f32 D_00714584[];
    extern f32 D_00714588[];
    extern f32 D_0071458C[];
    DistortColor packed;
    s32 st2;
    s32 st3;
    s32 cw;
    u32 pk;
    DistortVec3 tmpPos __attribute__((aligned(16)));
    DistortVec3 tmpOut __attribute__((aligned(16)));
    DistortVertex qb[4];
    f32 base;
    f32 t0;
    f32 t1;
    f32 sizeX;
    f32 sizeY;
    u32 mode;
    f32 bufferNear;
    f32 bufferFar;
    f32 cameraFar;
    f32 cameraNear;
    f32 recip;
    f32 depth;
    f32 c;
    f32 s;
    f32 f8;
    f32 f7;
    f32 f6;
    f32 f5;
    s32 bad;

    cw = *(s32 *)((u8 *)arg2 + 0x14);
    {
        f32 scale;
        P4Mmi128 lanes;

        scale = fGpffff81f4;
        lanes = p4_vu0_word_bytes_to_lanes((uint32_t)cw);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, scale);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_lqc2(11, arg4);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    }
    {
        P4Mmi128 zero = {0};
        P4Mmi128 r;

        p4_vu0_set_x(2, 0x437F0000U);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_ftoi(10, P4_VU_XYZW, 10);
        p4_vu0_qmfc2(&r, 10);
        r = p4_mmi_ppach(zero, r);
        r = p4_mmi_ppacb(zero, r);
        pk = r.w[0];
    }
    packed.word = pk;
    if (packed.bytes.alpha == 0) {
        return;
    }
    func_003f6690(2, &st2);
    func_003f6690(3, &st3);
    base = *(f32 *)((u8 *)arg2 + 0x18);
    t0 = base + *(f32 *)(arg1 + 0x8) * cosf(*(f32 *)(arg1 + 0x0));
    t1 = base + *(f32 *)(arg1 + 0x14) * cosf(*(f32 *)(arg1 + 0xC));
    mode = arg6 & 0xFF;
    switch (mode) {
    case 0:
        sizeX = (((f32 *)arg5)[0] * t0) / 32.0f;
        sizeY = (((f32 *)arg5)[1] * t1) / 32.0f;
        qb[0].r = (f32)(u32)packed.bytes.red;
        qb[0].g = (f32)(u32)packed.bytes.green;
        qb[0].b = (f32)(u32)packed.bytes.blue;
        qb[0].a = (f32)(u32)packed.bytes.alpha;
        qb[1].r = (f32)(u32)packed.bytes.red;
        qb[1].g = (f32)(u32)packed.bytes.green;
        qb[1].b = (f32)(u32)packed.bytes.blue;
        qb[1].a = (f32)(u32)packed.bytes.alpha;
        qb[2].r = (f32)(u32)packed.bytes.red;
        qb[2].g = (f32)(u32)packed.bytes.green;
        qb[2].b = (f32)(u32)packed.bytes.blue;
        qb[2].a = (f32)(u32)packed.bytes.alpha;
        qb[3].r = (f32)(u32)packed.bytes.red;
        qb[3].g = (f32)(u32)packed.bytes.green;
        qb[3].b = (f32)(u32)packed.bytes.blue;
        qb[3].a = (f32)(u32)packed.bytes.alpha;
        break;
    case 1: {
        f32 scale;

        if (*(f32 *)(arg0 + 0x28) < fGpffff80f0 || *(f32 *)(arg0 + 0x2C) < ((f32 *)&fGpffff80f0)[0]) {
            return;
        }
        ((u8 *)&packed)[3] = (u8)((255.0f * (f32)(*(u32 *)((u32)arg2 + 0x14) >> 24)) /
                                  (f32)*(u32 *)(*(u8 **)(*(u8 **)(arg0 + 0x5C) + 0x20) + 0x50));
        scale = *(f32 *)(arg1 + 0x18);
        sizeX = scale * (*(f32 *)(arg0 + 0x28) * ((((f32 *)arg5)[0] * t0) / 32.0f));
        sizeY = scale * (*(f32 *)(arg0 + 0x2C) * ((((f32 *)arg5)[1] * t1) / 32.0f));
        qb[0].r = 255.0f;
        qb[0].g = 255.0f;
        qb[0].b = 255.0f;
        qb[0].a = (f32)(u32)packed.bytes.alpha;
        qb[1].r = 255.0f;
        qb[1].g = 255.0f;
        qb[1].b = 255.0f;
        qb[1].a = (f32)(u32)packed.bytes.alpha;
        qb[2].r = 255.0f;
        qb[2].g = 255.0f;
        qb[2].b = 255.0f;
        qb[2].a = (f32)(u32)packed.bytes.alpha;
        qb[3].r = 255.0f;
        qb[3].g = 255.0f;
        qb[3].b = 255.0f;
        qb[3].a = (f32)(u32)packed.bytes.alpha;
        break;
    }
    }
    tmpPos.x = ((f32 *)arg3)[0];
    tmpPos.y = ((f32 *)arg3)[1];
    tmpPos.z = ((f32 *)arg3)[2];
    func_003e42a0(&tmpOut, &tmpPos, (u8 *)kwlnGetCamera() + 0x20);
    {
        f32 ry = tmpOut.y / tmpOut.z;
        f32 rx = tmpOut.x / tmpOut.z;

        if (rx < -2.0f || !(rx <= 2.0f) || ry < -2.0f || !(ry <= 2.0f)) {
            bad = 1;
        } else {
            bad = 0;
        }
    }
    if (bad != 0) {
        return;
    }
    bufferNear = D_008872FC[0];
    bufferFar = D_008872F8[0];
    cameraFar = *(f32 *)((u8 *)kwlnGetCamera() + 0x84);
    cameraNear = *(f32 *)((u8 *)kwlnGetCamera() + 0x80);
    depth = (cameraNear / tmpOut.z) * ((tmpOut.z - cameraFar) * ((bufferFar - bufferNear) / (cameraNear - cameraFar))) + (bufferNear + 0.0f);
    if (depth < 0.0f) {
        depth = 0.0f;
    }
    recip = 1.0f / depth;
    c = cosf(fGpffff8084 + *(f32 *)((u8 *)arg2 + 0x1C));
    s = sinf(fGpffff8084 + *(f32 *)((u32)arg2 + 0x1C));
    f8 = sizeX * c;
    f7 = sizeY * c;
    f6 = sizeX * s;
    f5 = sizeY * s;
    qb[0].u = 0.0f;
    qb[0].v = 0.0f;
    qb[1].u = 1.0f;
    qb[1].v = 0.0f;
    qb[2].u = 0.0f;
    qb[2].v = 1.0f;
    qb[3].u = 1.0f;
    qb[3].v = 1.0f;
    qb[0].x = 640.0f * ((tmpOut.x + (-f8 + f5)) / tmpOut.z);
    qb[0].y = 448.0f * ((tmpOut.y - (-f6 - f7)) / tmpOut.z);
    qb[0].z = depth;
    qb[0].recipZ = recip;
    qb[1].x = 640.0f * ((tmpOut.x + (f8 + f5)) / tmpOut.z);
    qb[1].y = 448.0f * ((tmpOut.y - (f6 - f7)) / tmpOut.z);
    qb[1].z = depth;
    qb[1].recipZ = recip;
    qb[2].x = 640.0f * ((tmpOut.x + (-f8 - f5)) / tmpOut.z);
    qb[2].y = 448.0f * ((tmpOut.y - (-f6 + f7)) / tmpOut.z);
    qb[2].z = depth;
    qb[2].recipZ = recip;
    qb[3].x = 640.0f * ((tmpOut.x + (f8 - f5)) / tmpOut.z);
    qb[3].y = 448.0f * ((tmpOut.y - (f6 + f7)) / tmpOut.z);
    qb[3].z = depth;
    qb[3].recipZ = recip;
    switch (mode) {
    case 0: {
        u32 uv;
        u32 *texture = (u32 *)TnSlotObject(0x15);
        u32 setStateTable = (u32)D_00887300;
        u32 primitiveTable;

        ((void (**)(u32, u32))setStateTable)[0](1, *texture);
        mdlEffectEmitMaskedTagPair();
        RpSkyRenderStateSet(2, 0x44);
        RpSkyRenderStateSet(3, 0x31001);
        primitiveTable = (u32)D_00887310;
        ((s32 (**)(s32, DistortVertex *, s32))primitiveTable)[0](4, qb, 4);
        mdlEffectEmitTagPair();
        ((void (**)(u32, u32))setStateTable)[0](1, **(u32 **)(arg0 + 0x60));
        RpSkyRenderStateSet(2, st2 | 0x10);
        uv = (arg7 & 0xFF) << 5;
        qb[0].u = *(f32 *)((u8 *)D_00714570 + uv);
        qb[0].v = *(f32 *)((u8 *)D_00714574 + uv);
        qb[1].u = *(f32 *)((u8 *)D_00714578 + uv);
        qb[1].v = *(f32 *)((u8 *)D_0071457C + uv);
        qb[2].u = *(f32 *)((u8 *)D_00714580 + uv);
        qb[2].v = *(f32 *)((u8 *)D_00714584 + uv);
        qb[3].u = *(f32 *)((u8 *)D_00714588 + uv);
        qb[3].v = *(f32 *)((u8 *)D_0071458C + uv);
        ((s32 (**)(s32, DistortVertex *, s32))primitiveTable)[0](4, qb, 4);
        mdlEffectEmitSlotPacket();
        break;
    }
    case 1:
        RpSkyRenderStateSet(2, 0x42);
        D_00887310[0](4, qb, 4);
        break;
    }
    RpSkyRenderStateSet(2, st2);
    RpSkyRenderStateSet(3, st3);
}
