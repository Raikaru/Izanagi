/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d5bc0 func_003d5bc0 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned int FUN_003d5bc0(float param_1,unsigned long long param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  unsigned char bVar5;
  unsigned char bVar6;
  long lVar7;
  int iVar8;
  int *piVar9;
  int *piVar10;
  int iVar11;
  float fVar12;
  int iVar13;
  
  if (0.0f < param_1) {
    piVar9 = (int *)param_2;
    iVar11 = *piVar9;
    iVar1 = piVar9[0xb];
    bVar6 = 0;
    fVar12 = *(float *)(iVar11 + 0xc);
    bVar5 = 0;
    if (((float)piVar9[1] < (float)piVar9[5]) && ((float)piVar9[5] <= (float)piVar9[1] + param_1)) {
      bVar5 = 1;
    }
    param_1 = (float)piVar9[1] + param_1;
    piVar9[1] = (int)param_1;
    if (fVar12 < param_1) {
      if (fVar12 < param_1) {
        bVar6 = 1;
        while (fVar12 < param_1) {
          param_1 = (float)piVar9[1] - fVar12;
          piVar9[1] = (int)param_1;
          bVar5 = (unsigned char)(bVar5 | ((float)piVar9[5] <= param_1));
        }
      }
      FUN_003d5840(param_2,iVar11);
      piVar9[1] = (int)param_1;
    }
    if (piVar9[2] == 0) {
      iVar13 = piVar9[1];
      FUN_003d5840(param_2,iVar11);
      piVar9[1] = iVar13;
    }
    iVar13 = *(int *)(iVar11 + 4);
    iVar2 = piVar9[10];
    iVar11 = *(int *)(iVar11 + 0x10);
    iVar8 = 0;
    piVar10 = (int *)piVar9[2];
    while ((piVar10 < (int *)(iVar11 + iVar13 * iVar2) &&
           (*(float *)(*piVar10 + 4) <= (float)piVar9[1]))) {
      do {
        iVar4 = iVar8 * piVar9[9];
        iVar8 = iVar8 + 1;
        if (iVar1 <= iVar8) {
          iVar8 = 0;
        }
        iVar3 = *(int *)((int)piVar9 + iVar4 + 0x50);
      } while (iVar3 != *piVar10);
      *(int *)((int)piVar9 + iVar4 + 0x4c) = iVar3;
      *(int *)((int)piVar9 + iVar4 + 0x50) = piVar9[2];
      piVar9[2] = piVar9[2] + piVar9[10];
      piVar10 = (int *)piVar9[2];
    }
    piVar10 = piVar9 + 0x13;
    iVar11 = 0;
    if (0 < iVar1) {
      do {
        (*(code *)piVar9[0x11])
                  (piVar9[1],piVar10,*piVar10,piVar10[1],*(unsigned int *)(*piVar9 + 0x14));
        iVar11 = iVar11 + 1;
        piVar10 = (int *)((int)piVar10 + piVar9[9]);
      } while (iVar11 < iVar1);
    }
    if (((bVar5) && ((code *)piVar9[3] != (code *)0x0)) &&
       (lVar7 = (*(code *)piVar9[3])(param_2,piVar9[4]), lVar7 == 0)) {
      piVar9[3] = 0;
    }
    if (((bVar6) && ((code *)piVar9[6] != (code *)0x0)) &&
       (lVar7 = (*(code *)piVar9[6])(param_2,piVar9[7]), lVar7 == 0)) {
      piVar9[6] = 0;
    }
  }
  return 1;
}
