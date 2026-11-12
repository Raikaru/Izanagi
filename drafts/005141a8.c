/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x005141a8 func_005141a8 src/cri/sfd_aoap.c - romwright draft, unreviewed */

unsigned long long FUN_005141a8(unsigned long long param_1)

{
  long lVar1;
  int iVar2;
  
  lVar1 = FUN_00525098(param_1,6);
  iVar2 = (int)param_1 + 0x365c;
  if (lVar1 != 0) {
    *(int *)((int)param_1 + 0x2114) = iVar2;
    FUN_005141e8(iVar2);
  }
  return 0;
}
