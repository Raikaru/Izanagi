/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00396940 func_00396940 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_00396940(unsigned long long param_1,int param_2)

{
  unsigned int *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  unsigned int *puVar5;
  
  puVar5 = (unsigned int *)((int)param_1 + DAT_007646c8);
  puVar1 = (unsigned int *)((unsigned int *)(param_2 + DAT_007646c8))[1];
  *puVar5 = *(unsigned int *)(param_2 + DAT_007646c8);
  if ((puVar1 != (unsigned int *)0x0) && ((*puVar1 & 1) == 0)) {
    iVar2 = FUN_00396fb0(puVar1[1],0,0,*puVar1,*(unsigned int *)(puVar1[8] + 0x20));
    iVar4 = 0;
    if (0 < *(int *)(iVar2 + 4)) {
      iVar3 = 0;
      do {
        iVar4 = iVar4 + 1;
        *(unsigned int *)(*(int *)(iVar2 + 0x10) + iVar3 + 0xc) = 0;
        *(unsigned int *)(*(int *)(iVar2 + 0x10) + iVar3 + 8) = *(unsigned int *)(puVar1[4] + iVar3 + 8)
        ;
        *(unsigned int *)(*(int *)(iVar2 + 0x10) + iVar3 + 4) = *(unsigned int *)(puVar1[4] + iVar3 + 4)
        ;
        *(unsigned int *)(*(int *)(iVar2 + 0x10) + iVar3) = *(unsigned int *)(puVar1[4] + iVar3);
        iVar3 = iVar3 + 0x10;
      } while (iVar4 < *(int *)(iVar2 + 4));
    }
    puVar5[1] = iVar2;
    *(int *)(iVar2 + 0x14) = (int)param_1;
  }
  return param_1;
}
