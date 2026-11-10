/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004222c0 func_004222c0 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned long long FUN_004222c0(unsigned long long param_1)

{
  unsigned long long uVar1;
  unsigned int uVar2;
  
  uVar2 = Status & 0x10000;
  if (uVar2 != 0) {
    FUN_0042ba20();
  }
  uVar1 = FUN_00421530(param_1);
  SYNC(0);
  if (uVar2 != 0) {
    FUN_0042ba70();
  }
  return uVar1;
}
