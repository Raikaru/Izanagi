/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b64c0 func_003b64c0 src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b64c0(int *param_1,int param_2,int param_3)

{
  int iVar1;
  unsigned int *puVar2;
  unsigned int *puVar3;
  unsigned int *puVar4;
  unsigned int *puVar5;
  int iVar6;
  
  puVar5 = (unsigned int *)*param_1;
  iVar1 = param_1[1];
  iVar6 = 0;
  if (0 < param_2) {
    do {
      puVar4 = (unsigned int *)((int)puVar5 + iVar1);
      puVar3 = (unsigned int *)((int)puVar4 + iVar1);
      iVar6 = iVar6 + 1;
      puVar2 = (unsigned int *)((int)puVar3 + iVar1);
      *puVar5 = *(unsigned int *)(param_3 + 0x90);
      puVar5[1] = *(unsigned int *)(param_3 + 0x9c);
      puVar5 = (unsigned int *)((int)puVar2 + iVar1);
      *puVar4 = *(unsigned int *)(param_3 + 0x98);
      puVar4[1] = *(unsigned int *)(param_3 + 0x9c);
      *puVar3 = *(unsigned int *)(param_3 + 0x98);
      puVar3[1] = *(unsigned int *)(param_3 + 0x94);
      *puVar2 = *(unsigned int *)(param_3 + 0x90);
      puVar2[1] = *(unsigned int *)(param_3 + 0x94);
    } while (iVar6 < param_2);
  }
  return;
}
