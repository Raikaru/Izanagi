/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00528570 SFD_SetUsrSj src/cri/sfd_tst.c - romwright draft, unreviewed */

unsigned long long FUN_00528570(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4)

{
  int iVar1;
  long lVar2;
  unsigned long long uVar3;
  int iVar4;
  
  lVar2 = FUN_00517cf0();
  if (lVar2 != 0) {
    uVar3 = FUN_00517ad0(0,0xffffffffff000191);
    return uVar3;
  }
  iVar1 = *(int *)((int)param_1 + 0x2160);
  if (iVar1 == 8) {
    uVar3 = FUN_00517ad0(param_1,0xffffffffff000602);
    return uVar3;
  }
  iVar4 = *(int *)((int)param_1 + 0x2158) + (int)param_2 * 0x10 + 4;
  FUN_005288c0(iVar4,param_3,param_4,0,0);
  FUN_00514bd8(param_1,iVar1,param_2,iVar4);
  return 0;
}
