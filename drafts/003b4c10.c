/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b4c10 func_003b4c10 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b4c10_arg4 fn_003b4c10_arg4;
struct fn_003b4c10_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x5c];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
    float field_0x70; /* +0x70 */
};


void FUN_003b4c10(int *param_1,float *param_2,float *param_3,int param_4,fn_003b4c10_arg4 *param_5)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  int iVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  
  pfVar7 = (float *)param_5->field_0x4;
  iVar1 = param_5->field_0x8;
  fVar13 = param_5->field_0x6c;
  pfVar6 = (float *)*param_1;
  iVar2 = param_1[1];
  fVar12 = param_5->field_0x68;
  iVar8 = 0;
  fVar10 = param_5->field_0x70 * 0.5f;
  fVar16 = fVar12 * *param_2 + fVar13 * *param_3;
  fVar11 = *param_2 * fVar10;
  fVar9 = fVar12 * param_2[1] + fVar13 * param_3[1];
  fVar14 = param_2[2] * fVar10;
  fVar17 = param_2[1] * fVar10;
  fVar15 = param_3[2] * fVar10;
  fVar12 = fVar12 * param_2[2] + fVar13 * param_3[2];
  fVar13 = *param_3 * fVar10;
  fVar10 = param_3[1] * fVar10;
  if (0 < param_4) {
    do {
      pfVar5 = (float *)((int)pfVar6 + iVar2);
      pfVar4 = (float *)((int)pfVar5 + iVar2);
      iVar8 = iVar8 + 1;
      pfVar3 = (float *)((int)pfVar4 + iVar2);
      *pfVar6 = *pfVar7 + fVar16 + (-fVar11 - fVar13);
      pfVar6[1] = pfVar7[1] + fVar9 + (-fVar17 - fVar10);
      pfVar6[2] = pfVar7[2] + fVar12 + (-fVar14 - fVar15);
      pfVar6 = (float *)((int)pfVar3 + iVar2);
      *pfVar5 = *pfVar7 + fVar16 + (fVar11 - fVar13);
      pfVar5[1] = pfVar7[1] + fVar9 + (fVar17 - fVar10);
      pfVar5[2] = pfVar7[2] + fVar12 + (fVar14 - fVar15);
      *pfVar4 = *pfVar7 + fVar16 + fVar11 + fVar13;
      pfVar4[1] = pfVar7[1] + fVar9 + fVar17 + fVar10;
      pfVar4[2] = pfVar7[2] + fVar12 + fVar14 + fVar15;
      *pfVar3 = *pfVar7 + fVar16 + -fVar11 + fVar13;
      pfVar3[1] = pfVar7[1] + fVar9 + -fVar17 + fVar10;
      pfVar4 = pfVar7 + 2;
      pfVar7 = (float *)((int)pfVar7 + iVar1);
      pfVar3[2] = *pfVar4 + fVar12 + -fVar14 + fVar15;
    } while (iVar8 < param_4);
  }
  return;
}
