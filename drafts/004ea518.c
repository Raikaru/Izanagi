/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea518 func_004ea518 src/cri/mfci.c - romwright draft, unreviewed */

unsigned int FUN_004ea518(long param_1)

{
  unsigned int uVar1;
  
  if (param_1 == 0) {
    FUN_004e9e68(0,0x75bfc0);
    uVar1 = 0;
  }
  else {
    uVar1 = *(unsigned int *)((int)param_1 + 4);
  }
  return uVar1;
}
