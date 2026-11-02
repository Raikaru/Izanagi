#include "p4pc/vu0.h"

void mdlEffectSetMatrixPose(u8 *arg0, u8 *arg1)
{
    struct {
        f32 first[4];
        u8 result[16];
        f32 values[16];
    } work;

    work.first[0] = *(f32 *)(arg1 + 0x30);
    work.first[1] = *(f32 *)(arg1 + 0x34);
    work.first[2] = *(f32 *)(arg1 + 0x38);
    work.first[3] = 0.0f;
    func_004861f0((void *)arg0, work.first);
    work.values[0] = *(f32 *)(arg1 + 0);
    work.values[1] = *(f32 *)(arg1 + 4);
    work.values[2] = *(f32 *)(arg1 + 8);
    work.values[3] = 0.0f;
    work.values[4] = *(f32 *)(arg1 + 0x10);
    work.values[5] = *(f32 *)(arg1 + 0x14);
    work.values[6] = *(f32 *)(arg1 + 0x18);
    work.values[7] = 0.0f;
    work.values[8] = *(f32 *)(arg1 + 0x20);
    work.values[9] = *(f32 *)(arg1 + 0x24);
    work.values[10] = *(f32 *)(arg1 + 0x28);
    work.values[11] = 0.0f;
    work.values[12] = *(f32 *)(arg1 + 0x30);
    work.values[13] = *(f32 *)(arg1 + 0x34);
    work.values[14] = *(f32 *)(arg1 + 0x38);
    work.values[15] = 0.0f;
    effMiscMatrixToQuatVU(work.values);
    p4_vu0_sqc2(work.result, 10);
    func_00486330(arg0, work.result);
}
