/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f6360 func_004f6360 src/sce/rofs_pfs.c - romwright draft, unreviewed */

unsigned long long FUN_004f6360(void)

{
  unsigned long long uVar1;
  
  if ((DAT_00743b58 != 0) && (*(code **)(DAT_00743b58 + 0x2c) != (code *)0x0)) {
    uVar1 = (**(code **)(DAT_00743b58 + 0x2c))();
  }
  else {
    FUN_004f53c0(0x75da28,0xe0,0x75da60,0,0xffffffffffffff35);
    uVar1 = 3;
  }
  return uVar1;
}
