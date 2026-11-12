/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e63f0 func_004e63f0 src/cri/dvci.c - romwright draft, unreviewed */

int FUN_004e63f0(long param_1)

{
  int iVar1;
  
  if (param_1 == 0) {
    FUN_004e5a38(0,0x75b1c8);
    iVar1 = 0;
  }
  else if (*(char *)((int)param_1 + 2) != '\x01') {
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)((int)param_1 + 0x14) << 0xb;
  }
  return iVar1;
}
