/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea128 mfCiOpen src/cri/mfci.c - romwright draft, unreviewed */

long FUN_004ea128(long param_1,unsigned long long param_2,long param_3)

{
  long lVar1;
  unsigned long long uVar2;
  
  if (param_1 == 0) {
    uVar2 = 0x75be80;
  }
  else if (param_3 != 0) {
    uVar2 = 0x75bea8;
  }
  else {
    lVar1 = FUN_004ea040();
    uVar2 = 0x75bed0;
    if (lVar1 != 0) {
      FUN_00442830((int)lVar1 + 0x1c,param_1);
      FUN_004ea0b8(lVar1);
      return lVar1;
    }
  }
  FUN_004e9e68(0,uVar2);
  return 0;
}
