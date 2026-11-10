/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003cdfa0 func_003cdfa0 src/promoted/code1_003c.c - romwright draft, unreviewed */

void FUN_003cdfa0(unsigned long long param_1)

{
  unsigned char bVar1;
  int iVar2;
  int *piVar3;
  int *piVar4;
  int iVar5;
  
  iVar2 = *(int *)param_1;
  iVar5 = iVar2 - 1;
  if (iVar2 != 0) {
    piVar4 = (int *)param_1 + iVar5;
    do {
      piVar3 = (int *)piVar4[1];
      if (piVar3 != (int *)0x0) {
        if ((*piVar3 != 0) || (piVar3[1] != 0)) {
          piVar3[4] = 0;
        }
        else {
          FUN_003cc6e0(piVar3);
          FUN_003e1ea0(piVar3);
        }
      }
      piVar4 = piVar4 + -1;
      bVar1 = iVar5 != 0;
      iVar5 = iVar5 - 1;
    } while (bVar1);
  }
  (*DAT_008873ec)(param_1);
  return;
}
