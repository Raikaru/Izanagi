/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004494b8 func_004494b8 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

float FUN_004494b8(float param_1,float param_2)

{
  int iVar1;
  unsigned int uVar2;
  float fVar3;
  unsigned int uVar4;
  
  if (param_2 == 1.0f) {
    fVar3 = (float)FUN_0044b340();
    return fVar3;
  }
  uVar2 = ((unsigned int)param_1 >> 0x1f) | (((int)param_2 >> 0x1e) & 2U);
  if ((0x7fffff < (unsigned int)ABS(param_1)) ||
     (((fVar3 = 3.1415925f, uVar2 != 2 && (fVar3 = param_1, 2 < uVar2)) &&
      (fVar3 = -3.1415925f, uVar2 != 3)))) {
    if ((unsigned int)ABS(param_2) < 0x800000) {
      fVar3 = -1.5707963f;
      if (-1 < (int)param_1) {
        fVar3 = 1.5707963f;
      }
    }
    else {
      iVar1 = ((int)ABS(param_1) - (int)ABS(param_2)) >> 0x17;
      fVar3 = DAT_00755c68;
      if ((iVar1 < 0x3d) && ((-1 < (int)param_2 || (fVar3 = 0.0f, -0x3d < iVar1)))) {
        uVar4 = FUN_0044b6c0(param_1 / param_2);
        fVar3 = (float)FUN_0044b340(uVar4);
      }
      if (uVar2 != 1) {
        if (uVar2 < 2) {
          if (uVar2 == 0) {
            return fVar3;
          }
        }
        else if (uVar2 == 2) {
          return DAT_00755c70 - (fVar3 - DAT_00755c6c);
        }
        fVar3 = (fVar3 - DAT_00755c6c) - DAT_00755c70;
      }
      else {
        fVar3 = -fVar3;
      }
    }
  }
  return fVar3;
}
