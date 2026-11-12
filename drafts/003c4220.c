/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c4220 func_003c4220 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned int FUN_003c4220(unsigned long long param_1)

{
  int iVar1;
  
  iVar1 = (int)param_1;
  if (*(short *)(iVar1 + 0x18) != 1) {
    *(short *)(iVar1 + 0x18) = *(short *)(iVar1 + 0x18) - 1;
  }
  else {
    FUN_003e3c20(0x70aff0,param_1);
    FUN_003c42b0(param_1,0);
    (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e0 + DAT_007647d0),param_1);
  }
  return 1;
}
