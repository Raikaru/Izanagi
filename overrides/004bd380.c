#include "p4pc/vu0.h"

void effMiscAxisAngleVU(const RwV3d* axis, f32 angle)
{
    RwMatrix matrix;

    effMiscAxisAngleMatrix(angle, axis, &matrix);
    p4_vu0_lqc2(28, (unsigned char *)&matrix + 0);
    p4_vu0_lqc2(29, (unsigned char *)&matrix + 16);
    p4_vu0_lqc2(30, (unsigned char *)&matrix + 32);
    p4_vu0_lqc2(31, (unsigned char *)&matrix + 48);
}
