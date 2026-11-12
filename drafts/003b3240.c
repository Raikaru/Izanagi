/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b3240 rpPTankSkyInsColorPVC src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b3240(unsigned int *param_1,int param_2,int param_3)

{
  int iVar1;
  unsigned char bVar2;
  unsigned int *puVar3;
  unsigned int *puVar4;
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
  unsigned int *puVar21;
  unsigned int *puVar22;
  unsigned int *puVar23;
  unsigned int *puVar24;
  unsigned int *puVar25;
  unsigned int *puVar26;
  unsigned int *puVar27;
  int iVar28;
  unsigned int *puVar29;
  unsigned int *puVar30;
  unsigned int *puVar31;
  int uStack_10;
  unsigned int uStack_c;
  unsigned int uStack_8;
  unsigned int uStack_4;
  
  puVar10 = (unsigned int *)*param_1;
  iVar1 = param_1[1];
  if (iVar1 != 4) {
    iVar28 = 0;
    if (0 < param_2) {
      if (8 < param_2) {
        bVar2 = 0;
        if ((-1 < param_2) && (param_2 < 0x7fffffff)) {
          bVar2 = 1;
        }
        if (bVar2) {
          do {
            puVar12 = (unsigned int *)((int)puVar10 + iVar1 + iVar1);
            puVar11 = (unsigned int *)((int)puVar12 + iVar1);
            iVar28 = iVar28 + 8;
            puVar27 = (unsigned int *)((int)puVar11 + iVar1);
            puVar26 = (unsigned int *)((int)puVar27 + iVar1);
            puVar25 = (unsigned int *)((int)puVar26 + iVar1);
            *puVar10 = *(unsigned int *)(param_3 + 0x80);
            puVar24 = (unsigned int *)((int)puVar25 + iVar1);
            puVar23 = (unsigned int *)((int)puVar24 + iVar1);
            puVar22 = (unsigned int *)((int)puVar23 + iVar1);
            puVar30 = (unsigned int *)((int)puVar22 + iVar1);
            puVar29 = (unsigned int *)((int)puVar30 + iVar1);
            puVar21 = (unsigned int *)((int)puVar29 + iVar1);
            puVar20 = (unsigned int *)((int)puVar21 + iVar1);
            *(unsigned int *)((int)puVar10 + iVar1) = *(unsigned int *)(param_3 + 0x84);
            puVar19 = (unsigned int *)((int)puVar20 + iVar1);
            puVar18 = (unsigned int *)((int)puVar19 + iVar1);
            puVar17 = (unsigned int *)((int)puVar18 + iVar1);
            puVar16 = (unsigned int *)((int)puVar17 + iVar1);
            puVar15 = (unsigned int *)((int)puVar16 + iVar1);
            puVar14 = (unsigned int *)((int)puVar15 + iVar1);
            puVar3 = (unsigned int *)((int)puVar14 + iVar1);
            *puVar12 = *(unsigned int *)(param_3 + 0x88);
            puVar13 = (unsigned int *)((int)puVar3 + iVar1);
            puVar12 = (unsigned int *)((int)puVar13 + iVar1);
            puVar4 = (unsigned int *)((int)puVar12 + iVar1);
            puVar5 = (unsigned int *)((int)puVar4 + iVar1);
            puVar6 = (unsigned int *)((int)puVar5 + iVar1);
            *puVar11 = *(unsigned int *)(param_3 + 0x8c);
            puVar11 = (unsigned int *)((int)puVar6 + iVar1);
            puVar7 = (unsigned int *)((int)puVar11 + iVar1);
            puVar31 = (unsigned int *)((int)puVar7 + iVar1);
            puVar8 = (unsigned int *)((int)puVar31 + iVar1);
            puVar9 = (unsigned int *)((int)puVar8 + iVar1);
            puVar10 = (unsigned int *)((int)puVar9 + iVar1);
            *puVar27 = *(unsigned int *)(param_3 + 0x80);
            *puVar26 = *(unsigned int *)(param_3 + 0x84);
            *puVar25 = *(unsigned int *)(param_3 + 0x88);
            *puVar24 = *(unsigned int *)(param_3 + 0x8c);
            *puVar23 = *(unsigned int *)(param_3 + 0x80);
            *puVar22 = *(unsigned int *)(param_3 + 0x84);
            *puVar30 = *(unsigned int *)(param_3 + 0x88);
            *puVar29 = *(unsigned int *)(param_3 + 0x8c);
            *puVar21 = *(unsigned int *)(param_3 + 0x80);
            *puVar20 = *(unsigned int *)(param_3 + 0x84);
            *puVar19 = *(unsigned int *)(param_3 + 0x88);
            *puVar18 = *(unsigned int *)(param_3 + 0x8c);
            *puVar17 = *(unsigned int *)(param_3 + 0x80);
            *puVar16 = *(unsigned int *)(param_3 + 0x84);
            *puVar15 = *(unsigned int *)(param_3 + 0x88);
            *puVar14 = *(unsigned int *)(param_3 + 0x8c);
            *puVar3 = *(unsigned int *)(param_3 + 0x80);
            *puVar13 = *(unsigned int *)(param_3 + 0x84);
            *puVar12 = *(unsigned int *)(param_3 + 0x88);
            *puVar4 = *(unsigned int *)(param_3 + 0x8c);
            *puVar5 = *(unsigned int *)(param_3 + 0x80);
            *puVar6 = *(unsigned int *)(param_3 + 0x84);
            *puVar11 = *(unsigned int *)(param_3 + 0x88);
            *puVar7 = *(unsigned int *)(param_3 + 0x8c);
            *puVar31 = *(unsigned int *)(param_3 + 0x80);
            *puVar8 = *(unsigned int *)(param_3 + 0x84);
            *puVar9 = *(unsigned int *)(param_3 + 0x88);
            *puVar10 = *(unsigned int *)(param_3 + 0x8c);
            puVar10 = (unsigned int *)((int)puVar10 + iVar1);
          } while (iVar28 < param_2 - 8);
        }
      }
      for (; iVar28 < param_2; iVar28 = iVar28 + 1) {
        puVar4 = (unsigned int *)((int)puVar10 + iVar1);
        puVar12 = (unsigned int *)((int)puVar4 + iVar1);
        puVar3 = (unsigned int *)((int)puVar12 + iVar1);
        *puVar10 = *(unsigned int *)(param_3 + 0x80);
        puVar10 = (unsigned int *)((int)puVar3 + iVar1);
        *puVar4 = *(unsigned int *)(param_3 + 0x84);
        *puVar12 = *(unsigned int *)(param_3 + 0x88);
        *puVar3 = *(unsigned int *)(param_3 + 0x8c);
      }
    }
  }
  else {
    uStack_10 = *(unsigned int *)(param_3 + 0x80);
    uStack_c = *(unsigned int *)(param_3 + 0x84);
    uStack_8 = *(unsigned int *)(param_3 + 0x88);
    uStack_4 = *(unsigned int *)(param_3 + 0x8c);
    FUN_003a4850(puVar10,param_2 << 4,&uStack_10);
  }
  return;
}
