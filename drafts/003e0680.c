/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003e0680 func_003e0680 src/promoted/code1_003e.c - romwright draft, unreviewed */

float * FUN_003e0680(float param_1,float param_2,float *param_3,float *param_4,long param_5)

{
  int iVar1;
  float *pfVar2;
  float *pfVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float afStack_90 [16];
  float afStack_50 [5];
  float fStack_3c;
  float fStack_38;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  unsigned int uStack_20;
  unsigned int uStack_1c;
  unsigned int uStack_18;
  int uStack_8;
  unsigned int uStack_4;
  
  fVar7 = *param_4;
  uStack_20 = 0;
  afStack_50[3] = 4.2039e-45f;
  fVar5 = param_4[1];
  uStack_1c = 0;
  uStack_18 = 0;
  afStack_50[0] = 1.0f - (1.0f - fVar7 * fVar7) * param_1;
  fVar4 = param_4[2];
  fStack_3c = 1.0f - (1.0f - fVar5 * fVar5) * param_1;
  fStack_28 = 1.0f - (1.0f - fVar4 * fVar4) * param_1;
  fVar8 = fVar5 * fVar4 * param_1;
  fVar6 = fVar4 * fVar7 * param_1;
  param_1 = fVar7 * fVar5 * param_1;
  afStack_50[1] = param_1 + fVar4 * param_2;
  afStack_50[4] = param_1 - fVar4 * param_2;
  afStack_50[2] = fVar6 - fVar5 * param_2;
  fStack_30 = fVar6 + fVar5 * param_2;
  fStack_38 = fVar8 + fVar7 * param_2;
  fStack_2c = fVar8 - fVar7 * param_2;
  if (param_5 != 2) {
    if (param_5 != 1) {
      if (param_5 != 0) {
        uStack_8 = 1;
        uStack_4 = FUN_003df590(0xffffffff80000003,0x70b6f0);
        FUN_003df4d0(&uStack_8);
        param_3 = (float *)0x0;
      }
      else {
        pfVar3 = afStack_50;
        iVar1 = 8;
        pfVar2 = param_3;
        do {
          iVar1 = iVar1 - 1;
          fVar4 = pfVar3[1];
          *pfVar2 = *pfVar3;
          pfVar3 = pfVar3 + 2;
          pfVar2[1] = fVar4;
          pfVar2 = pfVar2 + 2;
        } while (0 < iVar1);
      }
    }
    else {
      FUN_003e05f0(afStack_90,afStack_50,param_3);
      pfVar3 = afStack_90;
      iVar1 = 8;
      pfVar2 = param_3;
      do {
        iVar1 = iVar1 - 1;
        fVar4 = pfVar3[1];
        *pfVar2 = *pfVar3;
        pfVar3 = pfVar3 + 2;
        pfVar2[1] = fVar4;
        pfVar2 = pfVar2 + 2;
      } while (0 < iVar1);
    }
  }
  else {
    FUN_003e05f0(afStack_90,param_3,afStack_50);
    pfVar3 = afStack_90;
    iVar1 = 8;
    pfVar2 = param_3;
    do {
      iVar1 = iVar1 - 1;
      fVar4 = pfVar3[1];
      *pfVar2 = *pfVar3;
      pfVar3 = pfVar3 + 2;
      pfVar2[1] = fVar4;
      pfVar2 = pfVar2 + 2;
    } while (0 < iVar1);
  }
  return param_3;
}
