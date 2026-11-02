/* Normal-fusion result scan: retail IDs below 0xC0, then added personas. */
#include "p4pc/game_personas_native.h"
s32 fclFuseLowestCandidateAtLeast(u32 arg0, s32 arg1, u16 *arg2)
{
    extern u8 *DAT_007644c4;
    u16 *input = arg2;
    s32 target = (s32)(arg0 & 0xFF);
    s32 minArcana = arg1 & 0xFF;
    s32 firstPick = 0;
    s32 atLeastPick = 0;
    s32 slot;

    for (slot = 0; p4_game_personas_native_fusion_candidate((u32)(slot & 0xFFFF));
         slot = (s32)p4_game_personas_native_fusion_next((u32)slot)) {
        s32 id = slot & 0xFFFF;
        s32 off = (u16)slot * 0xE;
        u8 *rec = iGpffffb3d4 + off;
        u16 flags;
        s32 eligible = 1;
        s32 k;
        s32 bestRec;
        s32 arcana;
        s32 inInput;

        if (target != rec[2]) {
            continue;
        }
        flags = *(u16 *)rec;
        if (flags & 3) {
            continue;
        }
        for (k = 0; k < 0x17; k++) {
            u8 *ent = D_006420A0 + k * 8;
            s16 tag;

            if (id != *(s16 *)(ent + 4)) {
                continue;
            }
            tag = *(s16 *)(ent + 6);
            if (tag & 1) {
                if (datGetFlag(*(s32 *)ent) != 0) {
                    continue;
                }
                eligible = 0;
                break;
            } else if (tag & 2) {
                eligible = 0;
                if (datGetFlag(*(s32 *)ent) != 1) {
                    continue;
                }
                eligible = 1;
                break;
            }
        }
        if (eligible == 0) {
            continue;
        }

        bestRec = firstPick & 0xFFFF;
        if (bestRec == 0 || (s32)iGpffffb3d4[bestRec * 0xE + 3] < (s32)rec[3]) {
            firstPick = slot;
        }

        arcana = rec[3];
        if (arcana < minArcana) {
            continue;
        }
        inInput = 0;
        for (k = 0; k < 0xC; k++) {
            if (id == input[k]) {
                inInput = 1;
                break;
            }
        }
        if (!inInput && ((atLeastPick & 0xFFFF) == 0 ||
                         arcana < (s32)DAT_007644c4[(atLeastPick & 0xFFFF) * 0xE + 3])) {
            atLeastPick = slot;
        }
    }

    if ((firstPick & 0xFFFF) == 0) {
        return 0;
    }
    if ((atLeastPick & 0xFFFF) == 0) {
        s32 cand = firstPick & 0xFFFF;

        if ((s32)iGpffffb3d4[cand * 0xE + 3] < minArcana) {
            atLeastPick = cand;
        }
    }
    return atLeastPick;
}
