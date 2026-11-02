/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4ec0 func_004f4ec0 src/sce/rofs_mai.c - romwright draft, unreviewed */

void FUN_004f4ec0(unsigned long long param_1,long param_2,unsigned long long param_3)

{
  long lVar1;
  
  if (param_2 == 0) {
    FUN_004f53c0(0x75d6c8,0x768,0x75d7e0,0,0xffffffffffffff9c);
    return;
  }
  lVar1 = FUN_004f2e80(param_1,1);
  if (lVar1 == 0) {
    FUN_004f53c0(0x75d6c8,0x76e,0x75d798);
    return;
  }
  FUN_004f7e18(param_2,param_3,*(unsigned int *)((int)lVar1 + 0x10),0,0xffffffffffffff97);
  return;
}
