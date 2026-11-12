/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bb330 func_003bb330 src/promoted/code1_003b.c - romwright draft, unreviewed */

int FUN_003bb330(int param_1)

{
  int iVar1;
  int uStack_8;
  unsigned int uStack_4;
  
  if (*(int *)(param_1 + 0xc) != 2) {
    if (*(int *)(param_1 + 0xc) == 1) {
      iVar1 = *(int *)(param_1 + 8) - 2;
    }
    else {
      uStack_8 = 0x102;
      uStack_4 = FUN_003df590(1);
      FUN_003df4d0(&uStack_8);
      iVar1 = 0;
    }
  }
  else {
    iVar1 = *(int *)(param_1 + 8) - 3;
  }
  return iVar1;
}
