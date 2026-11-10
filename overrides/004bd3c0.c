#include "p4pc/vu0.h"

void effMiscRotateYVU(f32 angle)
{
    RwMatrix matrix;
    f32 cosine;
    f32 sine;
    P4Mmi128 zero;
    P4Mmi128 v2;
    P4Mmi128 v3;
    P4Mmi128 v4;
    P4Mmi128 v5;

    cosine = cosf(angle);
    sine = sinf(angle);

    memset(&zero, 0, sizeof(zero));
    p4_vu0_qmfc2(&v5, 0);
    v4 = p4_mmi_pextuw(zero, v5);
    v2 = p4_mmi_pextuw(zero, v4);
    v3 = p4_mmi_pextuw(v4, zero);
    memcpy((unsigned char *)&matrix + 0, &v2, 16);
    memcpy((unsigned char *)&matrix + 16, &v3, 16);
    memcpy((unsigned char *)&matrix + 32, &v4, 16);
    memcpy((unsigned char *)&matrix + 48, &v5, 16);

    matrix.right.x = cosine;
    matrix.right.z = -sine;
    matrix.at.x = sine;
    matrix.at.z = cosine;

    p4_vu0_lqc2(24, (unsigned char *)&matrix + 0);
    p4_vu0_lqc2(25, (unsigned char *)&matrix + 16);
    p4_vu0_lqc2(26, (unsigned char *)&matrix + 32);
    p4_vu0_lqc2(27, (unsigned char *)&matrix + 48);
}
