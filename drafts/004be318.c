/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004be318 func_004be318 src/cri/acssnd.c - romwright draft, unreviewed */

void FUN_004be318(unsigned long long param_1,unsigned long long param_2)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  if (0 < *(int *)((int)param_1 + 0xc)) {
    do {
      iVar1 = iVar2 + 1;
      FUN_004be3a8(param_1,iVar2,param_2);
      iVar2 = iVar1;
    } while (iVar1 < *(int *)((int)param_1 + 0xc));
  }
  return;
}
