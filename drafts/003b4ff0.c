/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b4ff0 func_003b4ff0 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b4ff0_arg4 fn_003b4ff0_arg4;
struct fn_003b4ff0_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x10];
    int field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
};


void FUN_003b4ff0(int *param_1,float *param_2,float *param_3,int param_4,fn_003b4ff0_arg4 *param_5)

{
  float *pfVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  float *pfVar9;
  float *pfVar10;
  int iVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  
  pfVar10 = (float *)param_5->field_0x4;
  iVar2 = param_5->field_0x8;
  pfVar9 = (float *)param_5->field_0x1c;
  iVar3 = param_5->field_0x20;
  pfVar8 = (float *)*param_1;
  iVar4 = param_1[1];
  iVar11 = 0;
  if (0 < param_4) {
    do {
      pfVar7 = (float *)((int)pfVar8 + iVar4);
      pfVar1 = pfVar9 + 1;
      pfVar6 = (float *)((int)pfVar7 + iVar4);
      iVar11 = iVar11 + 1;
      pfVar5 = (float *)((int)pfVar6 + iVar4);
      fVar14 = *pfVar9 * 0.5f;
      pfVar9 = (float *)((int)pfVar9 + iVar3);
      fVar16 = *pfVar1 * 0.5f;
      fVar18 = *param_2 * fVar14;
      fVar13 = param_3[1];
      fVar12 = param_2[1] * fVar14;
      fVar14 = param_2[2] * fVar14;
      fVar17 = *param_3 * fVar16;
      fVar15 = param_3[2] * fVar16;
      *pfVar8 = *pfVar10 + (-fVar18 - fVar17);
      fVar13 = fVar13 * fVar16;
      pfVar8[1] = pfVar10[1] + (-fVar12 - fVar13);
      pfVar8[2] = pfVar10[2] + (-fVar14 - fVar15);
      pfVar8 = (float *)((int)pfVar5 + iVar4);
      *pfVar7 = *pfVar10 + (fVar18 - fVar17);
      pfVar7[1] = pfVar10[1] + (fVar12 - fVar13);
      pfVar7[2] = pfVar10[2] + (fVar14 - fVar15);
      *pfVar6 = *pfVar10 + fVar18 + fVar17;
      pfVar6[1] = pfVar10[1] + fVar12 + fVar13;
      pfVar6[2] = pfVar10[2] + fVar14 + fVar15;
      *pfVar5 = *pfVar10 + -fVar18 + fVar17;
      pfVar5[1] = pfVar10[1] + -fVar12 + fVar13;
      pfVar1 = pfVar10 + 2;
      pfVar10 = (float *)((int)pfVar10 + iVar2);
      pfVar5[2] = *pfVar1 + -fVar14 + fVar15;
    } while (iVar11 < param_4);
  }
  return;
}
