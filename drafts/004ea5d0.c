/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea5d0 func_004ea5d0 src/cri/pl2enc.c - romwright draft, unreviewed */

unsigned int FUN_004ea5d0(long param_1)

{
  unsigned int uVar1;
  
  if (param_1 == 0) {
    FUN_004e9e68(0,0x75bfa8);
    uVar1 = 0;
  }
  else {
    uVar1 = *(unsigned int *)((int)param_1 + 0x14);
  }
  return uVar1;
}
