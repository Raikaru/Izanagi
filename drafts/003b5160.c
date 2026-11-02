/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b5160 rpPTankGENInsPosNCPSPR src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b5160_arg4 fn_003b5160_arg4;
struct fn_003b5160_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x10];
    int field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
    unsigned char pad_0x24[0x10];
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
};


void FUN_003b5160(int *param_1,float *param_2,float *param_3,int param_4,fn_003b5160_arg4 *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  float *pfVar9;
  float *pfVar10;
  unsigned int *puVar11;
  int iVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  
  iVar12 = 0;
  iVar1 = param_5->field_0x8;
  pfVar8 = (float *)param_5->field_0x4;
  puVar11 = (unsigned int *)param_5->field_0x34;
  pfVar10 = (float *)param_5->field_0x1c;
  pfVar9 = (float *)*param_1;
  iVar2 = param_1[1];
  iVar3 = param_5->field_0x38;
  iVar4 = param_5->field_0x20;
  if (0 < param_4) {
    do {
      fVar14 = (float)FUN_0044b7b0(*puVar11);
      fVar15 = (float)FUN_0044b610(*puVar11);
      pfVar7 = (float *)((int)pfVar9 + iVar2);
      pfVar6 = (float *)((int)pfVar7 + iVar2);
      iVar12 = iVar12 + 1;
      pfVar5 = (float *)((int)pfVar6 + iVar2);
      fVar13 = -fVar14;
      puVar11 = (unsigned int *)((int)puVar11 + iVar3);
      fVar16 = *pfVar10 * 0.5f;
      fVar19 = (fVar15 * *param_2 + fVar13 * *param_3) * fVar16;
      fVar20 = (fVar15 * param_2[1] + fVar13 * param_3[1]) * fVar16;
      fVar16 = (fVar15 * param_2[2] + fVar13 * param_3[2]) * fVar16;
      fVar13 = pfVar10[1] * 0.5f;
      pfVar10 = (float *)((int)pfVar10 + iVar4);
      fVar17 = (fVar14 * *param_2 + fVar15 * *param_3) * fVar13;
      fVar18 = (fVar14 * param_2[1] + fVar15 * param_3[1]) * fVar13;
      fVar13 = (fVar14 * param_2[2] + fVar15 * param_3[2]) * fVar13;
      *pfVar9 = *pfVar8 + (-fVar19 - fVar17);
      pfVar9[1] = pfVar8[1] + (-fVar20 - fVar18);
      pfVar9[2] = pfVar8[2] + (-fVar16 - fVar13);
      pfVar9 = (float *)((int)pfVar5 + iVar2);
      *pfVar7 = *pfVar8 + (fVar19 - fVar17);
      pfVar7[1] = pfVar8[1] + (fVar20 - fVar18);
      pfVar7[2] = pfVar8[2] + (fVar16 - fVar13);
      *pfVar6 = *pfVar8 + fVar19 + fVar17;
      pfVar6[1] = pfVar8[1] + fVar20 + fVar18;
      pfVar6[2] = pfVar8[2] + fVar16 + fVar13;
      *pfVar5 = *pfVar8 + -fVar19 + fVar17;
      pfVar5[1] = pfVar8[1] + -fVar20 + fVar18;
      pfVar6 = pfVar8 + 2;
      pfVar8 = (float *)((int)pfVar8 + iVar1);
      pfVar5[2] = *pfVar6 + -fVar16 + fVar13;
    } while (iVar12 < param_4);
  }
  return;
}
