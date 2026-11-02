/* func_003e4030 (retail 0x003e4030, 128 bytes): RwV3dNormalize(out arg0, in arg1). recip = 1 / sqrt(y * y + x * x + z * z) (the EE accumulator order y, x, z) when the
 * length is positive, else the (non-positive) length itself; out = in * recip, stored x first with y and z reloaded from the input (the output may be the input);
 * returns recip (a float in $f0).
 *
 * The decomp's C for it is only in the unbuilt code1_003e_cw119.c (no built unit has a `// FUN_003E4030` marker), so an override file would wait for that unit and the
 * trap stub stayed; this is the same retail code as plain port C, like func_003c95a0. */
#include "p4pc/eefloat.h"

float func_003e4030(float *arg0, float *arg1)
{
    float x = arg1[0];
    float recip;

    recip = arg1[1] * arg1[1];
    recip = p4pc_ee_add(recip, x * x);
    recip = p4pc_ee_add(recip, arg1[2] * arg1[2]);
    recip = p4pc_ee_sqrt(recip);
    recip = (recip > 0.0f) ? p4pc_ee_div(1.0f, recip) : recip;
    arg0[0] = x * recip;
    arg0[1] = arg1[1] * recip;
    arg0[2] = arg1[2] * recip;
    return recip;
}
