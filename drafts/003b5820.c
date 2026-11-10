/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b5820 func_003b5820 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b5820_arg4 fn_003b5820_arg4;
struct fn_003b5820_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x10];
    int field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
    unsigned char pad_0x24[0x10];
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
    unsigned char pad_0x3c[0x2c];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
};


void FUN_003b5820(int *param_1,float *param_2,float *param_3,long param_4,fn_003b5820_arg4 *param_5)

{
  float *pfVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  unsigned int *puVar9;
  float *pfVar10;
  float *pfVar11;
  float *pfVar12;
  int iVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  float fVar23;
  float fVar24;
  float fVar25;
  
  iVar13 = 0;
  iVar2 = param_5->field_0x8;
  pfVar10 = (float *)param_5->field_0x4;
  puVar9 = (unsigned int *)param_5->field_0x34;
  pfVar12 = (float *)param_5->field_0x1c;
  pfVar11 = (float *)*param_1;
  iVar3 = param_1[1];
  iVar4 = param_5->field_0x38;
  iVar5 = param_5->field_0x20;
  if (0 < param_4) {
    do {
      fVar14 = (float)FUN_0044b7b0(*puVar9);
      fVar15 = (float)FUN_0044b610(*puVar9);
      pfVar8 = (float *)((int)pfVar11 + iVar3);
      pfVar7 = (float *)((int)pfVar8 + iVar3);
      pfVar6 = (float *)((int)pfVar7 + iVar3);
      iVar13 = iVar13 + 1;
      fVar19 = -fVar14;
      puVar9 = (unsigned int *)((int)puVar9 + iVar4);
      fVar23 = fVar15 * *param_2 + fVar19 * *param_3;
      fVar25 = fVar15 * param_2[1] + fVar19 * param_3[1];
      fVar22 = param_5->field_0x6c;
      fVar20 = param_5->field_0x68;
      fVar19 = fVar15 * param_2[2] + fVar19 * param_3[2];
      fVar21 = fVar14 * *param_2 + fVar15 * *param_3;
      fVar24 = fVar14 * param_2[1] + fVar15 * param_3[1];
      fVar15 = fVar14 * param_2[2] + fVar15 * param_3[2];
      pfVar1 = pfVar12 + 1;
      fVar18 = fVar20 * fVar23 + fVar22 * fVar21;
      fVar17 = *pfVar12 * 0.5f;
      pfVar12 = (float *)((int)pfVar12 + iVar5);
      fVar14 = *pfVar1 * 0.5f;
      fVar23 = fVar23 * fVar17;
      fVar21 = fVar21 * fVar14;
      fVar16 = fVar20 * fVar25 + fVar22 * fVar24;
      fVar20 = fVar20 * fVar19 + fVar22 * fVar15;
      fVar24 = fVar24 * fVar14;
      fVar15 = fVar15 * fVar14;
      fVar25 = fVar25 * fVar17;
      fVar19 = fVar19 * fVar17;
      *pfVar11 = *pfVar10 + fVar18 + (-fVar23 - fVar21);
      pfVar11[1] = pfVar10[1] + fVar16 + (-fVar25 - fVar24);
      pfVar11[2] = pfVar10[2] + fVar20 + (-fVar19 - fVar15);
      pfVar11 = (float *)((int)pfVar6 + iVar3);
      *pfVar8 = *pfVar10 + fVar18 + (fVar23 - fVar21);
      pfVar8[1] = pfVar10[1] + fVar16 + (fVar25 - fVar24);
      pfVar8[2] = pfVar10[2] + fVar20 + (fVar19 - fVar15);
      *pfVar7 = *pfVar10 + fVar18 + fVar23 + fVar21;
      pfVar7[1] = pfVar10[1] + fVar16 + fVar25 + fVar24;
      pfVar7[2] = pfVar10[2] + fVar20 + fVar19 + fVar15;
      *pfVar6 = *pfVar10 + fVar18 + -fVar23 + fVar21;
      pfVar6[1] = pfVar10[1] + fVar16 + -fVar25 + fVar24;
      pfVar1 = pfVar10 + 2;
      pfVar10 = (float *)((int)pfVar10 + iVar2);
      pfVar6[2] = *pfVar1 + fVar20 + -fVar19 + fVar15;
    } while (iVar13 < (int)param_4);
  }
  return;
}
