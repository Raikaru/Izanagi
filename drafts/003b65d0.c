/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b65d0 func_003b65d0 src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b65d0(int *param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  unsigned int *puVar3;
  unsigned int *puVar4;
  unsigned int *puVar5;
  unsigned int *puVar6;
  unsigned int *puVar7;
  int iVar8;
  
  puVar7 = (unsigned int *)*param_1;
  iVar1 = param_1[1];
  puVar6 = *(unsigned int **)(param_3 + 0x14);
  iVar2 = *(int *)(param_3 + 0x18);
  iVar8 = 0;
  if (0 < param_2) {
    do {
      puVar5 = (unsigned int *)((int)puVar7 + iVar1);
      puVar4 = (unsigned int *)((int)puVar5 + iVar1);
      iVar8 = iVar8 + 1;
      puVar3 = (unsigned int *)((int)puVar4 + iVar1);
      *puVar7 = *puVar6;
      puVar7[1] = puVar6[1];
      puVar7[2] = puVar6[2];
      puVar7 = (unsigned int *)((int)puVar3 + iVar1);
      *puVar5 = *puVar6;
      puVar5[1] = puVar6[1];
      puVar5[2] = puVar6[2];
      *puVar4 = *puVar6;
      puVar4[1] = puVar6[1];
      puVar4[2] = puVar6[2];
      *puVar3 = *puVar6;
      puVar3[1] = puVar6[1];
      puVar3[2] = puVar6[2];
      puVar6 = (unsigned int *)((int)puVar6 + iVar2);
    } while (iVar8 < param_2);
  }
  return;
}
