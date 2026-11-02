/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e6578 func_004e6578 src/cri/dvci.c - romwright draft, unreviewed */

unsigned long long FUN_004e6578(long param_1,long param_2)

{
  unsigned long long uVar1;
  
  if (param_1 == 0) {
    return 0;
  }
  if (param_2 != 0xcb) {
    if (param_2 < 0xcc) {
      if (param_2 == 0xc9) {
        uVar1 = FUN_004e6538();
        return uVar1;
      }
      if (param_2 < 0xca) {
        if (param_2 != 200) {
          return 0;
        }
        uVar1 = FUN_004e6528();
        return uVar1;
      }
    }
    else {
      if (param_2 == 0xcd) {
        uVar1 = FUN_004e6560();
        return uVar1;
      }
      if (0xcc < param_2) {
        if (param_2 != 299) {
          return 0;
        }
        return 1;
      }
    }
    uVar1 = FUN_004e6558();
  }
  else {
    uVar1 = FUN_004e6560();
  }
  return uVar1;
}
