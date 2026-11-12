#include "p4pc/vu0.h"

void btlFormationAttachRotation(u8 *arg0, u8 *arg1, u8 *arg2, u8 *arg3)
{
    struct Frame {
        f32 input[16];
        f32 output[16];
        f32 vector[4];
    } frame;
    u8 type;
    u8 *entry;
    s32 flags;
    s32 lookup;
    s32 lookup_offset;
    u8 *lookup_base;

    extern void RtQuatTransformVectors(f32 *arg0, f32 *arg1, s32 arg2, u8 *arg3);
    extern f32 func_0044b950(f32 arg0, f32 arg1);
    extern s32 mdlCopyIndexedMatrix(u8 *arg0, s32 arg1, f32 *arg2);
    extern void effMiscMatrixToQuatVU(f32 *arg0);
    extern void effMiscEulerToQuatVU(f32 arg0, f32 arg1, f32 arg2);

    entry = arg2 + 8;
    type = *(u8 *)(arg2 + 8);
    switch (type) {
    case 0:
        RtQuatTransformVectors(frame.vector, (f32 *)D_0060A0F0, 1, arg0 + 0x40);
        if (frame.vector[0] != 0.0f)
            goto case0_nonzero;
        if (frame.vector[2] == 0.0f)
            goto case0_zero;
case0_nonzero:
        effMiscEulerToQuatVU(0.0f,
                      func_0044b950(frame.vector[0], frame.vector[2]),
                      0.0f);
        p4_vu0_sqc2(arg3, 10);
        goto case0_done;
case0_zero:
        p4_vu0_sqc2(arg3, 0);
case0_done:
        break;
    case 2:
    case 3:
    case 4:
    case 0xC:
        p4_vu0_sqc2(arg3, 0);
        break;
    case 1:
    case 0xD:
        flags = *(s32 *)(arg1 + 0x98);
        if ((flags & 2) == 0)
            goto case1_no_flags;
        if (mdlCopyIndexedMatrix(*(u8 **)(arg1 + 0xA00),
                          *(u16 *)(entry + 4), frame.input) == 0)
            goto case1_failure;
            frame.output[0] = frame.input[0];
            frame.output[1] = frame.input[1];
            frame.output[2] = frame.input[2];
            frame.output[3] = 0.0f;
            frame.output[4] = frame.input[4];
            frame.output[5] = frame.input[5];
            frame.output[6] = frame.input[6];
            frame.output[7] = 0.0f;
            frame.output[8] = frame.input[8];
            frame.output[9] = frame.input[9];
            frame.output[10] = frame.input[10];
            frame.output[11] = 0.0f;
            frame.output[12] = frame.input[12];
            frame.output[13] = frame.input[13];
            frame.output[14] = frame.input[14];
            frame.output[15] = 0.0f;
            effMiscMatrixToQuatVU(frame.output);
            p4_vu0_sqc2(arg3, 10);
        break;
case1_failure:
        p4_vu0_sqc2(arg3, 0);
        break;
case1_no_flags:
        p4_vu0_sqc2(arg3, 0);
        break;
    case 8:
    case 9:
        flags = *(s32 *)(arg1 + 0x98);
        if ((flags & 2) != 0) {
            lookup_offset = 12 * (type == 8);
            lookup_base = *(u8 **)(arg1 + 0xA00);
            lookup_base += 0x290;
            lookup = *(s32 *)(lookup_base + lookup_offset);
            if (lookup == 0)
                break;
            if (mdlCopyIndexedMatrix((u8 *)lookup,
                              *(u16 *)(entry + 4), frame.input) == 0)
                goto case8_failure;
            frame.output[0] = frame.input[0];
            frame.output[1] = frame.input[1];
            frame.output[2] = frame.input[2];
            frame.output[3] = 0.0f;
            frame.output[4] = frame.input[4];
            frame.output[5] = frame.input[5];
            frame.output[6] = frame.input[6];
            frame.output[7] = 0.0f;
            frame.output[8] = frame.input[8];
            frame.output[9] = frame.input[9];
            frame.output[10] = frame.input[10];
            frame.output[11] = 0.0f;
            frame.output[12] = frame.input[12];
            frame.output[13] = frame.input[13];
            frame.output[14] = frame.input[14];
            frame.output[15] = 0.0f;
            effMiscMatrixToQuatVU(frame.output);
            p4_vu0_sqc2(arg3, 10);
            break;
case8_failure:
            p4_vu0_sqc2(arg3, 0);
        } else {
            p4_vu0_sqc2(arg3, 0);
        }
        break;
    case 7:
    case 0xA:
    case 0xB:
        p4_vu0_sqc2(arg3, 0);
        break;
    case 5:
    case 6:
        break;
    default:
        break;
    }
}
