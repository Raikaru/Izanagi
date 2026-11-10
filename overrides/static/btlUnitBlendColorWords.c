#include "p4pc/vu0.h"

static inline u32 btlUnitBlendColorWords(const u32 *start, const u32 *target, f32 factor)
{
    u32 targetColor;
    u32 startColor;
    u32 packedColor;
    const u32 *source;
    f32 scale;
    f32 inverse;
    P4Mmi128 lanes;

    targetColor = *target;
    source = &targetColor;
    scale = fGpffff81f4;
    lanes = p4_vu0_word_bytes_to_lanes(*source);
    p4_vu0_qmtc2(11, &lanes);
    p4_vu0_itof(11, P4_VU_XYZW, 11);
    p4_vu0_set_xf(2, scale);
    p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);

    startColor = *start;
    source = &startColor;
    lanes = p4_vu0_word_bytes_to_lanes(*source);
    p4_vu0_qmtc2(10, &lanes);
    p4_vu0_itof(10, P4_VU_XYZW, 10);
    p4_vu0_set_xf(2, scale);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);

    inverse = 1.0f - factor;
    p4_vu0_set_xf(2, inverse);
    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
    p4_vu0_set_xf(2, factor);
    p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);

    packedColor = p4_vu0_pack_vf10_255();
    return packedColor;
}
