/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e9420 func_004e9420 src/cri/lsc.c - romwright draft, unreviewed */

unsigned char FUN_004e9420(long param_1)

{
  unsigned char uVar1;
  
  if (param_1 == 0) {
    FUN_004e9878(0x75baa8);
    uVar1 = 0xff;
  }
  else {
    uVar1 = *(unsigned char *)((int)param_1 + 1);
  }
  return uVar1;
}
