/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051edd0 func_0051edd0 src/cri/sfd_mpv.c - romwright draft, unreviewed */

unsigned long long FUN_0051edd0(unsigned long long param_1,unsigned long long param_2,unsigned int *param_3)

{
  int iVar1;
  unsigned int uVar2;
  long lVar3;
  long lVar4;
  int iVar5;
  
  iVar5 = (int)param_1;
  lVar3 = FUN_0051f7c8();
  if (lVar3 != 0) {
    FUN_0051ee90(param_1,lVar3,param_2);
    iVar1 = *(int *)param_2;
    *(unsigned int *)(iVar5 + 0xfac) = *(unsigned int *)(iVar1 + 0x14);
    *(unsigned int *)(iVar5 + 0xfb0) = *(unsigned int *)(iVar1 + 0x18);
    lVar4 = FUN_00526b18(param_1,iVar1);
    if (lVar4 != 0) {
      if (*(int *)(iVar5 + 0x58) != 2) {
        return 0;
      }
      uVar2 = FUN_0051fc60(param_1);
      *(unsigned int *)((int)lVar3 + 0x58) = uVar2;
      *param_3 = uVar2;
      return 0;
    }
  }
  *(int *)param_2 = 0;
  return 0;
}
