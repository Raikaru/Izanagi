/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea690 mfCiOptFn1 src/cri/pl2enc.c - romwright draft, unreviewed */

unsigned long long FUN_004ea690(long param_1,long param_2)

{
  unsigned long long uVar1;
  
  if (param_1 == 0) {
    return 0;
  }
  if (param_2 != 0xcb) {
    if (param_2 < 0xcc) {
      if (param_2 != 0xc9) {
        return 0;
      }
      uVar1 = FUN_004ea5d0();
      return uVar1;
    }
    if (param_2 != 0xcd) {
      return 0;
    }
  }
  uVar1 = FUN_004ea020();
  return uVar1;
}
