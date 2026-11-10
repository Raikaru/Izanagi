/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b61e0 func_003b61e0 src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b61e0(unsigned int *param_1,int param_2,int param_3)

{
  unsigned int uVar1;
  int iVar2;
  unsigned char bVar3;
  int iVar4;
  unsigned int *puVar5;
  
  puVar5 = (unsigned int *)*param_1;
  uVar1 = *(unsigned int *)(param_3 + 0x7c);
  iVar2 = param_1[1];
  iVar4 = 0;
  if (0 < param_2) {
    if (8 < param_2) {
      bVar3 = 0;
      if ((-1 < param_2) && (param_2 < 0x7fffffff)) {
        bVar3 = 1;
      }
      if (bVar3) {
        do {
          *puVar5 = uVar1;
          *(unsigned int *)((int)puVar5 + iVar2) = uVar1;
          iVar4 = iVar4 + 8;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
          *puVar5 = uVar1;
          puVar5 = (unsigned int *)((int)puVar5 + iVar2);
        } while (iVar4 < param_2 - 8);
      }
    }
    for (; iVar4 < param_2; iVar4 = iVar4 + 1) {
      *puVar5 = uVar1;
      *(unsigned int *)((int)puVar5 + iVar2) = uVar1;
      puVar5 = (unsigned int *)((int)puVar5 + iVar2 + iVar2);
      *puVar5 = uVar1;
      puVar5 = (unsigned int *)((int)puVar5 + iVar2);
      *puVar5 = uVar1;
      puVar5 = (unsigned int *)((int)puVar5 + iVar2);
    }
  }
  return;
}
