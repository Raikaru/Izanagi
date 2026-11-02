/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f2798 func_004f2798 src/sce/rofs_if.c - romwright draft, unreviewed */

void FUN_004f2798(unsigned long long param_1,unsigned long long param_2)

{
  FUN_004f54a0();
  if (DAT_00743a48 != 0) {
    (**(code **)(DAT_00743a48 + 0x40))(param_1,param_2);
    return;
  }
  FUN_004f53c0(0x75d488,0x1a8,0x75d570,0,0xffffffffffffff35);
  return;
}
