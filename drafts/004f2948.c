/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f2948 func_004f2948 src/sce/rofs_if.c - romwright draft, unreviewed */

unsigned long long FUN_004f2948(unsigned long long param_1)

{
  unsigned long long uVar1;
  
  FUN_004f54a0();
  if (DAT_00743a48 != 0) {
    uVar1 = (**(code **)(DAT_00743a48 + 0x48))(param_1);
  }
  else {
    FUN_004f53c0(0x75d488,0x1d6,0x75d5a0,0,0xffffffffffffff35);
    uVar1 = 0;
  }
  return uVar1;
}
