/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b4db0 func_003b4db0 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b4db0_arg4 fn_003b4db0_arg4;
struct fn_003b4db0_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x28];
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
    unsigned char pad_0x3c[0x34];
    float field_0x70; /* +0x70 */
    float field_0x74; /* +0x74 */
};


void FUN_003b4db0(int *param_1,float *param_2,float *param_3,int param_4,fn_003b4db0_arg4 *param_5)

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
  
  iVar10 = 0;
  iVar1 = param_5->field_0x38;
  pfVar9 = (float *)param_5->field_0x4;
  iVar2 = param_5->field_0x8;
  puVar8 = (unsigned int *)param_5->field_0x34;
  pfVar7 = (float *)*param_1;
  iVar3 = param_1[1];
  fVar20 = param_5->field_0x70 * 0.5f;
  fVar19 = param_5->field_0x74 * 0.5f;
  if (0 < param_4) {
    do {
      fVar11 = (float)FUN_0044b7b0(*puVar8);
      fVar12 = (float)FUN_0044b610(*puVar8);
      pfVar6 = (float *)((int)pfVar7 + iVar3);
      pfVar5 = (float *)((int)pfVar6 + iVar3);
      fVar13 = -fVar11;
      pfVar4 = (float *)((int)pfVar5 + iVar3);
      iVar10 = iVar10 + 1;
      fVar16 = (fVar12 * *param_2 + fVar13 * *param_3) * fVar20;
      puVar8 = (unsigned int *)((int)puVar8 + iVar1);
      fVar18 = (fVar12 * param_2[1] + fVar13 * param_3[1]) * fVar20;
      fVar15 = (fVar11 * param_2[1] + fVar12 * param_3[1]) * fVar19;
      fVar14 = (fVar11 * *param_2 + fVar12 * *param_3) * fVar19;
      fVar17 = (fVar11 * param_2[2] + fVar12 * param_3[2]) * fVar19;
      fVar11 = (fVar12 * param_2[2] + fVar13 * param_3[2]) * fVar20;
      *pfVar7 = *pfVar9 + (-fVar16 - fVar14);
      pfVar7[1] = pfVar9[1] + (-fVar18 - fVar15);
      pfVar7[2] = pfVar9[2] + (-fVar11 - fVar17);
      pfVar7 = (float *)((int)pfVar4 + iVar3);
      *pfVar6 = *pfVar9 + (fVar16 - fVar14);
      pfVar6[1] = pfVar9[1] + (fVar18 - fVar15);
      pfVar6[2] = pfVar9[2] + (fVar11 - fVar17);
      *pfVar5 = *pfVar9 + fVar16 + fVar14;
      pfVar5[1] = pfVar9[1] + fVar18 + fVar15;
      pfVar5[2] = pfVar9[2] + fVar11 + fVar17;
      *pfVar4 = *pfVar9 + -fVar16 + fVar14;
      pfVar4[1] = pfVar9[1] + -fVar18 + fVar15;
      pfVar5 = pfVar9 + 2;
      pfVar9 = (float *)((int)pfVar9 + iVar2);
      pfVar4[2] = *pfVar5 + -fVar11 + fVar17;
    } while (iVar10 < param_4);
  }
  return;
}
