/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x005189a0 func_005189a0 src/cri/sfd_mps.c - romwright draft, unreviewed */

unsigned long long
FUN_005189a0(unsigned long long param_1,int param_2,unsigned long long param_3,unsigned long long param_4,long param_5)

{
  int iVar1;
  unsigned int uVar2;
  long lVar3;
  long lVar4;
  unsigned long long uVar5;
  int iVar6;
  int iVar7;
  
  lVar3 = FUN_00525098(param_1,6);
  iVar7 = (int)param_1;
  if (lVar3 == 0) {
    return 1;
  }
  iVar1 = *(int *)(iVar7 + 0x1f7c);
  if (*(int *)(iVar1 + 0x38) == -1) {
    *(int *)(iVar1 + 0x38) = param_2;
  }
  if (*(int *)(iVar1 + 0x30) == -1) {
    *(int *)(iVar1 + 0x30) = param_2;
  }
  lVar3 = FUN_00525098(param_1,0x1e);
  if (lVar3 != -1) {
    lVar4 = FUN_00525098(param_1,0x37);
    if (lVar4 == 0) {
      if (param_2 != *(int *)(iVar1 + 0x30)) {
        iVar6 = *(int *)(iVar1 + 0x38);
      }
      else {
        *(int *)(iVar1 + 0x38) = (int)lVar3;
        iVar6 = *(int *)(iVar1 + 0x38);
      }
    }
    else if (*(int *)(iVar1 + 0x28) > param_2) {
      *(int *)(iVar1 + 0x38) = (int)lVar3;
      iVar6 = *(int *)(iVar1 + 0x38);
    }
    else {
      iVar6 = *(int *)(iVar1 + 0x38);
    }
  }
  else {
    iVar6 = *(int *)(iVar1 + 0x38);
  }
  *(int *)(iVar1 + 0x28) = param_2;
  if (iVar6 != param_2) {
    return 1;
  }
  if (param_5 < 0) {
    uVar2 = *(unsigned int *)(iVar7 + 0x1f8c);
  }
  else {
    lVar3 = *(long *)(iVar1 + 0x10);
    if (param_5 < *(long *)(iVar1 + 0x10)) {
      lVar3 = param_5;
    }
    lVar4 = *(long *)(iVar1 + 0x18);
    if (param_5 < *(long *)(iVar1 + 0x18)) {
      lVar4 = param_5;
    }
    *(long *)(iVar1 + 0x10) = lVar3;
    *(long *)(iVar1 + 0x18) = lVar4;
    uVar2 = *(unsigned int *)(iVar7 + 0x1f8c);
  }
  uVar5 = FUN_00519008(param_1,uVar2,param_3,param_4,param_5);
  return uVar5;
}
