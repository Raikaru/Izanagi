/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bf498 ACSSND_GetNextAdxt src/cri/acssnd.c - romwright draft, unreviewed */

unsigned long long FUN_004bf498(int param_1)

{
  int iVar1;
  unsigned long long uVar2;
  
  if (*(int *)(param_1 + 0x50) != 0) {
    uVar2 = 0;
    if (*(int *)(param_1 + 0x94) != 1) {
      iVar1 = *(int *)(param_1 + 0x10) + 1;
      if (*(int *)(param_1 + 0xa0) <= iVar1) {
        iVar1 = 0;
      }
      uVar2 = (**(code **)(**(int **)(param_1 + iVar1 * 4) + 0x3c))();
    }
  }
  else {
    FUN_004bd6a0(0x7570d8);
    uVar2 = 0;
  }
  return uVar2;
}
