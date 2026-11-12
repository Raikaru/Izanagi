/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039ae90 func_0039ae90 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_0039ae90(unsigned long long param_1,int param_2)

{
  int iVar1;
  int *piVar2;
  int *piVar3;
  int iVar4;
  
  piVar2 = (int *)(param_2 + DAT_007646e0);
  piVar3 = (int *)((int)param_1 + DAT_007646e0);
  if (0 < *piVar2) {
    iVar4 = *piVar2 * 0x14;
    iVar1 = (*DAT_008873e8)(iVar4,0x30105);
    piVar3[2] = iVar1;
    if (piVar3[2] == 0) {
      param_1 = 0;
    }
    else {
      iVar1 = piVar2[2];
      *piVar3 = *piVar2;
      FUN_0043f810(piVar3[2],iVar1,iVar4);
    }
  }
  return param_1;
}
