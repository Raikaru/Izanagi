/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bb5b0 func_003bb5b0 src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned long long FUN_003bb5b0(float param_1,int param_2,long param_3,unsigned long long param_4,long param_5)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  float *pfVar4;
  unsigned int uVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  int uStack_8;
  unsigned int uStack_4;
  
  if (0.0f <= param_1) {
    iVar1 = *(int *)(param_2 + 0xc);
  }
  else {
    param_1 = -param_1;
    iVar1 = *(int *)(param_2 + 0xc);
  }
  if ((iVar1 == 2) && (1.0f < param_1)) {
    if (param_1 < 2.1474836e+09f) {
      uVar5 = (unsigned int)param_1;
    }
    else {
      uVar5 = (int)(param_1 - 2.1474836e+09f) | 0x80000000;
    }
    param_1 = param_1 - (float)uVar5;
  }
  if (param_3 != 0xb) {
    if (param_3 != 10) {
      uStack_8 = 0x102;
      uStack_4 = FUN_003df590(1);
      FUN_003df4d0(&uStack_8);
      return 0;
    }
  }
  else if (param_1 <= 0.5f) {
    param_1 = param_1 * (param_1 + param_1);
  }
  else {
    param_1 = param_1 * 4.0f + param_1 * -2.0f * param_1 + -1.0f;
  }
  fVar6 = 1.0f;
  if (param_1 != 1.0f) {
    param_1 = (float)(*(int *)(param_2 + 8) - 3) * param_1;
    iVar2 = (int)param_1;
    fVar6 = (param_1 - (float)iVar2) * 256.0f;
    iVar1 = (int)fVar6;
    fVar6 = fVar6 - (float)iVar1;
  }
  else {
    iVar2 = *(int *)(param_2 + 8) - 4;
    iVar1 = 0xff;
  }
  iVar1 = iVar1 * 4;
  pfVar3 = (float *)(param_2 + 0x38 + iVar2 * 0xc);
  fVar9 = *(float *)(DAT_00764718 + iVar1);
  fVar8 = *(float *)(DAT_00764714 + iVar1);
  fVar7 = *(float *)(DAT_00764710 + iVar1);
  fVar10 = fVar6 * (((float *)(DAT_00764718 + iVar1))[1] - fVar9) + fVar9 + 0.0f;
  fVar11 = fVar6 * (((float *)(DAT_00764714 + iVar1))[1] - fVar8) + fVar8 + 0.0f;
  fVar8 = fVar6 * (((float *)(DAT_00764710 + iVar1))[1] - fVar7) + fVar7 + 0.0f;
  fVar9 = *(float *)(DAT_0076470c + iVar1);
  fVar7 = ((float *)(DAT_0076470c + iVar1))[1];
  pfVar4 = (float *)param_4;
  *pfVar4 = *pfVar3 * fVar10;
  pfVar4[1] = pfVar3[1] * fVar10;
  fVar7 = fVar6 * (fVar7 - fVar9) + fVar9 + 0.0f;
  pfVar4[2] = pfVar3[2] * fVar10;
  *pfVar4 = pfVar3[3] * fVar11 + *pfVar4 + 0.0f;
  pfVar4[1] = pfVar3[4] * fVar11 + pfVar4[1] + 0.0f;
  pfVar4[2] = pfVar3[5] * fVar11 + pfVar4[2] + 0.0f;
  *pfVar4 = pfVar3[6] * fVar8 + *pfVar4 + 0.0f;
  pfVar4[1] = pfVar3[7] * fVar8 + pfVar4[1] + 0.0f;
  pfVar4[2] = pfVar3[8] * fVar8 + pfVar4[2] + 0.0f;
  *pfVar4 = pfVar3[9] * fVar7 + *pfVar4 + 0.0f;
  pfVar4[1] = pfVar3[10] * fVar7 + pfVar4[1] + 0.0f;
  pfVar4[2] = pfVar3[0xb] * fVar7 + pfVar4[2] + 0.0f;
  if (param_5 != 0) {
    fVar9 = *(float *)(DAT_00764728 + iVar1);
    fVar8 = *(float *)(DAT_00764724 + iVar1);
    fVar7 = *(float *)(DAT_00764720 + iVar1);
    fVar9 = fVar6 * (((float *)(DAT_00764728 + iVar1))[1] - fVar9) + fVar9 + 0.0f;
    fVar10 = fVar6 * (((float *)(DAT_00764724 + iVar1))[1] - fVar8) + fVar8 + 0.0f;
    fVar7 = fVar6 * (((float *)(DAT_00764720 + iVar1))[1] - fVar7) + fVar7 + 0.0f;
    fVar8 = *(float *)(DAT_0076471c + iVar1);
    fVar6 = fVar6 * (((float *)(DAT_0076471c + iVar1))[1] - fVar8) + fVar8 + 0.0f;
    pfVar4 = (float *)param_5;
    *pfVar4 = *pfVar3 * fVar9;
    pfVar4[1] = pfVar3[1] * fVar9;
    pfVar4[2] = pfVar3[2] * fVar9;
    *pfVar4 = pfVar3[3] * fVar10 + *pfVar4 + 0.0f;
    pfVar4[1] = pfVar3[4] * fVar10 + pfVar4[1] + 0.0f;
    pfVar4[2] = pfVar3[5] * fVar10 + pfVar4[2] + 0.0f;
    *pfVar4 = pfVar3[6] * fVar7 + *pfVar4 + 0.0f;
    pfVar4[1] = pfVar3[7] * fVar7 + pfVar4[1] + 0.0f;
    pfVar4[2] = pfVar3[8] * fVar7 + pfVar4[2] + 0.0f;
    *pfVar4 = pfVar3[9] * fVar6 + *pfVar4 + 0.0f;
    pfVar4[1] = pfVar3[10] * fVar6 + pfVar4[1] + 0.0f;
    pfVar4[2] = pfVar3[0xb] * fVar6 + pfVar4[2] + 0.0f;
    fVar6 = (float)FUN_003e4030(param_5,param_5);
    if (fVar6 == 0.0f) {
      *pfVar4 = 0.0f;
      pfVar4[1] = 1.0f;
      pfVar4[2] = 0.0f;
    }
  }
  return param_4;
}
