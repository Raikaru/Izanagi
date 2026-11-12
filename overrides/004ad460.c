#include "p4pc/vu0.h"

void *EffHelperCreate(void *arg0)
{
    void *(*const *allocSlot)(u32, u32);
    EffHlpWork *work;
    s32 v20;
    s32 v;
    u8 *tex;
    u32 i;

    chkmemAllocSiteTag(D_007144F8, 0x1D);
    allocSlot = jtbl_008873E8;
    work = (EffHlpWork *)(*allocSlot)(0xA4, 0x40000);
    if (work == NULL) {
        kAssertFail(D_007144F8, 0x1E);
    }
    memset(work, 0, 0xA4);
    work->lastTex = -1;
    work->alpha = 1.0f;
    p4_vu0_sqc2(work, 0);
    p4_vu0_sqc2((u8 *)work + 16, 0);
    if (arg0 == NULL) {
        return work;
    }
    tex = mdlEffectResolveRelPtrA(arg0);
    if (tex == NULL) {
        kAssertFail(D_007144F8, 0x2E);
    }
    switch (*(u16 *)((u8 *)arg0 + 0xC)) {
    case 0:
        v20 = 0x70;
        memcpy((u8 *)work + 0x30, tex, 0x68);
        break;
    default:
        kAssertFail(D_007144F8, 0x38);
        v20 = 0;
        break;
    }
    work->res = (s32)mdlEffectCreateUniqueModel(tex + v20, *(s32 *)((u8 *)arg0 + 0x14) - v20);
    if (work->res == 0) {
        kAssertFail(D_007144F8, 0x40);
    }
    if (mdlLayerIsPresent(work->res, 0) != 0) {
        if (work->data.mode != 0) {
            mdlDispatchAnimIfPresent((u8*)work->res, 0, 0, 0, 0);
        } else {
            mdlDispatchAnimIfPresent((u8*)work->res, 0, 0, 0, 1);
        }
    }
    work->count = mdlCountIntUserData(work->res);
    if (work->count == 0) {
        return work;
    }
    v = (s32)mdlEffectResolveRelPtrB(arg0);
    if (v != 0) {
        if (*(u16 *)((u8 *)arg0 + 0x1C) != 6) {
            kAssertFail(D_007144F8, 0x53);
        }
        {
            u32 bytes = work->count * 4;
            chkmemAllocSiteTag(D_007144F8, 0x57);
            work->listAlloc = (*allocSlot)(bytes, 0x40000);
        }
        if (work->listAlloc == NULL) {
            kAssertFail(D_007144F8, 0x58);
        }
        work->list = work->listAlloc;
        work->list[0] = (u32)mdlEffectBuildFromResource((u8 *)v);
        for (i = 1; i < work->count; i++) {
            work->list[i] = (u32)mdlEffectCloneWithNodes((u8 *)work->list[0]);
        }
    }
    return work;
}
