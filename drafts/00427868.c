/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00427868 func_00427868 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_00427868(unsigned long long param_1,long param_2,unsigned long long *param_3)

{
  unsigned int uVar1;
  unsigned int uVar2;
  unsigned long long *puVar3;
  unsigned long long *puVar4;
  long lVar5;
  unsigned long long uVar6;
  int *piVar7;
  int iVar8;
  unsigned long long *puVar9;
  unsigned long long extraout_a1;
  unsigned long long extraout_a1_00;
  unsigned long long uVar10;
  unsigned long long extraout_a3;
  unsigned long long extraout_a3_00;
  unsigned long long uVar11;
  unsigned long long extraout_t0;
  unsigned long long extraout_t0_00;
  unsigned long long uVar12;
  unsigned long long uVar13;
  unsigned long long extraout_t1;
  unsigned long long extraout_t1_00;
  unsigned long long uVar14;
  unsigned long long uVar15;
  unsigned int *puVar16;
  unsigned char auStack_b0 [4];
  unsigned int uStack_ac;
  unsigned int uStack_a8;
  unsigned char *puStack_9c;
  unsigned int auStack_90 [4];
  
  lVar5 = FUN_00426248();
  FUN_004266c8(5);
  uVar10 = extraout_a1;
  uVar11 = extraout_a3;
  uVar12 = extraout_t0;
  uVar14 = extraout_t1;
  DAT_0088f748 = param_3;
  if (DAT_0070c67c == 0) {
    FUN_004267f0();
    uVar10 = extraout_a1_00;
    uVar11 = extraout_a3_00;
    uVar12 = extraout_t0_00;
    uVar14 = extraout_t1_00;
  }
  if ((lVar5 == 0) || (puVar16 = (unsigned int *)lVar5, puVar16[1] == 0)) {
    FUN_004266f8();
    return 0xfffffff7;
  }
  DAT_0088fb94 = 0;
  DAT_0088fb98 = 0;
  if (param_2 != 2) {
    if (param_2 < 3) {
      if (param_2 != 1) {
        DAT_0088f78c = *puVar16;
LAB_00427a08:
        DAT_0088f790._0_4_ = (unsigned int)param_2;
        if (param_3 == (unsigned long long *)0x0) {
          DAT_0088fb9c = 0;
        }
        else {
          puVar9 = (unsigned long long *)((int)&DAT_0088f790 + 4);
          DAT_0088fb9c = 0x400;
          if ((((unsigned int)param_3 | 0x88f794) & 7) != 0) {
            puVar4 = param_3 + 0x80;
            do {
              uVar1 = ((int)param_3 + 7U) & 7;
              uVar2 = (unsigned int)param_3 & 7;
              uVar10 = (((*(long *)(((int)param_3 + 7U) - uVar1) << (7 - uVar1) * 8) |
                        (uVar10 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                       (-1L << (8 - uVar2) * 8)) | (*(unsigned long long *)((int)param_3 - uVar2) >> uVar2 * 8);
              uVar1 = ((int)param_3 + 0xfU) & 7;
              uVar2 = (unsigned int)(param_3 + 1) & 7;
              uVar11 = (((*(long *)(((int)param_3 + 0xfU) - uVar1) << (7 - uVar1) * 8) |
                        (uVar11 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                       (-1L << (8 - uVar2) * 8)) |
                       (*(unsigned long long *)((int)(param_3 + 1) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)param_3 + 0x17U) & 7;
              uVar2 = (unsigned int)(param_3 + 2) & 7;
              uVar12 = (((*(long *)(((int)param_3 + 0x17U) - uVar1) << (7 - uVar1) * 8) |
                        (uVar12 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                       (-1L << (8 - uVar2) * 8)) |
                       (*(unsigned long long *)((int)(param_3 + 2) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)param_3 + 0x1fU) & 7;
              uVar2 = (unsigned int)(param_3 + 3) & 7;
              uVar14 = (((*(long *)(((int)param_3 + 0x1fU) - uVar1) << (7 - uVar1) * 8) |
                        (uVar14 & (0xffffffffffffffffU >> (uVar1 + 1) * 8))) &
                       (-1L << (8 - uVar2) * 8)) |
                       (*(unsigned long long *)((int)(param_3 + 3) - uVar2) >> uVar2 * 8);
              uVar1 = ((int)puVar9 + 7U) & 7;
              puVar3 = (unsigned long long *)(((int)puVar9 + 7U) - uVar1);
              *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar10 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)puVar9 & 7;
              *(unsigned long long *)((int)puVar9 - uVar1) =
                   (uVar10 << uVar1 * 8) |
                   (*(unsigned long long *)((int)puVar9 - uVar1) & (0xffffffffffffffffU >> (8 - uVar1) * 8));
              uVar1 = ((int)puVar9 + 0xfU) & 7;
              puVar3 = (unsigned long long *)(((int)puVar9 + 0xfU) - uVar1);
              *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar11 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)(puVar9 + 1) & 7;
              puVar3 = (unsigned long long *)((int)(puVar9 + 1) - uVar1);
              *puVar3 = (uVar11 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8))
              ;
              uVar1 = ((int)puVar9 + 0x17U) & 7;
              puVar3 = (unsigned long long *)(((int)puVar9 + 0x17U) - uVar1);
              *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar12 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)(puVar9 + 2) & 7;
              puVar3 = (unsigned long long *)((int)(puVar9 + 2) - uVar1);
              *puVar3 = (uVar12 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8))
              ;
              uVar1 = ((int)puVar9 + 0x1fU) & 7;
              puVar3 = (unsigned long long *)(((int)puVar9 + 0x1fU) - uVar1);
              *puVar3 = (*puVar3 & (-1L << (uVar1 + 1) * 8)) | (uVar14 >> (7 - uVar1) * 8);
              uVar1 = (unsigned int)(puVar9 + 3) & 7;
              puVar3 = (unsigned long long *)((int)(puVar9 + 3) - uVar1);
              *puVar3 = (uVar14 << uVar1 * 8) | (*puVar3 & (0xffffffffffffffffU >> (8 - uVar1) * 8))
              ;
              param_3 = param_3 + 4;
              puVar9 = puVar9 + 4;
            } while (param_3 != puVar4);
          }
          else {
            puVar4 = param_3 + 0x80;
            do {
              uVar6 = param_3[1];
              uVar13 = param_3[2];
              uVar15 = param_3[3];
              *puVar9 = *param_3;
              puVar9[1] = uVar6;
              puVar9[2] = uVar13;
              puVar9[3] = uVar15;
              param_3 = param_3 + 4;
              puVar9 = puVar9 + 4;
            } while (param_3 != puVar4);
          }
        }
        uStack_ac = 1;
        puStack_9c = &LAB_00754218;
        uStack_a8 = 0;
        uVar6 = FUN_004217e0(auStack_b0);
        DAT_0088f788 = 4;
        DAT_0088f780 = (unsigned int)uVar6;
        DAT_0088f784 = auStack_90;
        FUN_00424e78(0x88f780,0x420);
        lVar5 = FUN_00426158(0x890e80,5,0,0x88f780,0x420,0x8903c0,4,0);
        iVar8 = iRam208903c0;
        if (lVar5 < 0) {
          FUN_004217f0(uVar6);
          FUN_004266f8();
          return 0xfffffff5;
        }
        FUN_004266f8();
        if (iVar8 == 0) {
          FUN_004217f0(uVar6);
          return 0xfffffff5;
        }
        FUN_00421820(uVar6);
        FUN_004217f0(uVar6);
        return auStack_90[0];
      }
      FUN_00421820(DAT_0070c68c);
      iVar8 = 0;
      if (DAT_0070c5f8 == -1) {
        piVar7 = &DAT_0070c5f8;
        iVar8 = 1;
        while ((piVar7 = piVar7 + 1, iVar8 < 0x20 && (*piVar7 == -1))) {
          iVar8 = iVar8 + 1;
        }
      }
      if (iVar8 == 0x20) {
        *(unsigned int *)DAT_0088f748 = 0;
      }
      else {
        *(unsigned int *)DAT_0088f748 = 1;
      }
      FUN_00421800(DAT_0070c68c);
    }
    else {
      if (param_2 != 3) {
        DAT_0088f78c = *puVar16;
        goto LAB_00427a08;
      }
      *param_3 = *(unsigned long long *)((DAT_0070c678 * 0x440 + 0x890410U) | 0x20000000);
    }
  }
  else {
    *(unsigned int *)param_3 = *(unsigned int *)((DAT_0070c678 * 0x440 + 0x890410U) | 0x20000000);
  }
  FUN_004266f8();
  return 0;
}
