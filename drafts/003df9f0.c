/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003df9f0 func_003df9f0 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003df9f0_arg0 fn_003df9f0_arg0;
struct fn_003df9f0_arg0 {
    undefined4 field_0x0; /* +0x0 */
    undefined4 field_0x4; /* +0x4 */
    undefined4 field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    undefined4 field_0x10; /* +0x10 */
    undefined4 field_0x14; /* +0x14 */
    undefined4 field_0x18; /* +0x18 */
    unsigned char pad_0x1c[0x4];
    undefined4 field_0x20; /* +0x20 */
    undefined4 field_0x24; /* +0x24 */
    undefined4 field_0x28; /* +0x28 */
    unsigned char pad_0x2c[0x4];
    undefined4 field_0x30; /* +0x30 */
    undefined4 field_0x34; /* +0x34 */
    undefined4 field_0x38; /* +0x38 */
};


long FUN_003df9f0(fn_003df9f0_arg0 *param_1,float *param_2)

{
  float *pfVar1;
  float *pfVar2;
  float *pfVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float afStack_30 [12];
  
  afStack_30[9] = param_2[1];
  afStack_30[0] = param_2[8];
  afStack_30[10] = param_2[2];
  afStack_30[8] = *param_2;
  afStack_30[4] = param_2[4];
  afStack_30[5] = param_2[5];
  afStack_30[1] = param_2[9];
  afStack_30[6] = param_2[6];
  afStack_30[2] = param_2[10];
  fVar4 = SQRT(afStack_30[10] * afStack_30[10] +
               afStack_30[8] * afStack_30[8] + afStack_30[9] * afStack_30[9]);
  fVar6 = param_2[0xc];
  fVar8 = param_2[0xd];
  fVar5 = param_2[0xe];
  if (0.0f < fVar4) {
    fVar4 = 1.0f / fVar4;
  }
  fVar9 = SQRT(afStack_30[6] * afStack_30[6] +
               afStack_30[4] * afStack_30[4] + afStack_30[5] * afStack_30[5]);
  afStack_30[9] = afStack_30[9] * fVar4;
  afStack_30[8] = afStack_30[8] * fVar4;
  afStack_30[10] = afStack_30[10] * fVar4;
  if (0.0f < fVar9) {
    fVar9 = 1.0f / fVar9;
  }
  afStack_30[5] = afStack_30[5] * fVar9;
  afStack_30[6] = afStack_30[6] * fVar9;
  fVar7 = SQRT(afStack_30[2] * afStack_30[2] +
               afStack_30[0] * afStack_30[0] + afStack_30[1] * afStack_30[1]);
  afStack_30[4] = afStack_30[4] * fVar9;
  if (0.0f < fVar7) {
    fVar7 = 1.0f / fVar7;
  }
  afStack_30[0] = afStack_30[0] * fVar7;
  afStack_30[1] = afStack_30[1] * fVar7;
  afStack_30[2] = afStack_30[2] * fVar7;
  if (0.0f < fVar4) {
    if (0.0f < fVar9) {
      if (0.0f < fVar7) {
        fVar4 = afStack_30[6] * afStack_30[2] +
                afStack_30[4] * afStack_30[0] + afStack_30[5] * afStack_30[1] + 0.0f + 0.0f;
        if (fVar4 < 0.0f) {
          fVar4 = -fVar4;
        }
        fVar9 = afStack_30[2] * afStack_30[10] +
                afStack_30[0] * afStack_30[8] + afStack_30[1] * afStack_30[9];
        if (fVar9 < 0.0f) {
          fVar9 = -fVar9;
        }
        fVar7 = afStack_30[10] * afStack_30[6] +
                afStack_30[8] * afStack_30[4] + afStack_30[9] * afStack_30[5];
        if (fVar7 < 0.0f) {
          fVar7 = -fVar7;
        }
        if (fVar4 < fVar9) {
          if (fVar4 < fVar7) {
            pfVar3 = afStack_30 + 4;
            pfVar2 = afStack_30;
            pfVar1 = afStack_30 + 8;
          }
          else {
            pfVar3 = afStack_30 + 8;
            pfVar2 = afStack_30 + 4;
            pfVar1 = afStack_30;
          }
        }
        else if (fVar9 < fVar7) {
          pfVar3 = afStack_30;
          pfVar2 = afStack_30 + 8;
          pfVar1 = afStack_30 + 4;
        }
        else {
          pfVar3 = afStack_30 + 8;
          pfVar2 = afStack_30 + 4;
          pfVar1 = afStack_30;
        }
      }
      else {
        pfVar3 = afStack_30 + 8;
        pfVar2 = afStack_30 + 4;
        pfVar1 = afStack_30;
      }
    }
    else {
      pfVar3 = afStack_30;
      pfVar2 = afStack_30 + 8;
      pfVar1 = afStack_30 + 4;
    }
  }
  else {
    pfVar3 = afStack_30 + 4;
    pfVar2 = afStack_30;
    pfVar1 = afStack_30 + 8;
  }
  *pfVar1 = pfVar3[1] * pfVar2[2] - pfVar3[2] * pfVar2[1];
  pfVar1[1] = pfVar3[2] * *pfVar2 - *pfVar3 * pfVar2[2];
  pfVar1[2] = *pfVar3 * pfVar2[1] - pfVar3[1] * *pfVar2;
  fVar4 = SQRT(pfVar1[2] * pfVar1[2] + *pfVar1 * *pfVar1 + pfVar1[1] * pfVar1[1]);
  if (0.0f < fVar4) {
    fVar4 = 1.0f / fVar4;
  }
  *pfVar1 = *pfVar1 * fVar4;
  pfVar1[1] = pfVar1[1] * fVar4;
  fVar9 = pfVar1[2];
  pfVar1[2] = fVar9 * fVar4;
  *pfVar2 = pfVar1[1] * pfVar3[2] - fVar9 * fVar4 * pfVar3[1];
  pfVar2[1] = pfVar1[2] * *pfVar3 - *pfVar1 * pfVar3[2];
  pfVar2[2] = *pfVar1 * pfVar3[1] - pfVar1[1] * *pfVar3;
  fVar4 = SQRT(pfVar2[2] * pfVar2[2] + *pfVar2 * *pfVar2 + pfVar2[1] * pfVar2[1]);
  if (0.0f < fVar4) {
    fVar4 = 1.0f / fVar4;
  }
  *pfVar2 = *pfVar2 * fVar4;
  pfVar2[1] = pfVar2[1] * fVar4;
  pfVar2[2] = pfVar2[2] * fVar4;
  param_1->field_0x0 = afStack_30[8];
  param_1->field_0x4 = afStack_30[9];
  param_1->field_0x8 = afStack_30[10];
  param_1->field_0x10 = afStack_30[4];
  param_1->field_0x14 = afStack_30[5];
  param_1->field_0x18 = afStack_30[6];
  param_1->field_0x20 = afStack_30[0];
  param_1->field_0x24 = afStack_30[1];
  param_1->field_0x28 = afStack_30[2];
  param_1->field_0x30 = fVar6;
  param_1->field_0x34 = fVar8;
  param_1->field_0x38 = fVar5;
  param_1->field_0xc = (param_1->field_0xc & 0xfffdffffU) | 3;
  return (long)(int)param_1;
}
