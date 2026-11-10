/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x005287f0 SFUO_Create src/cri/sfuo.c - romwright draft, unreviewed */

unsigned long long FUN_005287f0(unsigned long long param_1)

{
  int iVar1;
  
  iVar1 = (int)param_1;
  *(int *)(iVar1 + 0x2158) = iVar1 + 0x3678;
  FUN_00528820(param_1,iVar1 + 0x3678,*(unsigned int *)(iVar1 + 0x2160));
  return 0;
}
