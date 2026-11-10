/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00518d70 func_00518d70 src/cri/sfd_mps.c - romwright draft, unreviewed */

unsigned long long FUN_00518d70(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4)

{
  long lVar1;
  unsigned long long uVar2;
  int uStack_40;
  
  lVar1 = FUN_00516df8();
  if (lVar1 != 0) {
    FUN_00519de8(param_1);
    if (uStack_40 != 0) {
      FUN_00518e10(param_1,0,(int)param_3 - 0x12,(int)param_4 + 0x12);
    }
    uVar2 = 1;
  }
  else {
    uVar2 = FUN_00518e10(param_1,param_2,param_3,param_4);
  }
  return uVar2;
}
