/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bf510 ACSSND_GetAdxt src/cri/acssnd.c - romwright draft, unreviewed */

unsigned long long FUN_004bf510(int param_1,int param_2)

{
  unsigned long long uVar1;
  
  if (*(int *)(param_1 + 0x50) != 0) {
    uVar1 = 0;
    if (((*(int *)(param_1 + 0x94) != 1) && (-1 < param_2)) &&
       (uVar1 = 0, param_2 < *(int *)(param_1 + 0xa0))) {
      uVar1 = (**(code **)(**(int **)(param_1 + param_2 * 4) + 0x3c))();
    }
  }
  else {
    FUN_004bd6a0(0x757110);
    uVar1 = 0;
  }
  return uVar1;
}
