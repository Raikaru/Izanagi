/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c2a80 func_003c2a80 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned int FUN_003c2a80(unsigned long long param_1)

{
  int iVar1;
  
  iVar1 = (int)param_1;
  if (*(short *)(iVar1 + 0xe) - 1 >= 1) {
    *(short *)(iVar1 + 0xe) = *(short *)(iVar1 + 0xe) - 1;
  }
  else {
    if (*(int *)(iVar1 + 0x58) != 0) {
      FUN_003e1ea0();
    }
    *(short *)(iVar1 + 0xe) = *(short *)(iVar1 + 0xe) - 1;
    *(short *)(iVar1 + 0xe) = *(short *)(iVar1 + 0xe) + 1;
    *(unsigned short *)(iVar1 + 0xc) = *(unsigned short *)(iVar1 + 0xc) | 0xfff;
    if (*(int *)(iVar1 + 0x54) != 0) {
      FUN_003c5760();
      *(unsigned int *)(iVar1 + 0x54) = 0;
    }
    FUN_003e3c20(0x70afb0,param_1);
    if (*(int *)(iVar1 + 0x5c) != 0) {
      (*DAT_008873ec)();
      *(unsigned int *)(iVar1 + 0x5c) = 0;
    }
    FUN_003c49a0(iVar1 + 0x20);
    *(short *)(iVar1 + 0xe) = *(short *)(iVar1 + 0xe) - 1;
    (*DAT_008873ec)(param_1);
  }
  return 1;
}
