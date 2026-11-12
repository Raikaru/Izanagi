/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f85c0 func_004f85c0 src/sce/rofs_if.c - romwright draft, unreviewed */

unsigned long long FUN_004f85c0(unsigned long long param_1,unsigned long long param_2,long param_3)

{
  unsigned long long uVar1;
  
  uVar1 = 1;
  if (((param_3 != 1) && (uVar1 = 0, param_3 != 0)) && (uVar1 = 2, param_3 != 2)) {
    return 0xffffffffffffff33;
  }
  uVar1 = FUN_004f18a0(param_1,param_2,uVar1);
  return uVar1;
}
