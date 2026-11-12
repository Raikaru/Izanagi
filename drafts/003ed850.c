/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003ed850 func_003ed850 src/rw/basync.c - romwright draft, unreviewed */

void FUN_003ed850(int param_1,unsigned long long param_2)

{
  int iVar1;
  unsigned long long uVar2;
  unsigned long long uVar3;
  
  for (; param_1 != 0; param_1 = *(int *)(param_1 + 0x9c)) {
    uVar2 = param_2 | *(unsigned char *)(param_1 + 3);
    if ((uVar2 & 4) != 0) {
      FUN_003e05f0(param_1 + 0x50,param_1 + 0x10,*(int *)(param_1 + 4) + 0x50);
      *(unsigned char *)(param_1 + 3) = *(unsigned char *)(param_1 + 3) & 0xfb;
    }
    for (iVar1 = *(int *)(param_1 + 0x98); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x9c)) {
      uVar3 = uVar2 | *(unsigned char *)(iVar1 + 3);
      if ((uVar3 & 4) != 0) {
        FUN_003e05f0(iVar1 + 0x50,iVar1 + 0x10,*(int *)(iVar1 + 4) + 0x50);
        *(unsigned char *)(iVar1 + 3) = *(unsigned char *)(iVar1 + 3) & 0xfb;
      }
      FUN_003ed850(*(unsigned int *)(iVar1 + 0x98),uVar3);
    }
  }
  return;
}
