/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea210 func_004ea210 src/cri/mfci.c - romwright draft, unreviewed */

unsigned int FUN_004ea210(long param_1,int param_2,long param_3)

{
  int iVar1;
  unsigned int uVar2;
  int iVar3;
  int iVar4;
  
  if (param_1 == 0) {
    FUN_004e9e68(0,0x75bf00);
    uVar2 = 0;
  }
  else {
    FUN_004e9e20();
    iVar4 = (int)param_1;
    if (param_3 == 0) {
      *(int *)(iVar4 + 0x10) = param_2;
      iVar3 = *(int *)(iVar4 + 0xc);
    }
    else if (param_3 == 2) {
      iVar3 = *(int *)(iVar4 + 0xc);
      *(int *)(iVar4 + 0x10) = iVar3 + param_2;
    }
    else {
      iVar3 = *(int *)(iVar4 + 0xc);
      if (param_3 == 1) {
        *(int *)(iVar4 + 0x10) = *(int *)(iVar4 + 0x10) + param_2;
      }
    }
    iVar1 = *(int *)(iVar4 + 0x10);
    if (iVar3 <= *(int *)(iVar4 + 0x10)) {
      iVar1 = iVar3;
    }
    if (iVar1 < 0) {
      iVar1 = 0;
    }
    *(int *)(iVar4 + 0x10) = iVar1;
    FUN_004e9e38();
    uVar2 = *(unsigned int *)(iVar4 + 0x10);
  }
  return uVar2;
}
