#include "p4pc/vu0.h"

void mdlEffectInterpolateRibbonColors(u8 *track, u32 *colors)
{
    extern u8 *func_003c22f0(u8 *geometry);
    extern f32 fGpffff8044;
    u32 firstWord;
    u32 secondWord;
    u32 thirdWord;
    u32 fourthWord;
    u32 firstPacked;
    u32 secondPacked;
    EffectVuVector first;
    EffectVuVector third;
    EffectVuVector second;
    EffectVuVector fourth;
    u8 *work;
    u32 rowCount;
    u32 vertexCount;
    u32 segmentCount;
    u8 *destination;
    u8 *firstRow;
    f32 fraction;
    f32 fractionStep;
    f32 scale;
    f32 inverse;
    u32 index;
    u32 rowBytes;
    u32 rowIndex;
    u8 *geometry;
    u8 *cap;
    u8 *capColors;
    u8 *nextCapRow;
    u32 capRowIndex;

    work = *(u8 **)(track + 0x10);
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(work + 0x10) + 0x18), 8);
    rowCount = *(s16 *)(work + 0x48);
    vertexCount = *(s16 *)(work + 8);
    segmentCount = vertexCount / 3U;
    destination = *(u8 **)(*(u8 **)(*(u8 **)(work + 0x10) + 0x18) + 0x30);
    firstRow = destination;
    fractionStep = 1.0f / (f32)segmentCount;
    fraction = 0.0f;
    scale = fGpffff8044;

    firstWord = colors[0];
    effectVuUnpackColor10V0(&firstWord, scale);
    effectVuStore10(&first);
    secondWord = colors[1];
    effectVuUnpackColor10V0(&secondWord, scale);
    effectVuStore10(&second);
    thirdWord = colors[2];
    effectVuUnpackColor10V0(&thirdWord, scale);
    effectVuStore10(&third);
    fourthWord = colors[3];
    effectVuUnpackColor10V0(&fourthWord, scale);
    effectVuStore10(&fourth);

    /* Each segment lerps the ribbon's first/third and second/fourth colours, then packs them back to bytes. */
    for (index = 0; index < segmentCount; index++) {
        effectVuLoad11(&first);
        effectVuLoad10(&third);
        effectVuScale10(fraction);
        inverse = 1.0f - fraction;
        effectVuScale11(inverse);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        firstPacked = p4_vu0_pack_vf10_255();
        *(u32 *)(destination + 4) = firstPacked;

        effectVuLoad11(&second);
        effectVuLoad10(&fourth);
        effectVuScale10(fraction);
        effectVuScale11(inverse);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        secondPacked = p4_vu0_pack_vf10_255();
        *(u32 *)destination = secondPacked;
        *(u32 *)(destination + 8) = *(u32 *)destination;
        fraction += fractionStep;
        destination += 12;
    }

    rowBytes = vertexCount * 4;
    for (rowIndex = 1; rowIndex < rowCount; rowIndex++) {
        memcpy(destination, firstRow, rowBytes);
        destination += rowBytes;
    }
    geometry = *(u8 **)(*(u8 **)(work + 0x10) + 0x18);
    func_003c22f0(geometry);
    if (*(u16 *)work & 4) {
        *(u16 *)(geometry + 0xC) |= 1;
    }

    /* Cap: first two colours, then the second colour repeated across the cap row, then cloned rows. */
    cap = *(u8 **)(track + 0x14);
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(cap + 0x10) + 0x18), 8);
    capColors = *(u8 **)(*(u8 **)(*(u8 **)(cap + 0x10) + 0x18) + 0x30);
    *(u32 *)capColors = colors[0];
    *(u32 *)(capColors + 4) = colors[1];
    *(u32 *)(capColors + 8) = *(u32 *)(capColors + 4);
    *(u32 *)(capColors + 12) = *(u32 *)(capColors + 4);
    *(u32 *)(capColors + 16) = *(u32 *)(capColors + 4);
    *(u32 *)(capColors + 20) = *(u32 *)(capColors + 4);
    *(u32 *)(capColors + 24) = *(u32 *)(capColors + 4);
    *(u32 *)(capColors + 28) = *(u32 *)(capColors + 4);
    nextCapRow = capColors + 32;
    for (capRowIndex = 1; capRowIndex < rowCount; capRowIndex++) {
        memcpy(nextCapRow, capColors, 32);
        nextCapRow += 32;
    }
    geometry = *(u8 **)(*(u8 **)(cap + 0x10) + 0x18);
    func_003c22f0(geometry);
    if (*(u16 *)cap & 4) {
        *(u16 *)(geometry + 0xC) |= 1;
    }
}
