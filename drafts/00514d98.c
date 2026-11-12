/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00514d98 func_00514d98 src/cri/sfd_buf.c - romwright draft, unreviewed */

unsigned long long FUN_00514d98(unsigned long long param_1,long param_2,long param_3,long param_4)

{
  int iVar1;
  unsigned long long uVar2;
  long lVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  
  uVar2 = 0;
  iVar4 = (int)param_1 + 0x1310 + (int)param_2 * 0x74;
  iVar1 = *(int *)(iVar4 + 0x14);
  if (((param_3 != 0) && (uVar2 = 0, *(int *)(iVar4 + 4) != 0)) && (iVar1 != 0)) {
    lVar3 = FUN_00515070(iVar1,param_4);
    iVar6 = (int)param_3;
    uVar2 = 0;
    if ((lVar3 < param_3) &&
       (iVar5 = iVar6 - (int)lVar3, iVar1 = FUN_00515070(iVar1,param_4,iVar5), iVar1 < iVar5)) {
      uVar2 = FUN_00517ad0(param_1,0xffffffffff00040b);
    }
    if (param_4 != 1) {
      if (-1 < *(int *)(iVar4 + 0x30)) {
        *(int *)(iVar4 + 0x30) = *(int *)(iVar4 + 0x30) + iVar6;
      }
    }
    else {
      if (param_2 != 1) {
        iVar1 = *(int *)(iVar4 + 0x34);
      }
      else {
        FUN_00514ee8(iVar4 + 0x10);
        iVar1 = *(int *)(iVar4 + 0x34);
      }
      if (-1 < iVar1) {
        *(int *)(iVar4 + 0x34) = iVar1 + iVar6;
      }
    }
    *(unsigned int *)((int)param_1 + 0x44) = 1;
  }
  return uVar2;
}
