/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f8648 func_004f8648 src/sce/rofs_if.c - romwright draft, unreviewed */

long FUN_004f8648(void)

{
  long lVar1;
  
  lVar1 = FUN_004f2438();
  if (lVar1 != 1) {
    if (lVar1 == 0) {
      lVar1 = 0;
    }
    else if (lVar1 != 2) {
      lVar1 = 3;
    }
  }
  return lVar1;
}
