#include "p4pc/vu0.h"

void LineNovaUpdateVertices(u8 *unused, u8 *effect)
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

    extern u8 *RwMatrixMultiply(u8 *, u8 *, u8 *);
    extern u8 *RwMatrixRotate(u8 *, const u8 *, f32, s32);
    extern u8 D_00714638[], D_00714640[];
    #pragma push
    #pragma pack(4)
    typedef struct { s64 xy; f32 z; } NovaAxis;
    #pragma pop
    NovaAxis axis;
    s64 axisXY;
    f32 axisZ;
    EffectVuVector top, bottom;
    u8 rotation[64] __attribute__((aligned(16)));
    u8 transform[64] __attribute__((aligned(16)));
    f32 centerX, centerY, depth, extentX, extentY;
    f32 width, spread, screenX, screenY;
    u32 progress, duration;
    u8 *matrix;
    axisXY = *(s64 *)D_00714638;
    axisZ = *(f32 *)D_00714640;
    axis.xy = axisXY;
    axis.z = axisZ;

    instance = effect;
    state = *(u8 **)(instance + 0x20);
    config = *(u8 **)(instance + 0x24);
    owner = *(u8 **)(state + 4);

    progress = *(u32 *)(instance + 0x14);

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

    duration = *(u32 *)(config + 0x7c);
    if (duration != 0) {
        if (progress < duration) {
            f32 time = (f32)progress / (f32)duration;
            centerX = 0.0f + (f32)*(s16 *)(config + 0x80) + time * (f32)(*(s16 *)(config + 0x88) - *(s16 *)(config + 0x80));
            centerY = 0.0f + (f32)*(s16 *)(config + 0x82) + time * (f32)(*(s16 *)(config + 0x8a) - *(s16 *)(config + 0x82));
            extentX = 0.0f + (f32)*(s16 *)(config + 0x84) + time * (f32)(*(s16 *)(config + 0x8c) - *(s16 *)(config + 0x84));
            extentY = 0.0f + (f32)*(s16 *)(config + 0x86) + time * (f32)(*(s16 *)(config + 0x8e) - *(s16 *)(config + 0x86));
        } else {
            centerX = (f32)*(s16 *)(config + 0x88);
            centerY = (f32)*(s16 *)(config + 0x8a);
            extentX = (f32)*(s16 *)(config + 0x8c);
            extentY = (f32)*(s16 *)(config + 0x8e);
        }
    } else {
        centerX = (f32)*(s16 *)(config + 0x80);
        centerY = (f32)*(s16 *)(config + 0x82);
        extentX = (f32)*(s16 *)(config + 0x84);
        extentY = (f32)*(s16 *)(config + 0x86);
    }
    cameraDimensions = kwlnGetCamera() + 0x68;
    depth = *(f32 *)(kwlnGetCamera() + 0x80) + 1.0f;
    screenX = 640.0f;
    {
        f32 x = extentX * (*(f32 *)cameraDimensions * depth) / screenX;
        f32 y;
        screenY = 448.0f;
        y = extentY * (*(f32 *)(cameraDimensions + 4) * depth) / screenY;
        bottom.lane[0] = -x; bottom.lane[1] = -y; bottom.lane[2] = depth;
        top.lane[0] = -x; top.lane[1] = y; top.lane[2] = depth;
        extentX = y * 2.0f;
        extentY = x * 2.0f;
    }
    particle = *(u8 **)state;
    count = *(u32 *)(config + 0x38);
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(owner + 0x10) + 0x18), 2);
    vertices = *(f32 **)(*(u8 **)(*(u8 **)(*(u8 **)(owner + 0x10) + 0x18) + 0x5c) + 0x14);
    width = *(f32 *)(config + 0x5c) / 10.0f;
    spread = *(f32 *)(config + 0x60) / 10.0f;
    index = 0;
    for (; index < count; index++, particle += 6, vertices += 18) {
        const f32 divisor = (f32)65535;
        f32 opacity = (f32)(u32)*(u16 *)particle / divisor;
        f32 horizontalWidth = width * opacity;
        f32 verticalWidth = spread * opacity;
        f32 horizontalPosition = extentY * (f32)(u32)*(u16 *)(particle + 4) / divisor;
        f32 verticalPosition;
        effectVuLoad10(&bottom);
        effectVuLoad11(&top);
        p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        /* Normalise vf10.xyz (see 004b36b0 for the same sequence). */
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        p4_vu0_move(12, P4_VU_XYZW, 10);

        verticalPosition = extentX * (f32)(u32)*(u16 *)(particle + 2) / divisor;
        effectVuScale10(verticalPosition);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_sqc2(D_00713D10, 10);
        vertices[12] = *(f32 *)D_00713D10;
        vertices[13] = *(f32 *)D_00713D14;
        vertices[14] = *(f32 *)D_00713D18;
        p4_vu0_sqc2(D_00713D10, 10);
        vertices[9] = *(f32 *)D_00713D10;
        vertices[10] = *(f32 *)D_00713D14;
        vertices[11] = *(f32 *)D_00713D18;
        p4_vu0_sqc2(D_00713D10, 10);
        vertices[15] = *(f32 *)D_00713D10;
        vertices[16] = *(f32 *)D_00713D14;
        vertices[17] = *(f32 *)D_00713D18;

        vertices[1] = vertices[10] - verticalWidth;
        vertices[7] = vertices[16] + verticalWidth;
        vertices[10] = vertices[10] - horizontalWidth;
        vertices[16] = vertices[16] + horizontalWidth;
        vertices[0] = vertices[9] + horizontalPosition;
        vertices[2] = vertices[11];
        vertices[3] = vertices[12] + horizontalPosition;
        vertices[4] = vertices[13]; vertices[5] = vertices[14];
        vertices[6] = vertices[15] + horizontalPosition;
        vertices[8] = vertices[17];
    }

    {
        u8 *geometry = *(u8 **)(*(u8 **)(owner + 0x10) + 0x18);
        func_003c22f0(geometry);
        if (*(u16 *)owner & 4) *(u16 *)(geometry + 0xc) |= 1;
    }

    matrix = func_003e9700(*(u8 **)(kwlnGetCamera() + 4));
    RwMatrixRotate(rotation, (const u8 *)&axis, (f32)*(s16 *)(config + 0x90), 0);
    *(f32 *)(rotation + 0x30) += novaCameraOffset(centerX, (const f32 *)cameraDimensions, depth, screenX, 320.0f);
    {
        f32 factor = 2.0f * *(const f32 *)(cameraDimensions + 4);
        factor = factor * depth;
        *(f32 *)(rotation + 0x34) += (224.0f - centerY) * factor / screenY;
    }
    RwMatrixMultiply(transform, rotation, matrix);
    func_003e9cb0(*(u8 **)(owner + 0xc), transform, 0);
}
