/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dcb40 RtQuatTransformVectors src/promoted/code1_003d.c - romwright draft, unreviewed */

float * FUN_003dcb40(float *param_1,float *param_2,int param_3,float *param_4)

{
  int iVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  
  fVar4 = param_4[3];
  fVar10 = param_4[1];
  iVar1 = 0;
  fVar11 = *param_4;
  fVar8 = param_4[2];
  fVar3 = fVar4 * fVar4 - (fVar8 * fVar8 + fVar11 * fVar11 + fVar10 * fVar10);
  fVar13 = (fVar11 + fVar11) * fVar4;
  fVar12 = (fVar10 + fVar10) * fVar4;
  fVar4 = (fVar8 + fVar8) * fVar4;
  pfVar2 = param_1;
  if (0 < param_3) {
    do {
      fVar9 = param_2[1];
      iVar1 = iVar1 + 1;
      fVar7 = *param_2;
      fVar6 = param_2[2];
      fVar5 = (fVar8 + fVar8) * fVar6 + (fVar11 + fVar11) * fVar7 + (fVar10 + fVar10) * fVar9;
      param_2 = param_2 + 3;
      *pfVar2 = fVar12 * fVar6 - fVar4 * fVar9;
      pfVar2[1] = fVar4 * fVar7 - fVar13 * fVar6;
      pfVar2[2] = fVar13 * fVar9 - fVar12 * fVar7;
      *pfVar2 = fVar11 * fVar5 + *pfVar2 + 0.0f;
      pfVar2[1] = fVar10 * fVar5 + pfVar2[1] + 0.0f;
      pfVar2[2] = fVar8 * fVar5 + pfVar2[2] + 0.0f;
      *pfVar2 = fVar7 * fVar3 + *pfVar2 + 0.0f;
      pfVar2[1] = fVar9 * fVar3 + pfVar2[1] + 0.0f;
      pfVar2[2] = fVar6 * fVar3 + pfVar2[2] + 0.0f;
      pfVar2 = pfVar2 + 3;
    } while (iVar1 < param_3);
  }
  return param_1;
}
