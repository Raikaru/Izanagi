/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00430e28 func_00430e28 src/promoted/code1_0043.c - romwright draft, unreviewed */

unsigned int FUN_00430e28(void)

{
  int iVar1;
  long lVar2;
  
  FUN_00424f28(0);
  do {
    lVar2 = FUN_004256d0(0x8952c0,0xffffffff80000701,0);
    iVar1 = 10000;
    if (lVar2 < 0) {
      FUN_004244c8(0x754848);
      return 0xffffffff;
    }
    do {
      iVar1 = iVar1 - 1;
    } while (iVar1 != -1);
  } while (DAT_008952e4 == 0);
  DAT_0070f8c8 = 0;
  DAT_0070f8cc = 0;
  FUN_00421a60(0);
  return 0;
}
