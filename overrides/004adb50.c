#include "p4pc/vu0.h"

void mdlEffectUpdateModelChildTransforms(u8 *arg0)
{
    extern void mdlEffectBuildBasisRows(void *arg0, void *arg1);
    extern void mdlApplyModelMatrix(void *arg0, void *arg1, s32 arg2);
    extern void mdlScale(void *arg0, void *arg1, s32 arg2);
    extern RwMatrix *mdlTranslateMatrix(RwMatrix *matrix, const RwV3d *translation, int combineOp);
    extern void mdlAdvanceModelFromFlags(void *arg0);
    extern s32 mdlEffectSamplePackedColor(u8 *arg0, u8 *arg1, s32 arg2, s32 arg3);
    extern f32 mdlEffectSampleScalarKeys(void *arg0, u32 arg1, u32 arg2);
    extern void mdlEffectSetChildPositions(void *arg0, f32 *arg1);
    extern void effMiscMatrixToQuatVU(f32 *arg0);
    extern void mdlEffectSetChildRotations(void *arg0, void *arg1);
    extern void mdlEffectSetChildScales(void *arg0, f32 arg1);
    extern void mdlEffectApplyPackedColor(u8 *arg0, s32 arg1);
    extern void mdlEffectCommitTreeFrame(void *arg0);
    extern f32 D_00761134;
    s32 sp13C;
    s32 sp138;
    s32 sp134;
    f32 vec[3];
    f32 pos[4];
    u8 out[0x10];
    u8 frame[0x40];
    f32 mat[16];
    u32 n;
    s32 *objs;
    s32 count;
    s32 total;
    u32 i;
    f32 temp_f0;
    f32 speed;
    s32 color;
    f32 scale;
    f32 zero;
    f32 t;

    count = *(s32 *)(arg0 + 0x2C);
    total = *(s32 *)(arg0 + 0x90);
    n = *(u32 *)(arg0 + 0x28);
    objs = *(s32 **)(arg0 + 0x98);
    if (count < total || total == 0) {
        mdlEffectBuildBasisRows(frame, arg0 + 0x10);
        mdlApplyModelMatrix(*(void **)(arg0 + 0xA0), frame, 0);
        temp_f0 = *(f32 *)(arg0 + 0x20);
        vec[2] = temp_f0;
        vec[1] = temp_f0;
        vec[0] = temp_f0;
        mdlScale(*(void **)(arg0 + 0xA0), vec, 2);
        vec[0] = *(f32 *)(arg0 + 0);
        vec[1] = *(f32 *)(arg0 + 4);
        vec[2] = *(f32 *)(arg0 + 8);
        mdlTranslateMatrix((RwMatrix *)*(void **)(arg0 + 0xA0), (const RwV3d *)vec, 2);
        mdlSetLayerWeight(*(void **)(arg0 + 0xA0), 0, *(f32 *)(arg0 + 0x94));
        mdlAdvanceModelFromFlags(*(void **)(arg0 + 0xA0));
        if (*(s32 *)(arg0 + 0x9C) != 0) {
            color = mdlEffectSamplePackedColor(arg0 + 0x30, arg0 + 0x54, count, total);
            sp13C = *(s32 *)(arg0 + 0x24);
            scale = D_00761134;
            sp138 = color;
            sp134 = (s32)p4_vu0_packed_color_scale2((uint32_t)sp13C, (uint32_t)sp138, scale);
            color = *(s32 *)&sp134;
            t = mdlEffectSampleScalarKeys(arg0 + 0x64, count, total) / 10.0f;
            speed = t * *(f32 *)(arg0 + 0x20);
            i = 0;
            zero = 0.0f;
            goto loop_check;
loop_body:

                if (mdlCopyIndexedMatrix(*(u8 **)(arg0 + 0xA0), i, frame) != 0) {
                    pos[0] = *(f32 *)(frame + 0x30);
                    pos[1] = *(f32 *)(frame + 0x34);
                    pos[2] = *(f32 *)(frame + 0x38);
                    pos[3] = zero;
                    mdlEffectSetChildPositions((void *)*objs, pos);
                    mat[0] = *(f32 *)(frame + 0x00);
                    mat[1] = *(f32 *)(frame + 0x04);
                    mat[2] = *(f32 *)(frame + 0x08);
                    mat[3] = zero;
                    mat[4] = *(f32 *)(frame + 0x10);
                    mat[5] = *(f32 *)(frame + 0x14);
                    mat[6] = *(f32 *)(frame + 0x18);
                    mat[7] = zero;
                    mat[8] = *(f32 *)(frame + 0x20);
                    mat[9] = *(f32 *)(frame + 0x24);
                    mat[10] = *(f32 *)(frame + 0x28);
                    mat[11] = zero;
                    mat[12] = *(f32 *)(frame + 0x30);
                    mat[13] = *(f32 *)(frame + 0x34);
                    mat[14] = *(f32 *)(frame + 0x38);
                    mat[15] = zero;
                    effMiscMatrixToQuatVU(mat);
                            p4_vu0_sqc2(out, 10);
                    mdlEffectSetChildRotations((void *)*objs, out);
                    mdlEffectSetChildScales((void *)*objs, speed);
                    mdlEffectApplyPackedColor((void *)*objs, color);
                    mdlEffectCommitTreeFrame((void *)*objs);
                }
                i += 1;
                objs += 1;
loop_check:
                if (i < n) {
                    goto loop_body;
                }
        }
    }
    *(s32 *)(arg0 + 0x2C) += 1;
}
