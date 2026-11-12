/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f2260 func_004f2260 src/sce/rofs_if.c - romwright draft, unreviewed */

unsigned long long FUN_004f2260(unsigned long long param_1,unsigned long long param_2)

{
  unsigned long long uVar1;
  
  FUN_004f54a0();
  if (DAT_00743a48 != 0) {
    uVar1 = (**(code **)(DAT_00743a48 + 0x28))(param_1,param_2);
  }
  else {
    FUN_004f53c0(0x75d488,0xe0,0x75d500,0,0xffffffffffffff35);
    uVar1 = 0;
  }
  return uVar1;
}
