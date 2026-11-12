/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0042e7c8 func_0042e7c8 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_0042e7c8(long param_1)

{
  unsigned int uVar1;
  long lVar2;
  
  if (param_1 == 0) {
    if (0 < DAT_0070dcd0) {
      FUN_004244c8(0x7545d0);
    }
    while ((DAT_0070dcf4 != 0 || (lVar2 = FUN_00425ab0(0x70ee90), lVar2 != 0))) {
      FUN_00422858(4000);
    }
    return 0;
  }
  uVar1 = 1;
  if (DAT_0070dcf4 == 0) {
    lVar2 = FUN_00425ab0(0x70ee90);
    uVar1 = 1;
    if (lVar2 == 0) {
      uVar1 = 0;
    }
  }
  return uVar1;
}
