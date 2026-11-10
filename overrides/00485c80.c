#include "p4pc/vu0.h"

u8 *mdlEffectCloneWithNodes(u8 *arg0)
{
    u_long128 sp80;
    u8 *clone;
    u8 *node;
    u8 *nodeClone;
    u8 *src;
    u8 *prim;
    u16 kind;
    void *(**alloc)(u32, u32);

    if (*(u8 **)(arg0 + 0x8C) == NULL) {
        kAssertFail(D_00713470, 0x6B7);
    }
    chkmemAllocSiteTag(D_00713470, 0x546);
    alloc = jtbl_008873E8;
    clone = (u8 *)alloc[0](0x90, 0x40000);
    if (clone == NULL) {
        kAssertFail(D_00713470, 0x547);
    }
    memset(clone, 0, 0x90);
    *(s32 *)(clone + 0x80) = 0;
    *(s32 *)(clone + 0x84) = 0;
    mdlEffectInitParticleState(clone);
    *(u_long128 *)clone = *(u_long128 *)arg0;
    *(u_long128 *)(clone + 0x10) = *(u_long128 *)(arg0 + 0x10);
    *(f32 *)(clone + 0x74) = *(f32 *)(arg0 + 0x74);
    *(s32 *)(clone + 0x68) = *(s32 *)(arg0 + 0x68);
    for (node = *(u8 **)(arg0 + 0x8C); node != NULL; node = *(u8 **)(node + 0xAC)) {
        chkmemAllocSiteTag(D_00713470, 0x559);
        nodeClone = (u8 *)alloc[0](0xC0, 0x40000);
        if (nodeClone == NULL) {
            kAssertFail(D_00713470, 0x55A);
        }
        memset(nodeClone, 0, 0xC0);
        memset(nodeClone, 0, 0x90);
        *(s32 *)(nodeClone + 0x84) = 1;
        *(u8 *)(nodeClone + 0x88) = 8;
        *(u8 *)(nodeClone + 0x89) = 0;
        *(u8 *)(nodeClone + 0x8A) = 0;
        mdlEffectInitParticleState(nodeClone);
        src = *(u8 **)(node + 0x90);
        if (*(u32 *)src > 0xD2) {
            kAssertFail(D_00713470, 0x318);
        }
        kind = *(u16 *)(src + 4);
        chkmemAllocSiteTag(D_00713470, 0x21);
        prim = (u8 *)alloc[0](0x2C, 0x40000);
        if (prim == NULL) {
            kAssertFail(D_00713470, 0x22);
        }
        memset(prim, 0, 0x2C);
        *(s32 *)prim = 0xD2;
        *(u16 *)(prim + 4) = kind;
        *(u16 *)(prim + 0xC) = *(u16 *)(src + 0xC);
        *(u16 *)(prim + 0x1C) = *(u16 *)(src + 0x1C);
        if (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6)) + 0x14) == NULL) {
            kAssertFail(D_00713470, 0x321);
        }
        *(void **)(prim + 8) =
            (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6)) + 0x14))(*(void **)(src + 8));
        *(u8 **)(nodeClone + 0x90) = prim;
        mdlEffectCopyRecord(nodeClone, node);
        *(s32 *)(nodeClone + 0xAC) = 0;
        if (*(u8 **)(clone + 0x88) != NULL) {
            *(u8 **)(*(u8 **)(clone + 0x88) + 0xAC) = nodeClone;
            *(u8 **)(nodeClone + 0xB0) = *(u8 **)(clone + 0x88);
        } else {
            *(u8 **)(clone + 0x8C) = nodeClone;
            *(u8 **)(nodeClone + 0xB0) = NULL;
        }
        *(u8 **)(clone + 0x88) = nodeClone;
        *(s32 *)(clone + 0x80) += 1;
    }
    p4_vu0_sqc2(&sp80, 0);
    mdlEffectSetChildPositions(clone, (f32 *)&sp80);
    mdlEffectSetChildRotations(clone, (u8 *)&sp80);
    mdlEffectSetChildScales(clone, 1.0f);
    mdlEffectApplyPackedColor(clone, -1);
    return clone;
}
