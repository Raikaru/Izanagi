#include "p4pc/game_mode.h"
#include "p4pc/golden_fusion_runtime.h"
u8 fclFuseTripleArcana(u8 *firstPersona, u8 *secondPersona, u8 *thirdPersona)
{
    s8 pairs[3][2];
    u8 *personas[4];
    u8 *clear;
    s32 remaining;
    s8 *source;
    s8 *destination;
    s32 copies;
    s8 firstByte;
    s8 secondByte;
    s16 pairIndex;
    u8 *base;
    s8 *pair;
    u8 **leftSlot;
    u8 **rightSlot;
    u8 *right;
    u8 *left;
    s32 leftLevel;
    s32 rightLevel;
    s32 firstArcana;
    u8 secondArcana;
    u8 thirdArcana;
    s32 result;
    s32 secondKey;
    s32 thirdKey;
    u8 resultByte;

    clear = (u8 *)personas;
    remaining = sizeof(personas);
    if (clear != NULL) {
        do {
            *clear++ = 0;
            remaining--;
        } while (remaining != 0);
    }
    personas[0] = firstPersona;
    personas[1] = secondPersona;
    personas[2] = thirdPersona;
    source = iGpffffa8d0;
    destination = (s8 *)&pairs;
    copies = 3;
    do {
        firstByte = source[0];
        secondByte = source[1];
        source += 2;
        copies--;
        destination[0] = firstByte;
        destination[1] = secondByte;
        destination += 2;
    } while (copies > 0);
    if (firstPersona == NULL || secondPersona == NULL || thirdPersona == NULL)
        kAssertFail(D_00642F30, 0x156);
    pairIndex = 0;
    base = iGpffffb3d4;
    while (pairIndex < 3) {
        pair = pairs[pairIndex];
        rightSlot = &personas[pair[1]];
        right = *rightSlot;
        rightLevel = ((DatPersonaRecord *)right)->level;
        leftSlot = &personas[pair[0]];
        left = *leftSlot;
        leftLevel = ((DatPersonaRecord *)left)->level;
        if (leftLevel < rightLevel ||
            (leftLevel == rightLevel &&
             *((u8 *)((u32)base + ((DatPersonaRecord *)left)->id * 14) + 2) > *((u8 *)((u32)base + ((DatPersonaRecord *)right)->id * 14) + 2))) {
            pair = pairs[pairIndex];
            leftSlot = &personas[pair[0]];
            personas[3] = left;
            rightSlot = &personas[pair[1]];
            *leftSlot = *rightSlot;
            *rightSlot = personas[3];
        }
        pairIndex++;
    }
    base = iGpffffb3d4 + 2;
    firstArcana = base[((DatPersonaRecord *)personas[0])->id * 14];
    secondArcana = base[((DatPersonaRecord *)personas[1])->id * 14];
    thirdArcana = base[((DatPersonaRecord *)personas[2])->id * 14];
    if (firstArcana == 0 || secondArcana == 0 || thirdArcana == 0)
        kAssertFail(D_00642F30, 0x167);
    secondKey = secondArcana;
    if (secondKey == 0 || thirdArcana == 0)
        kAssertFail(D_00642F30, 0x90);
    thirdKey = thirdArcana;
    if (p4_game_mode() == P4_GAME_MODE_GOLDEN)
        resultByte = (u8)p4_golden_fusion_chart(1, secondKey, thirdKey);
    else if (secondKey < thirdKey)
        resultByte = *(thirdKey + (D_00642727 + secondKey * 24));
    else
        resultByte = *(secondKey + (D_00642727 + thirdKey * 24));
    result = resultByte;
    if (result == 0)
        return 0;
    return fclTriangleUpperArcana(firstArcana, result);
}