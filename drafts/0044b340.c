/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0044b340 func_0044b340 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

float FUN_0044b340(float param_1)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  
  fVar2 = ABS(param_1);
  if (0x507fffff < (unsigned int)fVar2) {
    if (0 < (int)param_1) {
      return DAT_00756394;
    }
    return DAT_00756398;
  }
  if ((unsigned int)fVar2 < 0x3ee00000) {
    iVar1 = -1;
    fVar5 = param_1;
    if ((unsigned int)fVar2 < 0x31000000) {
      if (DAT_007563a0 < param_1 + DAT_0075639c) {
        return param_1;
      }
      iVar1 = -1;
    }
LAB_0044b3f8:
    fVar2 = fVar5 * fVar5;
  }
  else {
    fVar3 = (float)FUN_0044b6c0();
    if ((unsigned int)fVar2 < 0x3f980000) {
      if ((unsigned int)fVar2 < 0x3f300000) {
        iVar1 = 0;
        fVar2 = fVar3 + DAT_007563a4;
        fVar5 = (fVar3 + fVar3) - DAT_007563a0;
      }
      else {
        iVar1 = 1;
        fVar2 = fVar3 + DAT_007563a0;
        fVar5 = fVar3 - DAT_007563a0;
      }
    }
    else {
      if (0x401bffff < (unsigned int)fVar2) {
        fVar5 = DAT_007563ac / fVar3;
        iVar1 = 3;
        goto LAB_0044b3f8;
      }
      iVar1 = 2;
      fVar5 = fVar3 - DAT_007563a8;
      fVar2 = fVar3 * DAT_007563a8 + DAT_007563a0;
    }
    fVar5 = fVar5 / fVar2;
    fVar2 = fVar5 * fVar5;
  }
  fVar3 = fVar2 * fVar2;
  fVar4 = fVar3 * (fVar3 * (fVar3 * (fVar3 * (fVar3 * DAT_007563b4 + DAT_007563bc) + DAT_007563c4) +
                           DAT_007563cc) + DAT_007563d4);
  fVar2 = fVar2 * (fVar3 * (fVar3 * (fVar3 * (fVar3 * (fVar3 * DAT_007563b0 + DAT_007563b8) +
                                             DAT_007563c0) + DAT_007563c8) + DAT_007563d0) +
                  DAT_007563d8);
  if (iVar1 < 0) {
    fVar5 = fVar5 - fVar5 * (fVar2 + fVar4);
  }
  else {
    fVar5 = *(float *)(iVar1 * 4 + 0x756340) -
            ((fVar5 * (fVar2 + fVar4) - *(float *)(iVar1 * 4 + 0x756350)) - fVar5);
    if ((int)param_1 < 0) {
      fVar5 = -fVar5;
    }
  }
  return fVar5;
}
