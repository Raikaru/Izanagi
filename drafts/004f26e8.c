/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f26e8 func_004f26e8 src/sce/rofs_if.c - romwright draft, unreviewed */

long FUN_004f26e8(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3)

{
  long lVar1;
  
  FUN_004f54a0();
  if (DAT_00743a48 != 0) {
    lVar1 = (**(code **)(DAT_00743a48 + 0x3c))(param_1,param_2,param_3);
    if (lVar1 != 0) {
      FUN_004f1f58();
    }
    return lVar1;
  }
  lVar1 = FUN_004f53c0(0x75d488,0x199,0x75d560,0,0xffffffffffffff35);
  return lVar1;
}
