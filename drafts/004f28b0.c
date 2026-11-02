/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f28b0 func_004f28b0 src/sce/rofs_if.c - romwright draft, unreviewed */

void FUN_004f28b0(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3)

{
  FUN_004f54a0();
  if (DAT_00743a48 != 0) {
    (**(code **)(DAT_00743a48 + 0x50))(param_1,param_2,param_3);
    return;
  }
  FUN_004f53c0(0x75d488,0x1c7,0x75d590,0,0xffffffffffffff35);
  return;
}
