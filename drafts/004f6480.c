/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f6480 func_004f6480 src/sce/rofs_pfs.c - romwright draft, unreviewed */

long FUN_004f6480(void)

{
  long lVar1;
  
  if (DAT_00743b58 != 0) {
    if (*(code **)(DAT_00743b58 + 0x68) != (code *)0x0) {
      lVar1 = (**(code **)(DAT_00743b58 + 0x68))();
    }
    else {
      lVar1 = FUN_004f6420();
    }
    if (-1 < lVar1) {
      return lVar1;
    }
  }
  lVar1 = FUN_004f53c0(0x75da28,0x11c,0x75da90,0,0xffffffffffffff35);
  return lVar1;
}
