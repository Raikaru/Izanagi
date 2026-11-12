#include "p4pc/vu0.h"

u8 *mdlEffectBuildFromResource(u8 *arg0)
{
    extern void mdlEffectSetChildPositions(u8 *arg0, f32 *arg1);
    extern void mdlEffectSetChildRotations(u8 *arg0, u8 *arg1);
    extern void mdlEffectSetChildScales(u8 *arg0, f32 arg1);
    extern void mdlEffectApplyPackedColor(u8 *arg0, s32 arg1);
    extern char *strcpy(char *destination, const char *source);
    s32 mdlEffectGetChildIndex(u8 *arg0, u8 *arg1);
    u8 *mdlEffectGetChildAtIndex(u8 *arg0, s32 arg1);
    u_long128 vf0quad;
    void *(**alloc)(u32, u32);
    u8 *node;
    u8 *nodeClone;
    u8 *prim;
    u16 kind;
    u8 *found;
    s32 idx;
    u8 *csrc;
    s32 count;
    u8 *base;
    u8 *cur;
    s32 i;
    u8 *slot;
    u8 *target;
    u8 *addr20;
    u8 *addr24;
    s32 idx94;
    u8 *var2;
    s32 tmp;
    u8 *const input = arg0;

    chkmemAllocSiteTag(D_00713470, 0x546);
    alloc = jtbl_008873E8;
    {
        u8 *const clone = (u8 *)alloc[0](0x90, 0x40000);
        if (clone == NULL) {
            kAssertFail(D_00713470, 0x547);
        }
        memset(clone, 0, 0x90);
        *(s32 *)(clone + 0x80) = 0;
        *(s32 *)(clone + 0x84) = 0;
        mdlEffectInitParticleState(clone);
        /* Retail initializes both vectors from the first input quad. */
        *(u_long128 *)clone = *(u_long128 *)input;
        *(u_long128 *)(clone + 0x10) = *(u_long128 *)input;
        *(f32 *)(clone + 0x74) = *(f32 *)(input + 0x74);
        *(s32 *)(clone + 0x68) = *(s32 *)(input + 0x68);
        if (*(u8 **)(input + 0x8C) != NULL) {
            u8 *linkedSource;
            for (node = *(u8 **)(input + 0x8C); node != NULL; node = *(u8 **)(node + 0xAC)) {
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
                if ((*(s32 *)(node + 0x98) & 1) == 0) {
                    linkedSource = *(u8 **)(node + 0x90);
                    if (*(u32 *)linkedSource > 0xD2) {
                        kAssertFail(D_00713470, 0x2D7);
                    }
                    kind = *(u16 *)(linkedSource + 4);
                    chkmemAllocSiteTag(D_00713470, 0x21);
                    prim = (u8 *)alloc[0](0x2C, 0x40000);
                    if (prim == NULL) {
                        kAssertFail(D_00713470, 0x22);
                    }
                    memset(prim, 0, 0x2C);
                    *(s32 *)prim = 0xD2;
                    *(u16 *)(prim + 4) = kind;
                    *(u16 *)(prim + 0xC) = *(u16 *)(linkedSource + 0xC);
                    *(u16 *)(prim + 0x1C) = *(u16 *)(linkedSource + 0x1C);
                    if (*(u16 *)(prim + 4) >= 0x21U) {
                        kAssertFail(D_00713470, 0x2DD);
                    }
                    if (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6))) == NULL) {
                        kAssertFail(D_00713470, 0x2DE);
                    }
                    *(void **)(prim + 8) =
                        (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6))))(linkedSource);
                    *(u8 **)(nodeClone + 0x90) = prim;
                } else {
                    found = mdlEffectFindNodeById(input, *(s32 *)(node + 0x90));
                    idx = mdlEffectGetChildIndex(input, found);
                    csrc = *(u8 **)(mdlEffectGetChildAtIndex(clone, idx) + 0x90);
                    if (*(u32 *)csrc > 0xD2) {
                        kAssertFail(D_00713470, 0x318);
                    }
                    kind = *(u16 *)(csrc + 4);
                    chkmemAllocSiteTag(D_00713470, 0x21);
                    prim = (u8 *)alloc[0](0x2C, 0x40000);
                    if (prim == NULL) {
                        kAssertFail(D_00713470, 0x22);
                    }
                    memset(prim, 0, 0x2C);
                    *(s32 *)prim = 0xD2;
                    *(u16 *)(prim + 4) = kind;
                    *(u16 *)(prim + 0xC) = *(u16 *)(csrc + 0xC);
                    *(u16 *)(prim + 0x1C) = *(u16 *)(csrc + 0x1C);
                    if (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6)) + 0x14) == NULL) {
                        kAssertFail(D_00713470, 0x321);
                    }
                    *(void **)(prim + 8) =
                        (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6)) + 0x14))(*(void **)(csrc + 8));
                    *(u8 **)(nodeClone + 0x90) = prim;
                }
                mdlEffectCopyRecord(nodeClone, node);
                strcpy((char *)nodeClone + 0x9C, (const char *)node + 0x9C);
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
        } else {
            u8 *indexedSource;
            count = *(s32 *)(input + 0x80);
            base = input + *(s32 *)(input + 0x88);
            cur = base;
            i = 0;
            goto indexed_check;
    indexed_body:
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
            tmp = *(s32 *)(cur + 0x98);
            if ((tmp & 1) == 0) {
                indexedSource = input + *(s32 *)(cur + 0x90);
                if ((tmp & 2) != 0) {
                    addr24 = indexedSource + 0x24;
                    if (*(s32 *)addr24 != 0) {
                        kAssertFail(D_00713470, 0x5BB);
                    }
                    addr20 = indexedSource + 0x20;
                    if (*(s32 *)addr20 != 0) {
                        kAssertFail(D_00713470, 0x5BC);
                    }
                    idx94 = *(s32 *)(cur + 0x94);
                    slot = base + idx94 * 0xC0;
                    target = input + *(s32 *)(slot + 0x90);
                    if (*(s32 *)(target + 0x24) == 0) {
                        kAssertFail(D_00713470, 0x5BE);
                    }
                    *(s32 *)addr24 = *(s32 *)(target + 0x24);
                    if (*(s32 *)(target + 0x28) != 0) {
                        var2 = *(u8 **)(target + 0x20);
                    } else if (*(s32 *)(target + 0x20) != 0) {
                        var2 = target + *(s32 *)(target + 0x20);
                    } else {
                        var2 = NULL;
                    }
                    *(u32 *)addr20 = (u32)var2 - (u32)indexedSource;
                    if (*(u32 *)indexedSource > 0xD2) {
                        kAssertFail(D_00713470, 0x2D7);
                    }
                    kind = *(u16 *)(indexedSource + 4);
                    chkmemAllocSiteTag(D_00713470, 0x21);
                    prim = (u8 *)alloc[0](0x2C, 0x40000);
                    if (prim == NULL) {
                        kAssertFail(D_00713470, 0x22);
                    }
                    memset(prim, 0, 0x2C);
                    *(s32 *)prim = 0xD2;
                    *(u16 *)(prim + 4) = kind;
                    *(u16 *)(prim + 0xC) = *(u16 *)(indexedSource + 0xC);
                    *(u16 *)(prim + 0x1C) = *(u16 *)(indexedSource + 0x1C);
                    if (*(u16 *)(prim + 4) >= 0x21U) {
                        kAssertFail(D_00713470, 0x2DD);
                    }
                    if (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6))) == NULL) {
                        kAssertFail(D_00713470, 0x2DE);
                    }
                    *(void **)(prim + 8) =
                        (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6))))(indexedSource);
                    *(u8 **)(nodeClone + 0x90) = prim;
                    *(s32 *)addr24 = 0;
                    *(s32 *)addr20 = 0;
                } else {
                    if (*(u32 *)indexedSource > 0xD2) {
                        kAssertFail(D_00713470, 0x2D7);
                    }
                    kind = *(u16 *)(indexedSource + 4);
                    chkmemAllocSiteTag(D_00713470, 0x21);
                    prim = (u8 *)alloc[0](0x2C, 0x40000);
                    if (prim == NULL) {
                        kAssertFail(D_00713470, 0x22);
                    }
                    memset(prim, 0, 0x2C);
                    *(s32 *)prim = 0xD2;
                    *(u16 *)(prim + 4) = kind;
                    *(u16 *)(prim + 0xC) = *(u16 *)(indexedSource + 0xC);
                    *(u16 *)(prim + 0x1C) = *(u16 *)(indexedSource + 0x1C);
                    if (*(u16 *)(prim + 4) >= 0x21U) {
                        kAssertFail(D_00713470, 0x2DD);
                    }
                    if (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6))) == NULL) {
                        kAssertFail(D_00713470, 0x2DE);
                    }
                    *(void **)(prim + 8) =
                        (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6))))(indexedSource);
                    *(u8 **)(nodeClone + 0x90) = prim;
                }
            } else {
                csrc = *(u8 **)(mdlEffectGetChildAtIndex(clone, *(s32 *)(cur + 0x90)) + 0x90);
                if (*(u32 *)csrc > 0xD2) {
                    kAssertFail(D_00713470, 0x318);
                }
                kind = *(u16 *)(csrc + 4);
                chkmemAllocSiteTag(D_00713470, 0x21);
                prim = (u8 *)alloc[0](0x2C, 0x40000);
                if (prim == NULL) {
                    kAssertFail(D_00713470, 0x22);
                }
                memset(prim, 0, 0x2C);
                *(s32 *)prim = 0xD2;
                *(u16 *)(prim + 4) = kind;
                *(u16 *)(prim + 0xC) = *(u16 *)(csrc + 0xC);
                *(u16 *)(prim + 0x1C) = *(u16 *)(csrc + 0x1C);
                if (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6)) + 0x14) == NULL) {
                    kAssertFail(D_00713470, 0x321);
                }
                *(void **)(prim + 8) =
                    (*(EffectInstanceConstructor *)(((u8 *)D_00713480 + (*(u16 *)(prim + 4) << 6)) + 0x14))(*(void **)(csrc + 8));
                *(u8 **)(nodeClone + 0x90) = prim;
            }
            mdlEffectCopyRecord(nodeClone, cur);
            strcpy((char *)nodeClone + 0x9C, (const char *)cur + 0x9C);
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
            cur += 0xC0;
            i += 1;
    indexed_check:
            if (i < count) {
                goto indexed_body;
            }
        }
        /* VU0's fixed vector is (0, 0, 0, 1). */
        p4_vu0_sqc2(&vf0quad, 0);
        mdlEffectSetChildPositions(clone, (f32 *)&vf0quad);
        mdlEffectSetChildRotations(clone, (u8 *)&vf0quad);
        mdlEffectSetChildScales(clone, 1.0f);
        mdlEffectApplyPackedColor(clone, -1);
        return clone;
    }
}
