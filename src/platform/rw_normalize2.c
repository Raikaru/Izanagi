/* func_003e41e0 (retail 0x003e41e0, 192 bytes): the 2D normalize (out arg0, in arg1 = {x, y}). length = sqrt(y * y + x * x) (the EE accumulator order); recip = 1 / length
 * when it is positive, else the length; out = {x * recip, y * recip} (y reloaded from the input after x was stored, the output may be the input). A length that is
 * not positive reports {1, func_003df590(0x19)} through func_003df4d0. Returns the length (a float in $f0).
 *
 * The decomp's C for it is only in the unbuilt code1_003e_cw119.c, so this is plain port C (see rw_normalize3.c). */
#include "p4pc/eefloat.h"
#include <stdint.h>

extern int32_t func_003df590(int32_t code, ...);
extern void func_003df4d0(int32_t *err);

float func_003e41e0(float *arg0, float *arg1)
{
    float x = arg1[0];
    float length;
    float recip;

    length = arg1[1] * arg1[1];
    length = p4pc_ee_add(length, x * x);
    length = p4pc_ee_sqrt(length);
    recip = (length > 0.0f) ? p4pc_ee_div(1.0f, length) : length;
    arg0[0] = x * recip;
    arg0[1] = arg1[1] * recip;
    if (length <= 0.0f) {
        int32_t err[2];

        err[0] = 1;
        err[1] = func_003df590(0x19);
        func_003df4d0(err);
    }
    return length;
}
