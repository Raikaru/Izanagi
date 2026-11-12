#include "p4pc/vu0.h"

void mdlEffectFlushTreeFrame(u8 *arg0)
{
    extern u_long128 mdlEffectResolveChildOffset(u8 *arg0, u8 *arg1, u_long128 *arg2);
    extern u_long128 mdlEffectResolveChildOrientation(u8 *arg0, u8 *arg1, u_long128 *arg2);
    extern void mdlEffectSetChildRotations(u8 *arg0, u8 *arg1);
    /* 00484b30 initializes the 0x80-byte state; 00484bb0 allocates a
     * 0x90-byte root. The union keeps field and quadword transfers in the
     * same object, including all four components of the saved rotation. */
    typedef union SceneStateStorage {
        Code48InitialState fields;
        u_long128 quad[8];
    } SceneStateStorage;
    typedef struct SceneRoot {
        SceneStateStorage state;
        s32 childCount;
        u32 frame; /* Wrapping storage, read as signed for selection. */
        u8 *lastChild;
        u8 *firstChild;
    } SceneRoot;
    typedef char SceneRootExtent[sizeof(SceneRoot) == 0x90 ? 1 : -1];
    SceneRoot *root;
    /* The saved parent rotation survives both propagation calls. */
    struct {
        u_long128 savedRotation;
        u_long128 rotation;
        u_long128 position;
        u_long128 origin;
    } work;
    f32 scale;
    f32 five;
    u8 *child;
    s32 flags;
    s32 count_minus_1;
    u8 *primitive;
    void (*positionCallback)(s32, void *);
    void (*rotationCallback)(s32, void *);
    s32 childFlags;
    u32 mask;

    root = (SceneRoot *)(void *)arg0;
    if ((s32)root->frame <= 0) {
        return;
    }
    flags = root->state.fields.word68;
    p4_vu0_lqc2(10, arg0 + 0x40);
    p4_vu0_lqc2(11, arg0);
    p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    p4_vu0_sqc2(&work.origin, 10);
    if ((flags & 0x60) != 0) {
        if ((flags & 0x80000000) == 0) {
            u_long128 *save_slot;
            u_long128 *restore_slot;
            save_slot = &work.savedRotation;
            *save_slot = root->state.quad[5];
            mdlEffectResolveChildOrientation(arg0, (u8 *)&work.origin, &work.rotation);
            mdlEffectSetChildRotations(arg0, (u8 *)&work.rotation);
            restore_slot = &work.savedRotation;
            /* lint: allow H009 -- retail copies this quad through $2 at 0x00485900; b210 keeps $v0 live after calls (user-approved 2026-10-09) */
            *(u_long128 *)((u8 *)root + 0x50) = *restore_slot;
        }
    }
    /* Reload after propagation, then subtract in the word's wrapping domain. */
    count_minus_1 = (s32)(root->frame - 1U);
    scale = root->state.fields.scalar60 * root->state.fields.scalar74;
    child = root->firstChild;
    mask = 0x80000000;
    five = 5.0f;
    for (; child != NULL; child = *(u8 **)(child + 0xAC)) {
        if (count_minus_1 < *(s32 *)(child + 0x80)) {
            continue;
        }
        if ((*(s32 *)(child + 0x84) & 2) != 0) {
            continue;
        }
        if ((flags & mask) == 0) {
            if ((*(s32 *)(child + 0x68) & 0x18) != 0) {
                mdlEffectResolveChildOffset(child, (u8 *)&work.origin, &work.position);
                p4_vu0_lqc2(10, &work.origin);
                childFlags = *(s32 *)(child + 0x68);
                if ((childFlags & 4) != 0) {
                    p4_vu0_set_xf(2, five); p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
                }
                p4_vu0_lqc2(11, &work.position);
                if ((childFlags & 0x80) != 0) {
                    p4_vu0_set_xf(2, scale); p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                }
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_sqc2(&work.position, 10);
                primitive = *(u8 **)(child + 0x90);
                positionCallback = *(void (**)(s32, void *))(((u8 *)D_00713480 + (*(u16 *)(primitive + 4) << 6)) + 0x20);
                if (positionCallback != NULL) {
                    positionCallback(*(s32 *)(primitive + 8), &work.position);
                }
            }
            if ((*(s32 *)(child + 0x68) & 0x60) != 0) {
                mdlEffectResolveChildOrientation(child, (u8 *)&work.origin, &work.rotation);
                primitive = *(u8 **)(child + 0x90);
                rotationCallback = *(void (**)(s32, void *))(((u8 *)D_00713480 + (*(u16 *)(primitive + 4) << 6)) + 0x24);
                if (rotationCallback != NULL) {
                    rotationCallback(*(s32 *)(primitive + 8), &work.rotation);
                }
            }
        }
        primitive = *(u8 **)(child + 0x90);
        (*(void (**)(s32))(((u8 *)D_00713480 + (*(u16 *)(primitive + 4) << 6)) + 0x0C))(*(s32 *)(primitive + 8));
    }
    root->state.fields.word68 &= 0x7fffffffU;
}
