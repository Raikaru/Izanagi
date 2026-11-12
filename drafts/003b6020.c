/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b6020 func_003b6020 src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b6020(unsigned int *param_1,int param_2,int param_3)

{
  unsigned int uVar1;
  unsigned int uVar2;
  unsigned int uVar3;
  int iVar4;
  unsigned int uVar5;
  unsigned char bVar6;
  int iVar7;
  unsigned int *puVar8;
  
  puVar8 = (unsigned int *)*param_1;
  uVar1 = *(unsigned int *)(param_3 + 0x80);
  uVar2 = *(unsigned int *)(param_3 + 0x84);
  uVar3 = *(unsigned int *)(param_3 + 0x88);
  iVar4 = param_1[1];
  uVar5 = *(unsigned int *)(param_3 + 0x8c);
  iVar7 = 0;
  if (0 < param_2) {
    if (8 < param_2) {
      bVar6 = 0;
      if ((-1 < param_2) && (param_2 < 0x7fffffff)) {
        bVar6 = 1;
      }
      if (bVar6) {
        do {
          *puVar8 = uVar1;
          *(unsigned int *)((int)puVar8 + iVar4) = uVar2;
          iVar7 = iVar7 + 8;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar1;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar2;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar1;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar2;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar1;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar2;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar1;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar2;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar1;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar2;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar1;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar2;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar1;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar2;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar3;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
          *puVar8 = uVar5;
          puVar8 = (unsigned int *)((int)puVar8 + iVar4);
        } while (iVar7 < param_2 - 8);
      }
    }
    for (; iVar7 < param_2; iVar7 = iVar7 + 1) {
      *puVar8 = uVar1;
      *(unsigned int *)((int)puVar8 + iVar4) = uVar2;
      puVar8 = (unsigned int *)((int)puVar8 + iVar4 + iVar4);
      *puVar8 = uVar3;
      puVar8 = (unsigned int *)((int)puVar8 + iVar4);
      *puVar8 = uVar5;
      puVar8 = (unsigned int *)((int)puVar8 + iVar4);
    }
  }
  return;
}
