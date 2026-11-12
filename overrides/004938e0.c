#include "p4pc/vu0.h"

void TrackUpdateSlots(u8 *arg0)
{
    extern f32 fGpffff8044;
    extern void ParticleUpdateScaled(void *arg0, f32 arg1);
    s32 sp9C;
    s32 sp98;
    s32 sp94;
    u32 packed;
    f32 sp80[3];
    f32 scale;
    u8 *track;
    u8 *entry;
    s32 i;
    u8 *owner;
    s32 count;
    s32 mode;

    if (*(s32 *)arg0 == 0) {
        return;
    }
    mdlEffectAdvanceEmitter(*(void **)(arg0 + 0x30));
    track = *(u8 **)(arg0 + 0x30);
    sp98 = *(s32 *)(arg0 + 4);
    scale = fGpffff8044;
    entry = *(u8 **)(track + 0x18);
    count = *(s32 *)arg0;
    owner = *(u8 **)(arg0 + 0x28);
    mdlEffectCopyOriginAndLockGeometry(owner);
    for (i = 0; i < count; i++, entry += 0x20) {
        if (mdlEffectSlotIsFree(owner, i) != 0) {
            continue;
        }
        mode = *(s32 *)(entry + 0x10);
        if (mode == -2) {
            mdlEffectAdvanceRibbon(owner, i, D_00922D80, 0.0f);
            mdlEffectSetRibbonMaterialColor(owner, i, &iGpffffbb64);
            mdlEffectSlotRelease(owner, i);
            continue;
        }
        if (mode == -1) {
            goto clear_entry;
        }
        if (mode != 0) {
            goto draw_entry;
        }
clear_entry:
        mdlEffectAdvanceRibbon(owner, i, D_00922D80, 0.0f);
        mdlEffectSetRibbonMaterialColor(owner, i, &iGpffffbb64);
        mdlEffectSlotSetZero(owner, i);
        continue;
draw_entry:
        sp80[0] = *(f32 *)(entry + 0);
        sp80[1] = *(f32 *)(entry + 4);
        sp80[2] = *(f32 *)(entry + 8);
        mdlEffectAdvanceRibbon(owner, i, sp80, *(f32 *)(entry + 0x18));
        sp94 = *(s32 *)(entry + 0x14);
        /* Retail packs (track colour * scale) * (entry colour * scale) * 255 through
         * the COP2 blur pipeline; the helper reproduces that and returns the low word. */
        packed = p4_vu0_packed_color_scale2((uint32_t)sp98, (uint32_t)sp94, scale);
        sp9C = (s32)packed;
        mdlEffectSetRibbonMaterialColor(owner, i, &sp9C);
    }
    mdlEffectUnlockGeometryPair(owner);
    if (*(u8 **)(arg0 + 0x2C) == NULL) {
        return;
    }
    if (*(u32 *)(*(u8 **)(arg0 + 0x30) + 0x10) < *(u32 *)(arg0 + 0x10)) {
        return;
    }
    mdlSetObjectHeaderValue(*(void **)(arg0 + 0x2C), *(u32 *)(arg0 + 4));
    ParticleUpdateScaled(*(void **)(arg0 + 0x2C), *(f32 *)(arg0 + 0x24) / 10.0f);
}
