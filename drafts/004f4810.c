/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4810 RFMAI_GetFileSiz src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned int FUN_004f4810(void)

{
  long lVar1;
  
  lVar1 = FUN_004f4858();
  if ((-1 < lVar1) && (0x7fffffff < lVar1)) {
    lVar1 = 0x7fffffff;
  }
  return (int)lVar1;
}
