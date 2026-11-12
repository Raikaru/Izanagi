/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b31a0 rpPTankSkyInsColorPV src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b31a0(unsigned int *param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  unsigned int *puVar3;
  unsigned int *puVar4;
  unsigned int *puVar5;
  unsigned int *puVar6;
  int iVar7;
  
  iVar1 = param_1[1];
  iVar2 = *(int *)(param_3 + 0x30);
  param_1 = (unsigned int *)*param_1;
  if (iVar2 != iVar1 << 2) {
    iVar7 = 0;
    puVar6 = *(unsigned int **)(param_3 + 0x2c);
    if (0 < param_2) {
      do {
        puVar5 = (unsigned int *)((int)param_1 + iVar1);
        puVar4 = (unsigned int *)((int)puVar5 + iVar1);
        iVar7 = iVar7 + 1;
        puVar3 = (unsigned int *)((int)puVar4 + iVar1);
        *param_1 = *puVar6;
        param_1 = (unsigned int *)((int)puVar3 + iVar1);
        *puVar5 = puVar6[1];
        *puVar4 = puVar6[2];
        *puVar3 = puVar6[3];
        puVar6 = (unsigned int *)((int)puVar6 + iVar2);
      } while (iVar7 < param_2);
    }
  }
  else {
    FUN_003a4d50(param_1,*(unsigned int **)(param_3 + 0x2c),param_2 * iVar1 * 4);
  }
  return;
}
