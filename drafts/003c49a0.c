/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c49a0 func_003c49a0 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003c49a0(unsigned long long param_1)

{
  unsigned int *puVar1;
  int iVar2;
  unsigned int *puVar3;
  unsigned int *puVar4;
  int iVar5;
  
  puVar3 = (unsigned int *)param_1;
  puVar1 = (unsigned int *)*puVar3;
  if (puVar1 != (unsigned int *)0x0) {
    iVar2 = puVar3[1];
    iVar5 = 0;
    puVar4 = puVar1;
    if (0 < iVar2) {
      do {
        FUN_003c4220(*puVar4);
        iVar5 = iVar5 + 1;
        *puVar4 = 0;
        puVar4 = puVar4 + 1;
      } while (iVar5 < iVar2);
    }
    (*DAT_008873ec)(puVar1);
    *puVar3 = 0;
  }
  puVar3[1] = 0;
  puVar3[2] = 0;
  return param_1;
}
