/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00524d90 func_00524d90 src/cri/sfd_set.c - romwright draft, unreviewed */

unsigned long long FUN_00524d90(long param_1,unsigned long long param_2,unsigned long long param_3)

{
  long lVar1;
  unsigned long long uVar2;
  
  if (param_1 == 0) {
    FUN_00524e48(param_2,param_3);
    *(int *)(&LAB_007458c0 + (int)param_2 * 4) = (int)param_3;
  }
  else {
    lVar1 = FUN_00517cf0();
    if (lVar1 != 0) {
      uVar2 = FUN_00517ad0(0,0xffffffffff000112);
      return uVar2;
    }
    FUN_00524ed0(param_1,param_2,param_3);
    FUN_00524f20(param_1,param_2,param_3);
  }
  return 0;
}
