/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f6420 func_004f6420 src/sce/rofs_pfs.c - romwright draft, unreviewed */

void FUN_004f6420(void)

{
  if ((DAT_00743b58 != 0) && (*(code **)(DAT_00743b58 + 0x38) != (code *)0x0)) {
    (**(code **)(DAT_00743b58 + 0x38))();
    return;
  }
  FUN_004f53c0(0x75da28,0xfd,0x75da80,0,0xffffffffffffff35);
  return;
}
