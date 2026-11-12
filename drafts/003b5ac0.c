/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b5ac0 func_003b5ac0 src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b5ac0(int *param_1,unsigned long long param_2,unsigned long long param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  float *pfVar3;
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
  
  pfVar8 = (float *)*param_1;
  iVar1 = param_1[1];
  pfVar7 = *(float **)(param_5 + 0xc);
  iVar2 = *(int *)(param_5 + 0x10);
  iVar9 = 0;
  if (0 < param_4) {
    do {
      pfVar5 = (float *)((int)pfVar8 + iVar1);
      pfVar4 = (float *)((int)pfVar5 + iVar1);
      iVar9 = iVar9 + 1;
      pfVar6 = pfVar7 + 0xc;
      pfVar3 = (float *)((int)pfVar4 + iVar1);
      fVar14 = *pfVar7 * 0.5f + pfVar7[4] * 0.5f;
      fVar13 = *pfVar7 * 0.5f - pfVar7[4] * 0.5f;
      fVar15 = pfVar7[1] * 0.5f + pfVar7[5] * 0.5f;
      fVar10 = pfVar7[1] * 0.5f - pfVar7[5] * 0.5f;
      fVar12 = pfVar7[2] * 0.5f + pfVar7[6] * 0.5f;
      fVar11 = pfVar7[2] * 0.5f - pfVar7[6] * 0.5f;
      *pfVar8 = *pfVar6 + fVar13;
      pfVar8[1] = pfVar7[0xd] + fVar10;
      pfVar8[2] = pfVar7[0xe] + fVar11;
      pfVar8 = (float *)((int)pfVar3 + iVar1);
      *pfVar5 = *pfVar6 - fVar14;
      pfVar5[1] = pfVar7[0xd] - fVar15;
      pfVar5[2] = pfVar7[0xe] - fVar12;
      *pfVar4 = *pfVar6 - fVar13;
      pfVar4[1] = pfVar7[0xd] - fVar10;
      pfVar4[2] = pfVar7[0xe] - fVar11;
      *pfVar3 = *pfVar6 + fVar14;
      pfVar3[1] = pfVar7[0xd] + fVar15;
      pfVar3[2] = pfVar7[0xe] + fVar12;
      pfVar7 = (float *)((int)pfVar7 + iVar2);
    } while (iVar9 < param_4);
  }
  return;
}
