/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0044e7d8 dptofp src/middleware/middleware_inferred_0044e608.c - romwright draft, unreviewed */

void FUN_0044e7d8(unsigned long long param_1)

{
  unsigned long long uVar1;
  int uStack_40;
  unsigned int uStack_3c;
  unsigned int uStack_38;
  unsigned long long uStack_30;
  unsigned long long auStack_20 [2];
  
  auStack_20[0] = param_1;
  FUN_0044ddf0(auStack_20,&uStack_40);
  uVar1 = (long)(uStack_30 << 2) >> 0x20;
  if ((uStack_30 & 0x3fffffff) != 0) {
    uVar1 = uVar1 | 1;
  }
  FUN_0044dca8(uStack_40,uStack_3c,uStack_38,uVar1);
  return;
}
