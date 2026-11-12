/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00519ad0 func_00519ad0 src/cri/sfd_mps.c - romwright draft, unreviewed */

void FUN_00519ad0(unsigned long long param_1)

{
  long lVar1;
  long lVar2;
  unsigned int *puVar3;
  
  puVar3 = (unsigned int *)((int)param_1 + 0x2198);
  *(unsigned int **)((int)param_1 + 0x1f7c) = puVar3;
  FUN_00519b98(puVar3);
  lVar1 = FUN_00503a68();
  if (lVar1 == 0) {
    FUN_00517ad0(0,0xffffffffff000d08);
    return;
  }
  lVar2 = FUN_005038f8(lVar1,0x519c20,param_1);
  if (lVar2 != 0) {
    FUN_00519c88(lVar1);
    FUN_00517ad0(0,0xffffffffff000d09);
    return;
  }
  *puVar3 = (int)lVar1;
  return;
}
