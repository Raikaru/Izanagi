/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d59d0 func_003d59d0 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned int FUN_003d59d0(float param_1,unsigned long long param_2)

{
  unsigned char bVar1;
  unsigned char bVar2;
  unsigned char bVar3;
  long lVar4;
  int iVar5;
  int *piVar6;
  int iVar7;
  int *piVar8;
  int iVar9;
  float fVar10;
  float fVar11;
  
  bVar3 = 0;
  piVar6 = (int *)param_2;
  bVar2 = 0;
  if (((float)piVar6[5] < (float)piVar6[1]) && ((float)piVar6[1] - param_1 <= (float)piVar6[5])) {
    bVar3 = 1;
  }
  piVar6[1] = (int)((float)piVar6[1] - param_1);
  fVar11 = (float)piVar6[1];
  if (0.0f <= fVar11) {
    iVar9 = piVar6[0xb];
    piVar8 = piVar6 + 0x13;
    while (bVar1 = iVar9 != 0, iVar9 = iVar9 - 1, bVar1) {
      iVar7 = *piVar8;
      if ((float)piVar6[1] < *(float *)(iVar7 + 4)) {
        do {
          piVar8[1] = iVar7;
          *piVar8 = *(int *)*piVar8;
          iVar7 = *piVar8;
        } while ((float)piVar6[1] < *(float *)(iVar7 + 4));
        iVar5 = *piVar6;
      }
      else {
        iVar5 = *piVar6;
      }
      (*(code *)piVar6[0x11])(piVar8,iVar7,piVar8[1],*(unsigned int *)(iVar5 + 0x14));
      piVar8 = (int *)((int)piVar8 + piVar6[9]);
    }
    piVar6[2] = 0;
  }
  else {
    fVar10 = *(float *)(*piVar6 + 0xc);
    while (fVar11 < 0.0f) {
      fVar11 = (float)piVar6[1] + fVar10;
      piVar6[1] = (int)fVar11;
    }
    FUN_003d5840(param_2);
    FUN_003d5bc0(fVar11,param_2);
    bVar3 = (unsigned char)(bVar3 | ((float)piVar6[1] < (float)piVar6[5]));
    bVar2 = 1;
  }
  if (((bVar3) && ((code *)piVar6[3] != (code *)0x0)) &&
     (lVar4 = (*(code *)piVar6[3])(param_2,piVar6[4]), lVar4 == 0)) {
    piVar6[3] = 0;
  }
  if (((bVar2) && ((code *)piVar6[6] != (code *)0x0)) &&
     (lVar4 = (*(code *)piVar6[6])(param_2,piVar6[7]), lVar4 == 0)) {
    piVar6[6] = 0;
  }
  return 1;
}
