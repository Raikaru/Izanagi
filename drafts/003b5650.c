/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b5650 func_003b5650 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b5650_arg4 fn_003b5650_arg4;
struct fn_003b5650_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x10];
    int field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
    unsigned char pad_0x24[0x44];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
};


void FUN_003b5650(int *param_1,float *param_2,float *param_3,int param_4,fn_003b5650_arg4 *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  float *pfVar9;
  int iVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  
  fVar15 = param_5->field_0x6c;
  pfVar9 = (float *)param_5->field_0x4;
  iVar1 = param_5->field_0x8;
  fVar17 = param_5->field_0x68;
  pfVar8 = (float *)param_5->field_0x1c;
  iVar2 = param_5->field_0x20;
  pfVar7 = (float *)*param_1;
  iVar3 = param_1[1];
  iVar10 = 0;
  fVar14 = fVar17 * *param_2 + fVar15 * *param_3;
  fVar13 = fVar17 * param_2[1] + fVar15 * param_3[1];
  fVar15 = fVar17 * param_2[2] + fVar15 * param_3[2];
  if (0 < param_4) {
    do {
      pfVar6 = (float *)((int)pfVar7 + iVar3);
      fVar16 = *pfVar8;
      pfVar5 = (float *)((int)pfVar6 + iVar3);
      iVar10 = iVar10 + 1;
      pfVar4 = (float *)((int)pfVar5 + iVar3);
      fVar18 = pfVar8[1] * 0.5f;
      pfVar8 = (float *)((int)pfVar8 + iVar2);
      fVar16 = fVar16 * 0.5f;
      fVar19 = *param_3 * fVar18;
      fVar12 = *param_2 * fVar16;
      fVar11 = param_2[1] * fVar16;
      fVar16 = param_2[2] * fVar16;
      fVar17 = param_3[1] * fVar18;
      fVar18 = param_3[2] * fVar18;
      *pfVar7 = *pfVar9 + fVar14 + (-fVar12 - fVar19);
      pfVar7[1] = pfVar9[1] + fVar13 + (-fVar11 - fVar17);
      pfVar7[2] = pfVar9[2] + fVar15 + (-fVar16 - fVar18);
      pfVar7 = (float *)((int)pfVar4 + iVar3);
      *pfVar6 = *pfVar9 + fVar14 + (fVar12 - fVar19);
      pfVar6[1] = pfVar9[1] + fVar13 + (fVar11 - fVar17);
      pfVar6[2] = pfVar9[2] + fVar15 + (fVar16 - fVar18);
      *pfVar5 = *pfVar9 + fVar14 + fVar12 + fVar19;
      pfVar5[1] = pfVar9[1] + fVar13 + fVar11 + fVar17;
      pfVar5[2] = pfVar9[2] + fVar15 + fVar16 + fVar18;
      *pfVar4 = *pfVar9 + fVar14 + -fVar12 + fVar19;
      pfVar4[1] = pfVar9[1] + fVar13 + -fVar11 + fVar17;
      pfVar5 = pfVar9 + 2;
      pfVar9 = (float *)((int)pfVar9 + iVar1);
      pfVar4[2] = *pfVar5 + fVar15 + -fVar16 + fVar18;
    } while (iVar10 < param_4);
  }
  return;
}
