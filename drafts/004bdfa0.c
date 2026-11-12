/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bdfa0 func_004bdfa0 src/cri/acssbadx.c - romwright draft, unreviewed */

unsigned int FUN_004bdfa0(int param_1)

{
  unsigned long long uVar1;
  unsigned int uVar2;
  
  uVar1 = FUN_004d43f8(*(unsigned int *)(param_1 + 4));
  switch(uVar1) {
  case 0:
    uVar2 = 0;
    break;
  case 1:
  case 2:
    uVar2 = 1;
    break;
  case 3:
  case 4:
    uVar2 = 2;
    break;
  case 5:
    uVar2 = 3;
    break;
  case 6:
    uVar2 = 4;
    break;
  default:
    uVar2 = 4;
  }
  return uVar2;
}
