/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4858 RFMAI_GetFileSiz64 src/sce/rofs_mai.c - romwright draft, unreviewed */

long FUN_004f4858(unsigned long long param_1,unsigned long long param_2)

{
  long lVar1;
  unsigned int *apuStack_20 [4];
  
  lVar1 = FUN_004f48a8(param_1,param_2,apuStack_20);
  if (lVar1 == 0) {
    lVar1 = FUN_004f5860(apuStack_20[0][1],*apuStack_20[0]);
    if (lVar1 < 0) {
      lVar1 = 0x7fffffffffffffff;
    }
  }
  return lVar1;
}
