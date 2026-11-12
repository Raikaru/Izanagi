/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00396e80 func_00396e80 src/promoted/code1_0039.c - romwright draft, unreviewed */

int FUN_00396e80(int param_1)

{
  unsigned int *puVar1;
  unsigned char bVar2;
  int iVar3;
  int *piVar4;
  
  piVar4 = (int *)(param_1 + DAT_007646c8);
  bVar2 = *piVar4 != -1;
  if (!bVar2) {
    bVar2 = piVar4[1] != 0;
  }
  if (bVar2) {
    puVar1 = (unsigned int *)piVar4[1];
    iVar3 = 0xc;
    if ((puVar1 != (unsigned int *)0x0) && ((*puVar1 & 1) == 0)) {
      iVar3 = puVar1[1] * 0xc + 0x14;
    }
  }
  else {
    iVar3 = 0;
  }
  return iVar3;
}
