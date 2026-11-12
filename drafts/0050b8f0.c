/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0050b8f0 MWSFLIB_SetErrCode src/promoted/code1_0050.c - romwright draft, unreviewed */

unsigned long long FUN_0050b8f0(unsigned long long param_1)

{
  int iVar1;
  
  iVar1 = FUN_0050b3d0();
  *(int *)(iVar1 + 0x68) = (int)param_1;
  return param_1;
}
