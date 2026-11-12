#include "p4pc/vu0.h"

void effMiscEulerToQuatVU(f32 angleX, f32 angleY, f32 angleZ)
{
    RwV4d quaternion;
    f32 halfAngle;
    f32 cosY;
    f32 sinY;
    f32 cosZ;
    f32 cosX;
    f32 sinX;
    f32 sinZ;

    halfAngle = -angleX * 0.5f;
    cosX = cosf(halfAngle);
    sinX = sinf(halfAngle);
    halfAngle = -angleY * 0.5f;
    cosY = cosf(halfAngle);
    sinY = sinf(halfAngle);
    halfAngle = -angleZ * 0.5f;
    cosZ = cosf(halfAngle);
    sinZ = sinf(halfAngle);
    quaternion.x = cosX * (sinZ * sinY) + sinX * (cosZ * cosY);
    quaternion.y = cosX * (cosZ * sinY) - sinX * (sinZ * cosY);
    quaternion.z = cosX * (sinZ * cosY) + sinX * (cosZ * sinY);
    quaternion.w = cosX * (cosZ * cosY) - sinX * (sinZ * sinY);

    p4_vu0_lqc2(10, &quaternion);
}
