/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0042f548 func_0042f548 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_0042f548(void)

{
  unsigned int uVar1;
  long lVar2;
  
  lVar2 = FUN_0042e8d8(0x1e);
  if (lVar2 == 0) {
    uVar1 = 0;
  }
  else {
    DAT_0070dd20 = 8;
    lVar2 = FUN_004258b0(0x70f840,0x16,0,0,0,0x70eec0,4,0);
    uVar1 = uRam2070eec0;
    if (lVar2 < 0) {
      FUN_00421800(DAT_0070dcec);
      DAT_0070dd20 = 0;
      uVar1 = 0;
    }
    else {
      DAT_0070dd20 = 0;
      FUN_00421800(DAT_0070dcec);
    }
  }
  return uVar1;
}
