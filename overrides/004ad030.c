#include "p4pc/vu0.h"

/* Samples a packed colour with func_0048abd0, applies it to the model object at +0xB4, sets alpha-blend flags from +0xA0/+0xA4, sets the layer weight from +0x08, attaches the owner and queues the model for draw. */
void EffModelDrawableUpdate(s32 arg0, u8 *arg1)
{
    extern f32 mdlGetLayerWeight(s32 arg0, s32 arg1);
    extern s32 mdlEffectSamplePackedColor(u8 *arg0, u8 *arg1, s32 arg2, s32 arg3);
    extern void mdlSetColor(void *arg0, s32 *arg1);
    extern void mdlSetLayerWeight(void *arg0, s32 arg1, f32 arg2);
    extern s32 TnGetTableBase(void);
    extern void mdlSetAttachedOwner(void *arg0, s32 arg1);
    extern void mdlQueueModelForDraw(s32 arg0, void *arg1);
    extern f32 D_00761134;
    s32 sp5C;
    s32 sp58;
    s32 sp54;
    s32 sp50;
    f32 temp_f0;
    u32 frame_index;
    u32 frame_count;
    s32 color;
    u32 packed;

    temp_f0 = mdlGetLayerWeight(*(s32 *)(arg1 + 0xB4), 0);
    frame_index = (u32)temp_f0;
    frame_count = *(u32 *)(arg1 + 0x9C);
    if ((frame_count >= frame_index) || (frame_count == 0)) {
        color = mdlEffectSamplePackedColor(arg1 + 0x10, arg1 + 0x34, frame_index, frame_count);

        /* Unpack the stored colour word and scale it by the shared 1/255 constant into VF11. */
        sp58 = *(s32 *)(arg1 + 0xC);
        effectVuUnpackColor10V0((const u32 *)&sp58, D_00761134);
        p4_vu0_move(11, P4_VU_XYZW, 10);

        /* Unpack the sampled colour, multiply by VF11, scale by 255 and pack to four bytes. */
        sp54 = color;
        effectVuUnpackColor10V0((const u32 *)&sp54, D_00761134);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        packed = p4_vu0_pack_vf10_255();
        sp50 = (s32)packed;
        sp5C = sp50;
        mdlSetColor(*(u8 **)(arg1 + 0xB4), &sp5C);
        mdlEffectSampleScalarKeys(arg1 + 0x44, frame_index, frame_count);
        *(s32 *)(*(u8 **)(arg1 + 0xB4) + 0xD8) &= ~0x100;
        if ((*(u8 *)(arg1 + 0xA1) != 0) && (*(s32 *)(arg1 + 0xA4) == 0)) {
            *(s32 *)(*(u8 **)(arg1 + 0xB4) + 0xD8) |= 0x18;
        } else {
            *(s32 *)(*(u8 **)(arg1 + 0xB4) + 0xD8) &= ~0x18;
        }
        mdlSetLayerWeight(*(u8 **)(arg1 + 0xB4), 0, *(f32 *)(arg1 + 8));
        mdlSetAttachedOwner(*(u8 **)(arg1 + 0xB4), TnGetTableBase());
        mdlQueueModelForDraw(arg0, *(u8 **)(arg1 + 0xB4));
        *(s32 *)(arg1 + 8) = 0;
    }
}
