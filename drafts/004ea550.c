/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea550 func_004ea550 src/cri/pl2enc.c - romwright draft, unreviewed */

void FUN_004ea550(long param_1,long param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  
  if (param_1 == 0) {
    FUN_004e9e68(0,0x75bfd8);
    return;
  }
  iVar3 = (int)param_1;
  iVar2 = (int)param_2;
  iVar1 = *(int *)(iVar3 + 4);
  *(int *)(iVar3 + 4) = iVar2;
  if (param_2 == 0) {
    trap(7);
  }
  *(int *)(iVar3 + 0xc) = ((*(int *)(iVar3 + 8) + iVar2) - 1) / iVar2;
  *(int *)(iVar3 + 0x10) = (*(int *)(iVar3 + 0x10) * iVar1) / iVar2;
  *(int *)(iVar3 + 0x14) = *(int *)(iVar3 + 0x18) * iVar2;
  return;
}
