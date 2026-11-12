/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f2548 func_004f2548 src/sce/rofs_if.c - romwright draft, unreviewed */

void FUN_004f2548(unsigned long long param_1,unsigned long long param_2)

{
  FUN_004f54a0();
  if (DAT_00743a48 != 0) {
    (**(code **)(DAT_00743a48 + 0x34))(param_1,param_2);
    return;
  }
  FUN_004f53c0(0x75d488,0x15a,0x75d520,0,0xffffffffffffff35);
  return;
}
