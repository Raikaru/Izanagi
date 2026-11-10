/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e60e8 func_004e60e8 src/cri/dvci.c - romwright draft, unreviewed */

unsigned int FUN_004e60e8(long param_1)

{
  unsigned int uVar1;
  
  if (param_1 == 0) {
    FUN_004e5a38(0,0x75b1c8);
    uVar1 = 0;
  }
  else {
    uVar1 = *(unsigned int *)((int)param_1 + 0xc);
  }
  return uVar1;
}
