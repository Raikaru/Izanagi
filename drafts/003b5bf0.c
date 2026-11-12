/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b5bf0 func_003b5bf0 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003b5bf0_arg4 fn_003b5bf0_arg4;
struct fn_003b5bf0_arg4 {
    unsigned char pad_0x0[0xc];
    int field_0xc; /* +0xc */
    int field_0x10; /* +0x10 */
    unsigned char pad_0x14[0x54];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
};


void FUN_003b5bf0(int *param_1,unsigned long long param_2,unsigned long long param_3,int param_4,
                 fn_003b5bf0_arg4 *param_5)

{
  float *pfVar1;
  int iVar2;
  int iVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  int iVar9;
  float fVar10;
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
  float fVar23;
  
  fVar11 = param_5->field_0x68;
  pfVar7 = (float *)param_5->field_0xc;
  fVar10 = param_5->field_0x6c;
  pfVar8 = (float *)*param_1;
  iVar2 = param_1[1];
  iVar3 = param_5->field_0x10;
  iVar9 = 0;
  if (0 < param_4) {
    do {
      pfVar6 = (float *)((int)pfVar8 + iVar2);
      fVar14 = pfVar7[4];
      pfVar5 = (float *)((int)pfVar6 + iVar2);
      fVar12 = *pfVar7;
      iVar9 = iVar9 + 1;
      fVar23 = pfVar7[5];
      pfVar4 = (float *)((int)pfVar5 + iVar2);
      fVar18 = pfVar7[1];
      fVar21 = pfVar7[6];
      fVar15 = pfVar7[2];
      fVar16 = (fVar12 - fVar14) * 0.5f;
      fVar20 = (fVar18 + fVar23) * 0.5f;
      fVar19 = (fVar15 + fVar21) * 0.5f;
      fVar13 = (fVar18 - fVar23) * 0.5f;
      fVar22 = (fVar12 + fVar14) * 0.5f;
      fVar17 = fVar14 * fVar10 + fVar12 * fVar11 + pfVar7[0xc] + 0.0f;
      fVar14 = (fVar15 - fVar21) * 0.5f;
      pfVar1 = pfVar7 + 0xd;
      fVar12 = pfVar7[0xe];
      pfVar7 = (float *)((int)pfVar7 + iVar3);
      fVar18 = fVar23 * fVar10 + fVar18 * fVar11 + *pfVar1 + 0.0f;
      *pfVar8 = fVar17 + fVar16;
      fVar12 = fVar21 * fVar10 + fVar15 * fVar11 + fVar12 + 0.0f;
      pfVar8[1] = fVar18 + fVar13;
      pfVar8[2] = fVar12 + fVar14;
      pfVar8 = (float *)((int)pfVar4 + iVar2);
      *pfVar6 = fVar17 - fVar22;
      pfVar6[1] = fVar18 - fVar20;
      pfVar6[2] = fVar12 - fVar19;
      *pfVar5 = fVar17 - fVar16;
      pfVar5[1] = fVar18 - fVar13;
      pfVar5[2] = fVar12 - fVar14;
      *pfVar4 = fVar17 + fVar22;
      pfVar4[1] = fVar18 + fVar20;
      pfVar4[2] = fVar12 + fVar19;
    } while (iVar9 < param_4);
  }
  return;
}
