#include "p4pc/vu0.h"

void LineNovaUpdateCameraQuads(u8 *unused, u8 *effect)
{
    extern s32 mdlEffectSamplePackedColor(u8 *, u8 *, s32, s32);
    extern u8 *kwlnGetCamera(void);
    extern u8 *func_003c22f0(u8 *);
    extern u8 *func_003e9700(u8 *);
    extern u8 *func_003e9cb0(u8 *, u8 *, s32);
    extern f32 fGpffff8044;
    extern u8 D_00713CF0[], D_00713D10[], D_00713D14[], D_00713D18[];
    typedef struct { u8 r, g, b, a; } NovaRgba;
    union { u32 word; u8 byte[4]; NovaRgba color; } rgba;
    u32 effectColor, sampled, packed;
    u32 sampleResult;
    const u32 *colorInput;
    u8 *instance, *state, *config, *owner;
    u8 *cameraDimensions;
    u8 *particle;
    f32 normalization;
    u32 count, index;
    f32 *vertices;

    extern u8 *func_003e42a0(u8 *, u8 *, u8 *);
    EffectVuVector projected;
    union { EffectVuVector vector; u32 word[4]; } center;
    EffectVuVector corners[4], interpolated;
    f32 farClip, xScale, yScale, xExtent, yExtent, minusX, minusY;
    f32 width, spread, divisor;
    u8 mode;

    instance = effect;
    state = *(u8 **)(instance + 0x20);
    config = *(u8 **)(instance + 0x24);
    owner = *(u8 **)(state + 4);

    sampleResult = (u32)mdlEffectSamplePackedColor(config, config + 0x24, (s32)*(u32 *)(instance + 0x14), (s32)*(u32 *)(config + 0x34));
    effectColor = *(u32 *)(instance + 0x10);
    colorInput = &effectColor;
    normalization = fGpffff8044;
    effectVuUnpackColor10V0(colorInput, normalization);
    p4_vu0_move(11, P4_VU_XYZW, 10);
    sampled = sampleResult;
    effectVuUnpackColor10V0(&sampled, normalization);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    {
        P4Mmi128 g;
        P4Mmi128 zero;
        P4Mmi128 pk;
        float c255 = 255.0f;
        uint32_t bits255;

        __builtin_memcpy(&bits255, &c255, 4);
        __builtin_memset(&zero, 0, sizeof zero);
        p4_vu0_set_x(2, bits255);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_ftoi(10, P4_VU_XYZW, 10);
        p4_vu0_qmfc2(&g, 10);
        pk = p4_mmi_ppach(zero, g);
        pk = p4_mmi_ppacb(zero, pk);
        packed = pk.w[0];
    }
    rgba.word = packed;
    if (rgba.byte[3] != 255) {
        u8 *target = *(u8 **)(owner + 0x14);
        *(NovaRgba *)(target + 4) = rgba.color;
    } else {
        u8 *target;
        rgba.byte[3] = 254;
        target = *(u8 **)(owner + 0x14);
        *(NovaRgba *)(target + 4) = rgba.color;
        rgba.byte[3] = 255;
    }
    if (*(u8 *)(*(u8 **)(owner + 0x14) + 7) == 0) return;

    cameraDimensions = kwlnGetCamera() + 0x68;
    farClip = *(f32 *)(kwlnGetCamera() + 0x80);
    xScale = *(f32 *)cameraDimensions;
    xExtent = xScale * farClip;
    yScale = *(f32 *)(cameraDimensions + 4);
    yExtent = yScale * farClip;
    minusX = -xExtent;
    corners[0].lane[0] = minusX;
    minusY = -yExtent;
    corners[0].lane[1] = minusY; corners[0].lane[2] = farClip;
    corners[1].lane[0] = xExtent; corners[1].lane[1] = minusY; corners[1].lane[2] = farClip;
    corners[2].lane[0] = xExtent; corners[2].lane[1] = yExtent; corners[2].lane[2] = farClip;
    corners[3].lane[0] = minusX; corners[3].lane[1] = yExtent; corners[3].lane[2] = farClip;
    center.word[0] = 0; center.word[1] = 0; center.vector.lane[2] = farClip;
    mode = *(u8 *)(config + 0x7c);
    switch (mode) {
    case 0: {
        f32 depth, x, y;
        {
            u8 *view = kwlnGetCamera() + 0x20;
            func_003e42a0((u8 *)&interpolated, instance, view);
        }
        depth = interpolated.lane[2];
        x = *(f32 *)cameraDimensions * depth;
        y = *(f32 *)(cameraDimensions + 4) * depth;
        projected.lane[0] = (2.0f * -x) * (interpolated.lane[0] / depth - 0.5f);
        projected.lane[1] = (2.0f * -y) * (interpolated.lane[1] / depth - 0.5f);
        projected.lane[2] = depth;
    } break;
    case 1: {
        f32 x, y;
        f32 half, two;
        farClip = (300.0f * farClip);
        xScale = xScale * farClip;
        yScale = yScale * farClip;
        half = 0.5f;
        two = 2.0f;
        x = (f32)*(s16 *)(config + 0x7e);
        x = (x / 640.0f) - half;
        projected.lane[0] = (two * -xScale * x);
        y = (f32)*(s16 *)(config + 0x80);
        y = (y / 448.0f) - half;
        projected.lane[1] = (two * -yScale * y);
        projected.lane[2] = farClip;
    } break;
    }
    particle = *(u8 **)state;
    count = *(u32 *)(config + 0x38);
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(owner + 0x10) + 0x18), 2);
    vertices = *(f32 **)(*(u8 **)(*(u8 **)(*(u8 **)(owner + 0x10) + 0x18) + 0x5c) + 0x14);
    {
        f32 opacity = (f32)(u32)particle[2] / 255.0f;
        width = *(f32 *)(config + 0x5c) * opacity;
        spread = *(f32 *)(config + 0x60) * opacity;
    }
    index = 0;
    divisor = (f32)65535;
    for (; index < count; index++, particle += 8, vertices += 18) {
        f32 length, factor;
        effectVuLoad10(&corners[index & 3]);
        effectVuLoad11(&corners[(index + 1) & 3]);
        p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);

        /* |vf10.xyz| via the dot-product chain; the decomp reads the sqrt back through Q. */
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
        p4_vu0_sqrt(2, P4_VU_BC_X);
        length = p4pc_vu0.q;
        factor = length * ((f32)(u32)*(u16 *)particle / divisor);

        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        effectVuScale10(factor);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_move(12, P4_VU_XYZW, 10);

        effectVuLoad11(&projected);
        p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_set_x(2, *(u32 *)(particle + 4));
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        effectVuStore10(&interpolated);
        p4_vu0_move(10, P4_VU_XYZW, 12);

        effectVuLoad11(&center.vector);
        p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);

        effectVuLoad11((EffectVuVector *)D_00713CF0);
        p4_vu0_opmula(10, 11);
        p4_vu0_opmsub(10, 11, 10);
        p4_vu0_move(11, P4_VU_XYZW, 10);

        effectVuScale10(spread);
        p4_vu0_sqc2(D_00713D10, 12);
        vertices[12] = *(f32 *)D_00713D10;
        vertices[13] = *(f32 *)D_00713D14;
        vertices[14] = *(f32 *)D_00713D18;
        p4_vu0_add(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
        p4_vu0_sqc2(D_00713D10, 12);
        vertices[9] = *(f32 *)D_00713D10;
        vertices[10] = *(f32 *)D_00713D14;
        vertices[11] = *(f32 *)D_00713D18;
        p4_vu0_sub(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
        p4_vu0_sub(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
        p4_vu0_sqc2(D_00713D10, 12);
        vertices[15] = *(f32 *)D_00713D10;
        vertices[16] = *(f32 *)D_00713D14;
        vertices[17] = *(f32 *)D_00713D18;
        p4_vu0_move(10, P4_VU_XYZW, 11);

        p4_vu0_lqc2(12, &interpolated);
        effectVuScale10(width);
        p4_vu0_sqc2(D_00713D10, 12);
        vertices[3] = *(f32 *)D_00713D10;
        vertices[4] = *(f32 *)D_00713D14;
        vertices[5] = *(f32 *)D_00713D18;
        p4_vu0_add(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
        p4_vu0_sqc2(D_00713D10, 12);
        vertices[0] = *(f32 *)D_00713D10;
        vertices[1] = *(f32 *)D_00713D14;
        vertices[2] = *(f32 *)D_00713D18;
        p4_vu0_sub(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
        p4_vu0_sub(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
        p4_vu0_sqc2(D_00713D10, 12);
        vertices[6] = *(f32 *)D_00713D10;
        vertices[7] = *(f32 *)D_00713D14;
        vertices[8] = *(f32 *)D_00713D18;
    }

    {
        u8 *geometry = *(u8 **)(*(u8 **)(owner + 0x10) + 0x18);
        func_003c22f0(geometry);
        if (*(u16 *)owner & 4) *(u16 *)(geometry + 0xc) |= 1;
    }

    {
        u8 *matrix = func_003e9700(*(u8 **)(kwlnGetCamera() + 4));
        func_003e9cb0(*(u8 **)(owner + 0xc), matrix, 0);
    }
}
