/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e63b0 func_004e63b0 src/cri/dvci.c - romwright draft, unreviewed */

unsigned char FUN_004e63b0(long param_1)

{
  unsigned char uVar1;
  
  if (param_1 == 0) {
    FUN_004e5a38(0,0x75b1c8);
    uVar1 = 0;
  }
  else {
    uVar1 = *(unsigned char *)((int)param_1 + 2);
  }
  return uVar1;
}
