/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dc740 func_003dc740 src/promoted/code1_003d.c - romwright draft, unreviewed */

long FUN_003dc740(float param_1,long param_2,long param_3,long param_4)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fStack_40;
  float fStack_3c;
  float fStack_38;
  float fStack_34;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  float fStack_14;
  int uStack_10;
  unsigned int uStack_c;
  int uStack_8;
  unsigned int uStack_4;
  
  if ((param_2 != 0) && (param_3 != 0)) {
    pfVar1 = (float *)param_2;
    if (param_4 != 2) {
      if (param_4 != 1) {
        if (param_4 == 0) {
          fVar2 = (float)FUN_0044b7b0(param_1 * 0.008726646f);
          fVar3 = (float)FUN_0044b610(param_1 * 0.008726646f);
          pfVar1[3] = fVar3;
          FUN_003e40b0(param_2,param_3);
          *pfVar1 = *pfVar1 * fVar2;
          pfVar1[1] = pfVar1[1] * fVar2;
          pfVar1[2] = pfVar1[2] * fVar2;
        }
        else {
          uStack_8 = 0x195;
          uStack_4 = FUN_003df590(0xffffffff80000003,0x70b6c0);
          FUN_003df4d0(&uStack_8);
          param_2 = 0;
        }
      }
      else {
        fStack_2c = pfVar1[1];
        fStack_30 = *pfVar1;
        fStack_28 = pfVar1[2];
        fStack_24 = pfVar1[3];
        fVar2 = (float)FUN_0044b7b0(param_1 * 0.008726646f);
        fStack_14 = (float)FUN_0044b610(param_1 * 0.008726646f);
        FUN_003e40b0(&fStack_20,param_3);
        fStack_1c = fStack_1c * fVar2;
        fStack_20 = fStack_20 * fVar2;
        fStack_18 = fStack_18 * fVar2;
        pfVar1[3] = fStack_24 * fStack_14 -
                    (fStack_28 * fStack_18 + fStack_30 * fStack_20 + fStack_2c * fStack_1c);
        *pfVar1 = fStack_2c * fStack_18 - fStack_28 * fStack_1c;
        pfVar1[1] = fStack_28 * fStack_20 - fStack_30 * fStack_18;
        pfVar1[2] = fStack_30 * fStack_1c - fStack_2c * fStack_20;
        *pfVar1 = fStack_20 * fStack_24 + *pfVar1 + 0.0f;
        pfVar1[1] = fStack_1c * fStack_24 + pfVar1[1] + 0.0f;
        pfVar1[2] = fStack_18 * fStack_24 + pfVar1[2] + 0.0f;
        *pfVar1 = fStack_30 * fStack_14 + *pfVar1 + 0.0f;
        pfVar1[1] = fStack_2c * fStack_14 + pfVar1[1] + 0.0f;
        pfVar1[2] = fStack_28 * fStack_14 + pfVar1[2] + 0.0f;
      }
    }
    else {
      fVar2 = pfVar1[1];
      fVar6 = *pfVar1;
      fVar3 = pfVar1[2];
      fVar4 = pfVar1[3];
      fVar5 = (float)FUN_0044b7b0(param_1 * 0.008726646f);
      fStack_34 = (float)FUN_0044b610(param_1 * 0.008726646f);
      FUN_003e40b0(&fStack_40,param_3);
      fStack_3c = fStack_3c * fVar5;
      fStack_40 = fStack_40 * fVar5;
      fStack_38 = fStack_38 * fVar5;
      pfVar1[3] = fStack_34 * fVar4 - (fStack_38 * fVar3 + fStack_40 * fVar6 + fStack_3c * fVar2);
      *pfVar1 = fStack_3c * fVar3 - fStack_38 * fVar2;
      pfVar1[1] = fStack_38 * fVar6 - fStack_40 * fVar3;
      pfVar1[2] = fStack_40 * fVar2 - fStack_3c * fVar6;
      *pfVar1 = fVar6 * fStack_34 + *pfVar1 + 0.0f;
      pfVar1[1] = fVar2 * fStack_34 + pfVar1[1] + 0.0f;
      pfVar1[2] = fVar3 * fStack_34 + pfVar1[2] + 0.0f;
      *pfVar1 = fStack_40 * fVar4 + *pfVar1 + 0.0f;
      pfVar1[1] = fStack_3c * fVar4 + pfVar1[1] + 0.0f;
      pfVar1[2] = fStack_38 * fVar4 + pfVar1[2] + 0.0f;
    }
  }
  else {
    uStack_10 = 0x195;
    uStack_c = FUN_003df590(0xffffffff80000016);
    FUN_003df4d0(&uStack_10);
    param_2 = 0;
  }
  return param_2;
}
