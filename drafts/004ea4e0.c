/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea4e0 func_004ea4e0 src/cri/mfci.c - romwright draft, unreviewed */

unsigned char FUN_004ea4e0(long param_1)

{
  unsigned char uVar1;
  
  if (param_1 == 0) {
    FUN_004e9e68(0,0x75bfa8);
    uVar1 = 0;
  }
  else {
    uVar1 = *(unsigned char *)((int)param_1 + 1);
  }
  return uVar1;
}
