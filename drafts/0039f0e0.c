/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039f0e0 func_0039f0e0 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned int FUN_0039f0e0(int param_1,unsigned int *param_2)

{
  int iVar1;
  unsigned int uVar2;
  
  iVar1 = *(int *)(*(int *)(param_1 + 8) + 0x18);
  if ((*(unsigned int *)(iVar1 + 8) & 0x1000000) == 0) {
    uVar2 = *(unsigned int *)(iVar1 + DAT_00886498);
    FUN_003d0fa0();
    FUN_0039c000(*(unsigned int *)(param_1 + 0x2c),uVar2,*param_2);
  }
  return 1;
}
