/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b53b0 func_003b53b0 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b53b0_arg4 fn_003b53b0_arg4;
struct fn_003b53b0_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x28];
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
    unsigned char pad_0x3c[0x2c];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
    float field_0x70; /* +0x70 */
    float field_0x74; /* +0x74 */
};


void FUN_003b53b0(int *param_1,float *param_2,float *param_3,int param_4,fn_003b53b0_arg4 *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  unsigned int *puVar8;
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
  float fVar20;
  float fVar21;
  float fVar22;
  
  iVar10 = 0;
  iVar1 = param_5->field_0x8;
  pfVar9 = (float *)param_5->field_0x4;
  puVar8 = (unsigned int *)param_5->field_0x34;
  pfVar7 = (float *)*param_1;
  iVar2 = param_1[1];
  fVar22 = param_5->field_0x70 * 0.5f;
  iVar3 = param_5->field_0x38;
  fVar21 = param_5->field_0x74 * 0.5f;
  if (0 < param_4) {
    do {
      fVar11 = (float)FUN_0044b7b0(*puVar8);
      fVar12 = (float)FUN_0044b610(*puVar8);
      iVar10 = iVar10 + 1;
      pfVar6 = (float *)((int)pfVar7 + iVar2);
      fVar14 = -fVar11;
      puVar8 = (unsigned int *)((int)puVar8 + iVar3);
      fVar18 = fVar12 * *param_2 + fVar14 * *param_3;
      pfVar4 = (float *)((int)pfVar6 + iVar2);
      pfVar5 = (float *)((int)pfVar4 + iVar2);
      fVar16 = fVar12 * param_2[1] + fVar14 * param_3[1];
      fVar15 = param_5->field_0x6c;
      fVar20 = fVar18 * fVar22;
      fVar17 = fVar12 * param_2[2] + fVar14 * param_3[2];
      fVar13 = fVar11 * *param_2 + fVar12 * *param_3;
      fVar14 = fVar11 * param_2[1] + fVar12 * param_3[1];
      fVar12 = fVar11 * param_2[2] + fVar12 * param_3[2];
      fVar11 = param_5->field_0x68;
      fVar19 = fVar13 * fVar21;
      fVar18 = fVar11 * fVar18 + fVar15 * fVar13;
      fVar13 = fVar11 * fVar16 + fVar15 * fVar14;
      fVar11 = fVar11 * fVar17 + fVar15 * fVar12;
      fVar17 = fVar17 * fVar22;
      fVar16 = fVar16 * fVar22;
      fVar12 = fVar12 * fVar21;
      *pfVar7 = *pfVar9 + fVar18 + (-fVar20 - fVar19);
      fVar14 = fVar14 * fVar21;
      pfVar7[1] = pfVar9[1] + fVar13 + (-fVar16 - fVar14);
      pfVar7[2] = pfVar9[2] + fVar11 + (-fVar17 - fVar12);
      pfVar7 = (float *)((int)pfVar5 + iVar2);
      *pfVar6 = *pfVar9 + fVar18 + (fVar20 - fVar19);
      pfVar6[1] = pfVar9[1] + fVar13 + (fVar16 - fVar14);
      pfVar6[2] = pfVar9[2] + fVar11 + (fVar17 - fVar12);
      *pfVar4 = *pfVar9 + fVar18 + fVar20 + fVar19;
      pfVar4[1] = pfVar9[1] + fVar13 + fVar16 + fVar14;
      pfVar4[2] = pfVar9[2] + fVar11 + fVar17 + fVar12;
      *pfVar5 = *pfVar9 + fVar18 + -fVar20 + fVar19;
      pfVar5[1] = pfVar9[1] + fVar13 + -fVar16 + fVar14;
      pfVar4 = pfVar9 + 2;
      pfVar9 = (float *)((int)pfVar9 + iVar1);
      pfVar5[2] = *pfVar4 + fVar11 + -fVar17 + fVar12;
    } while (iVar10 < param_4);
  }
  return;
}
