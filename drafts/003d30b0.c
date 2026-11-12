/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d30b0 func_003d30b0 src/promoted/code1_003d.c - romwright draft, unreviewed */

void FUN_003d30b0(int param_1)

{
  unsigned int *puVar1;
  long lVar2;
  
  puVar1 = *(unsigned int **)(param_1 + 0x14);
  lVar2 = FUN_003d2c40();
  if (lVar2 != 0) {
    puVar1[6] = &LAB_0070c260;
    puVar1[7] = 0x20;
    puVar1[5] = 0x114;
    *puVar1 = 0;
    puVar1[1] = 0;
    puVar1[2] = 0;
    puVar1[3] = 0;
    puVar1[4] = 0;
    puVar1[0xca] = 0x34f9;
  }
  return;
}
