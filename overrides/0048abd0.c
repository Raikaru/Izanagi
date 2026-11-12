#include "p4pc/vu0.h"

s32 mdlEffectSamplePackedColor(u8 *colorData, u8 *opacityData, s32 frame, s32 duration)
{
    const EffectPackedColorKeys *colors = (const EffectPackedColorKeys *)colorData;
    const EffectPackedOpacityKeys *opacityKeys = (const EffectPackedOpacityKeys *)opacityData;
    s32 secondColor;
    s32 firstColor;
    s32 packedRgb;
    s32 mixedColor;
    f32 normalization;
    f32 blend;
    f32 durationFloat;
    f32 scaledAlpha;
    f32 alphaFloat;
    f32 opacity;
    s32 baseAlpha;
    s32 alpha;
    s32 endColor;
    s32 startColor;
    u8 mode;
    f32 inverseBlend;
    f32 segmentSpan;
    s32 endFrame;
    s32 startFrame;
    P4Mmi128 lanes;
    P4Mmi128 zero;
    P4Mmi128 r;

    if (duration == 0) {
        return (s32)((colors->startColor & 0xFFFFFFU) |
                     (opacityKeys->alpha << 24));
    }
    durationFloat = (f32)duration;
    mode = colors->mode;
    switch (mode) {
    case 0:
        startColor = (s32)colors->startColor;
        endColor = (s32)colors->endColor;
        blend = (f32)frame / durationFloat;
        break;
    case 1:
        endFrame = (s32)(colors->firstFraction * durationFloat);
        if (frame < endFrame) {
            startColor = (s32)colors->startColor;
            endColor = (s32)colors->firstMiddleColor;
            blend = (f32)frame / (f32)endFrame;
        } else {
            startColor = (s32)colors->firstMiddleColor;
            endColor = (s32)colors->endColor;
            blend = (segmentSpan = (f32)(duration - endFrame), (f32)(frame - endFrame) / segmentSpan);
        }
        break;
    case 2:
        startFrame = (s32)(colors->firstFraction * durationFloat);
        if (frame < startFrame) {
            startColor = (s32)colors->startColor;
            endColor = (s32)colors->firstMiddleColor;
            blend = (f32)frame / (f32)startFrame;
        } else {
            endFrame = (s32)(colors->secondFraction * durationFloat);
            if (frame < endFrame) {
                startColor = (s32)colors->firstMiddleColor;
                endColor = (s32)colors->secondMiddleColor;
                blend = (segmentSpan = (f32)(endFrame - startFrame), (f32)(frame - startFrame) / segmentSpan);
            } else {
                startColor = (s32)colors->secondMiddleColor;
                endColor = (s32)colors->endColor;
                blend = (segmentSpan = (f32)(duration - endFrame), (f32)(frame - endFrame) / segmentSpan);
            }
        }
        break;
    default:
        startColor = (s32)colors->startColor;
        endColor = (s32)colors->endColor;
        blend = 0.0f;
        break;
    }

    __builtin_memset(&zero, 0, sizeof zero);
    normalization = fGpffff8044;

    /* End colour: bytes -> int lanes -> float -> scaled by 1/255. Saved to VF11. */
    secondColor = endColor;
    lanes = p4_vu0_word_bytes_to_lanes((u32)secondColor);
    p4_vu0_qmtc2(10, &lanes);
    p4_vu0_itof(10, P4_VU_XYZW, 10);
    p4_vu0_set_xf(2, normalization);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_move(11, P4_VU_XYZW, 10);

    /* Start colour: same normalisation, into VF10. */
    firstColor = startColor;
    lanes = p4_vu0_word_bytes_to_lanes((u32)firstColor);
    p4_vu0_qmtc2(10, &lanes);
    p4_vu0_itof(10, P4_VU_XYZW, 10);
    p4_vu0_set_xf(2, normalization);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);

    opacity = 1.0f; /* Full alpha between the two independently configured ramps. */
    inverseBlend = opacity - blend;
    p4_vu0_set_xf(2, inverseBlend);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_set_xf(2, blend);
    p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);

    /* Scale to 0..255, truncate to ints, pack the four low bytes into one word. */
    p4_vu0_set_x(2, 0x437F0000U); /* 255.0f */
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_ftoi(10, P4_VU_XYZW, 10);
    p4_vu0_qmfc2(&r, 10);
    r = p4_mmi_ppach(zero, r);
    r = p4_mmi_ppacb(zero, r);
    packedRgb = (s32)r.w[0];

    mixedColor = packedRgb;
    {
        s32 fadeIn = (s32)(opacityKeys->fadeInFraction * durationFloat);
        s32 fadeOut = (s32)(opacityKeys->fadeOutFraction * durationFloat);
        opacity = mdlEffectColorOpacity(frame, duration, fadeIn, fadeOut, opacity);
    }
    baseAlpha = (s32)opacityKeys->alpha;
    alphaFloat = (f32)(u32)baseAlpha;
    scaledAlpha = alphaFloat * opacity;
    alpha = (s32)(u32)scaledAlpha;
    return (s32)(((u32)mixedColor & 0xFFFFFFU) | ((u32)alpha << 24));
}
