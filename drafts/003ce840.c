/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003ce840 func_003ce840 src/promoted/code1_003c.c - romwright draft, unreviewed */

int FUN_003ce840(int param_1)

{
  unsigned int uVar1;
  unsigned int uVar2;
  unsigned int uVar3;
  unsigned int uVar4;
  unsigned int uVar5;
  unsigned int uVar6;
  unsigned int uVar7;
  unsigned int uVar8;
  unsigned int uVar9;
  int iVar10;
  unsigned int *puVar11;
  unsigned int uVar12;
  unsigned int *puVar13;
  
  iVar10 = 0;
  if ((*(unsigned int *)(param_1 + 8) & 0x1000000) != 0) {
    iVar10 = 0x10;
    puVar13 = *(unsigned int **)(param_1 + DAT_00764820);
    uVar1 = *puVar13;
    uVar12 = 0;
    if (uVar1 != 0) {
      puVar11 = puVar13;
      if (8 < uVar1) {
        do {
          uVar2 = puVar11[1];
          uVar12 = uVar12 + 8;
          uVar3 = puVar11[2];
          uVar4 = puVar11[3];
          uVar5 = puVar11[4];
          uVar6 = puVar11[5];
          uVar7 = puVar11[6];
          uVar8 = puVar11[7];
          uVar9 = puVar11[8];
          iVar10 = iVar10 + (*(int *)(uVar2 + 8) - (*(int *)(uVar2 + 0x20) - uVar2)) +
                   (*(int *)(uVar3 + 8) - (*(int *)(uVar3 + 0x20) - uVar3)) +
                   (*(int *)(uVar4 + 8) - (*(int *)(uVar4 + 0x20) - uVar4)) +
                   (*(int *)(uVar5 + 8) - (*(int *)(uVar5 + 0x20) - uVar5)) +
                   (*(int *)(uVar6 + 8) - (*(int *)(uVar6 + 0x20) - uVar6)) +
                   (*(int *)(uVar7 + 8) - (*(int *)(uVar7 + 0x20) - uVar7)) +
                   (*(int *)(uVar8 + 8) - (*(int *)(uVar8 + 0x20) - uVar8)) +
                   (*(int *)(uVar9 + 8) - (*(int *)(uVar9 + 0x20) - uVar9)) + 0x40;
          puVar11 = puVar11 + 8;
        } while (uVar12 < uVar1 - 8);
      }
      if (uVar12 < uVar1) {
        puVar13 = puVar13 + uVar12;
        do {
          uVar2 = puVar13[1];
          uVar12 = uVar12 + 1;
          puVar13 = puVar13 + 1;
          iVar10 = iVar10 + (*(int *)(uVar2 + 8) - (*(int *)(uVar2 + 0x20) - uVar2)) + 8;
        } while (uVar12 < uVar1);
      }
    }
  }
  return iVar10;
}
