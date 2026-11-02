/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bfba0 func_004bfba0 src/cri/acssnd.c - romwright draft, unreviewed */

void FUN_004bfba0(unsigned int *param_1)

{
  unsigned int *puVar1;
  unsigned int *puVar2;
  int iVar3;
  
  iVar3 = 1;
  param_1[2] = 0;
  param_1[3] = 0;
  param_1[4] = 0;
  param_1[5] = 0;
  *param_1 = 0;
  puVar1 = param_1;
  puVar2 = param_1 + 6;
  while( 1 ) {
    puVar1 = puVar1 + 1;
    iVar3 = iVar3 - 1;
    FUN_0043f9c8(puVar2,0,0xc);
    if (iVar3 < 0) break;
    *puVar1 = 0;
    puVar2 = puVar2 + 3;
  }
  FUN_0043f9c8(param_1 + 0xc,0,0x20);
  param_1[0x14] = 0;
  param_1[0x15] = 0;
  param_1[0x16] = 0;
  param_1[0x17] = 0;
  param_1[0x18] = 0;
  param_1[0x19] = 0;
  param_1[0x1a] = 0;
  param_1[0x1b] = 0;
  param_1[0x1c] = 0;
  param_1[0x1d] = 0;
  FUN_0043f9c8(param_1 + 0x1e,0,0x28);
  param_1[0x28] = 0;
  param_1[0x2a] = 0x1e;
  param_1[0x2e] = 0x3c;
  param_1[0x33] = 0;
  *(unsigned char *)(param_1 + 0x29) = 0;
  param_1[0x2b] = 0;
  param_1[0x2c] = 0;
  param_1[0x2d] = 0;
  param_1[0x2f] = 0;
  param_1[0x30] = 0;
  param_1[0x31] = 0;
  param_1[0x32] = 0;
  return;
}
