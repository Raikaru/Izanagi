/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d7c50 func_003d7c50 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned int FUN_003d7c50(unsigned long long param_1)

{
  int iVar1;
  
  iVar1 = *(int *)((int)param_1 + 0x14);
  *(int *)(iVar1 + 0x40) = *(int *)(iVar1 + 0x40) - 1;
  if (*(int *)(iVar1 + 0x40) == 0) {
    (*DAT_008873fc)(DAT_00764838,*(unsigned int *)((int)param_1 + 0x14));
    FUN_003d5300(param_1);
  }
  return 1;
}
