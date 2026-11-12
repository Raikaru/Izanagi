/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b5d20 func_003b5d20 src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b5d20(unsigned int *param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  unsigned int uVar3;
  unsigned char bVar4;
  unsigned int *puVar5;
  unsigned int *puVar6;
  unsigned int *puVar7;
  unsigned int *puVar8;
  unsigned int *puVar9;
  unsigned int *puVar10;
  unsigned int *puVar11;
  unsigned int *puVar12;
  unsigned int *puVar13;
  unsigned int *puVar14;
  unsigned int *puVar15;
  unsigned int *puVar16;
  unsigned int *puVar17;
  unsigned int *puVar18;
  unsigned int *puVar19;
  unsigned int *puVar20;
  int iVar21;
  unsigned int *puVar22;
  unsigned int *puVar23;
  unsigned int *puVar24;
  unsigned int *puVar25;
  unsigned int *puVar26;
  unsigned int *puVar27;
  unsigned int *puVar28;
  unsigned int *puVar29;
  unsigned int *puVar30;
  
  puVar6 = (unsigned int *)*param_1;
  iVar1 = param_1[1];
  puVar20 = *(unsigned int **)(param_3 + 0x24);
  iVar2 = *(int *)(param_3 + 0x28);
  iVar21 = 0;
  if (0 < param_2) {
    if (8 < param_2) {
      bVar4 = 0;
      if ((-1 < param_2) && (param_2 < 0x7fffffff)) {
        bVar4 = 1;
      }
      if (bVar4) {
        do {
          uVar3 = *puVar20;
          puVar8 = (unsigned int *)((int)puVar6 + iVar1 + iVar1);
          puVar7 = (unsigned int *)((int)puVar8 + iVar1);
          puVar9 = (unsigned int *)((int)puVar20 + iVar2 + iVar2);
          puVar18 = (unsigned int *)((int)puVar9 + iVar2);
          puVar13 = (unsigned int *)((int)puVar7 + iVar1);
          puVar28 = (unsigned int *)((int)puVar18 + iVar2);
          puVar11 = (unsigned int *)((int)puVar13 + iVar1);
          puVar12 = (unsigned int *)((int)puVar28 + iVar2);
          puVar10 = (unsigned int *)((int)puVar11 + iVar1);
          *puVar6 = uVar3;
          puVar5 = (unsigned int *)((int)puVar12 + iVar2);
          *(unsigned int *)((int)puVar6 + iVar1) = uVar3;
          puVar19 = (unsigned int *)((int)puVar10 + iVar1);
          *puVar8 = uVar3;
          iVar21 = iVar21 + 8;
          *puVar7 = uVar3;
          puVar8 = (unsigned int *)((int)puVar19 + iVar1);
          uVar3 = *(unsigned int *)((int)puVar20 + iVar2);
          puVar7 = (unsigned int *)((int)puVar8 + iVar1);
          puVar6 = (unsigned int *)((int)puVar7 + iVar1);
          puVar22 = (unsigned int *)((int)puVar6 + iVar1);
          puVar29 = (unsigned int *)((int)puVar22 + iVar1);
          puVar17 = (unsigned int *)((int)puVar29 + iVar1);
          puVar16 = (unsigned int *)((int)puVar17 + iVar1);
          *puVar13 = uVar3;
          *puVar11 = uVar3;
          *puVar10 = uVar3;
          *puVar19 = uVar3;
          uVar3 = *puVar9;
          puVar14 = (unsigned int *)((int)puVar5 + iVar2);
          puVar20 = (unsigned int *)((int)puVar14 + iVar2);
          *puVar8 = uVar3;
          *puVar7 = uVar3;
          *puVar6 = uVar3;
          *puVar22 = uVar3;
          puVar15 = (unsigned int *)((int)puVar16 + iVar1);
          puVar22 = (unsigned int *)((int)puVar15 + iVar1);
          puVar19 = (unsigned int *)((int)puVar22 + iVar1);
          puVar13 = (unsigned int *)((int)puVar19 + iVar1);
          puVar11 = (unsigned int *)((int)puVar13 + iVar1);
          puVar10 = (unsigned int *)((int)puVar11 + iVar1);
          puVar30 = (unsigned int *)((int)puVar10 + iVar1);
          puVar24 = (unsigned int *)((int)puVar30 + iVar1);
          puVar25 = (unsigned int *)((int)puVar24 + iVar1);
          puVar26 = (unsigned int *)((int)puVar25 + iVar1);
          puVar27 = (unsigned int *)((int)puVar26 + iVar1);
          uVar3 = *puVar18;
          puVar9 = (unsigned int *)((int)puVar27 + iVar1);
          puVar8 = (unsigned int *)((int)puVar9 + iVar1);
          puVar7 = (unsigned int *)((int)puVar8 + iVar1);
          puVar23 = (unsigned int *)((int)puVar7 + iVar1);
          *puVar29 = uVar3;
          puVar18 = (unsigned int *)((int)puVar23 + iVar1);
          *puVar17 = uVar3;
          puVar6 = (unsigned int *)((int)puVar18 + iVar1);
          *puVar16 = uVar3;
          *puVar15 = uVar3;
          uVar3 = *puVar28;
          *puVar22 = uVar3;
          *puVar19 = uVar3;
          *puVar13 = uVar3;
          *puVar11 = uVar3;
          uVar3 = *puVar12;
          *puVar10 = uVar3;
          *puVar30 = uVar3;
          *puVar24 = uVar3;
          *puVar25 = uVar3;
          uVar3 = *puVar5;
          *puVar26 = uVar3;
          *puVar27 = uVar3;
          *puVar9 = uVar3;
          *puVar8 = uVar3;
          uVar3 = *puVar14;
          *puVar7 = uVar3;
          *puVar23 = uVar3;
          *puVar18 = uVar3;
          *puVar6 = uVar3;
          puVar6 = (unsigned int *)((int)puVar6 + iVar1);
        } while (iVar21 < param_2 - 8);
      }
    }
    for (; iVar21 < param_2; iVar21 = iVar21 + 1) {
      uVar3 = *puVar20;
      puVar7 = (unsigned int *)((int)puVar6 + iVar1 + iVar1);
      puVar5 = (unsigned int *)((int)puVar7 + iVar1);
      *puVar6 = uVar3;
      puVar20 = (unsigned int *)((int)puVar20 + iVar2);
      *(unsigned int *)((int)puVar6 + iVar1) = uVar3;
      puVar6 = (unsigned int *)((int)puVar5 + iVar1);
      *puVar7 = uVar3;
      *puVar5 = uVar3;
    }
  }
  return;
}
