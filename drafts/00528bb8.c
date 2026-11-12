/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00528bb8 SFVOM_GetRead src/cri/sfvom.c - romwright draft, unreviewed */

unsigned long long FUN_00528bb8(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3)

{
  unsigned long long uVar1;
  
  if (*(int *)((int)param_1 + 0x48) - 3U >= 2) {
    *(unsigned int *)param_2 = 0;
    uVar1 = 0;
  }
  else {
    uVar1 = FUN_00515870(param_1,*(unsigned int *)((int)param_1 + 0x20d8),param_2,param_3);
  }
  return uVar1;
}
