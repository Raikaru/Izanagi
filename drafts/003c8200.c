/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c8200 func_003c8200 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003c8200(int param_1,unsigned long long param_2,unsigned int param_3)

{
  int iVar1;
  unsigned int *puVar2;
  int *piVar3;
  int *piVar4;
  unsigned int *puVar5;
  unsigned int uVar6;
  unsigned int *puVar7;
  int *piVar8;
  unsigned int *puVar9;
  unsigned long long uVar10;
  int iVar11;
  unsigned int uVar12;
  unsigned int uVar13;
  unsigned int uVar14;
  int *piVar15;
  unsigned int *puVar16;
  int iVar17;
  unsigned int uVar18;
  unsigned int uVar19;
  unsigned int uVar20;
  unsigned int uStack_40;
  int *piStack_20;
  int *piStack_4;
  
  uVar18 = 0;
  piVar3 = (int *)(*DAT_008873e8)(*(int *)(param_1 + 4) << 2,0x10502);
  if (piVar3 != (int *)0x0) {
    uVar14 = 0;
    if (*(int *)(param_1 + 4) != 0) {
      iVar11 = 0;
      piVar4 = piVar3;
      do {
        uVar14 = uVar14 + 1;
        *piVar4 = *(int *)(param_1 + 8) + iVar11;
        iVar11 = iVar11 + 0x14;
        uVar12 = *(unsigned int *)(param_1 + 4);
        piVar4 = piVar4 + 1;
      } while (uVar14 < uVar12);
    }
    else {
      uVar12 = 0;
    }
    FUN_00440bb8(piVar3,uVar12,4,0x3c6080);
    uVar14 = *(unsigned int *)(param_1 + 4);
    iVar11 = 1;
    if (1 < uVar14) {
      iVar17 = *(int *)(*piVar3 + 8);
      uVar12 = 1;
      piVar4 = piVar3;
      if (1 < uVar14) {
        do {
          iVar1 = *(int *)(piVar4[1] + 8);
          if (iVar1 != iVar17) {
            iVar11 = iVar11 + 1;
            iVar17 = iVar1;
          }
          uVar12 = uVar12 + 1;
          piVar4 = piVar4 + 1;
        } while (uVar12 < uVar14);
      }
    }
    piVar4 = (int *)(*DAT_008873e8)(iVar11 << 2,0x1010502);
    puVar5 = (unsigned int *)(*DAT_008873e8)(iVar11 * 0xc,0x10502);
    puVar5[2] = *(unsigned int *)(*piVar3 + 8);
    puVar5[1] = 0;
    *puVar5 = 0;
    uVar14 = *(unsigned int *)(param_1 + 4);
    uVar12 = 1;
    if ((1 < uVar14) && (uVar13 = 0, uVar14 != 1)) {
      puVar9 = puVar5 + 3;
      piVar15 = piVar3;
      do {
        if (*(int *)(*piVar15 + 8) != *(int *)(piVar15[1] + 8)) {
          puVar9[2] = *(int *)(piVar15[1] + 8);
          uVar12 = uVar12 + 1;
          puVar9[1] = uVar13 + 1;
          *puVar9 = 0;
          puVar9[-2] = (uVar13 + 1) - puVar9[-2];
          puVar9 = puVar9 + 3;
        }
        uVar14 = *(unsigned int *)(param_1 + 4);
        uVar13 = uVar13 + 1;
        piVar15 = piVar15 + 1;
      } while (uVar13 < uVar14 - 1);
    }
    puVar5[uVar12 * 3 - 2] = uVar14 - puVar5[uVar12 * 3 - 2];
    uVar6 = FUN_003e1210(0x10,*(unsigned int *)(param_1 + 4) / 10 + 5,4,0x10502);
    piStack_4 = (int *)0x0;
    uStack_40 = 0;
    *(unsigned int *)((int)&DAT_008872e4 + DAT_007647e0) = uVar6;
    puVar9 = puVar5;
    piStack_20 = piVar3;
    if (uVar12 != 0) {
      do {
        puVar7 = (unsigned int *)(*DAT_008873e8)(puVar9[1] * 0x14,0x10502);
        uVar13 = 0;
        uVar14 = 0;
        puVar16 = puVar7;
        if (puVar9[1] != 0) {
          do {
            uVar13 = uVar13 + 1;
            puVar2 = (unsigned int *)*piStack_20;
            piStack_20 = piStack_20 + 1;
            uVar20 = puVar2[1];
            uVar6 = puVar2[2];
            uVar19 = puVar2[3];
            *puVar16 = *puVar2;
            puVar16[1] = uVar20;
            puVar16[2] = uVar6;
            puVar16[3] = uVar19;
            puVar16[4] = puVar2[4];
            uVar14 = puVar9[1];
            puVar16 = puVar16 + 5;
          } while (uVar13 < uVar14);
        }
        FUN_003c70e0(puVar7,uVar14,&piStack_4,param_2);
        FUN_003c7d90(&piStack_4,param_3);
        for (piVar15 = piStack_4; piVar15 != (int *)0x0; piVar15 = (int *)piVar15[3]) {
          piVar8 = (int *)(*DAT_008873e8)(piVar15[1] * 2 + 0xc,0x10502);
          piVar8[2] = puVar9[2];
          piVar8[1] = piVar15[1];
          *piVar8 = (int)(piVar8 + 3);
          uVar14 = 0;
          if (piVar8[1] != 0) {
            iVar11 = 0;
            do {
              uVar14 = uVar14 + 1;
              *(unsigned short *)(*piVar8 + iVar11) = *(unsigned short *)(*piVar15 + iVar11);
              iVar11 = iVar11 + 2;
            } while (uVar14 < (unsigned int)piVar8[1]);
          }
          piVar4[uVar18] = (int)piVar8;
          uVar18 = (uVar18 + 1) & 0xffff;
        }
        while (piVar15 = piStack_4, piStack_4 != (int *)0x0) {
          iVar11 = *piStack_4;
          piStack_4 = (int *)piStack_4[3];
          (*DAT_008873ec)(iVar11);
          *piVar15 = 0;
          (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647e0),piVar15);
        }
        (*DAT_008873ec)(puVar7);
        uStack_40 = uStack_40 + 1;
        puVar9 = puVar9 + 3;
      } while (uStack_40 < uVar12);
    }
    FUN_003e12f0(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647e0));
    iVar11 = 0x10;
    iVar17 = 0;
    uVar14 = 0;
    *(unsigned int *)((int)&DAT_008872e4 + DAT_007647e0) = 0;
    if (uVar18 != 0) {
      piVar15 = piVar4;
      if (8 < uVar18) {
        do {
          uVar14 = uVar14 + 8;
          iVar17 = iVar17 + *(int *)(*piVar15 + 4) + *(int *)(piVar15[1] + 4) +
                   *(int *)(piVar15[2] + 4) + *(int *)(piVar15[3] + 4) + *(int *)(piVar15[4] + 4) +
                   *(int *)(piVar15[5] + 4) + *(int *)(piVar15[6] + 4) + *(int *)(piVar15[7] + 4);
          iVar11 = iVar11 + *(int *)(*piVar15 + 4) * 2 + *(int *)(piVar15[1] + 4) * 2 +
                   *(int *)(piVar15[2] + 4) * 2 + *(int *)(piVar15[3] + 4) * 2 +
                   *(int *)(piVar15[4] + 4) * 2 + *(int *)(piVar15[5] + 4) * 2 +
                   *(int *)(piVar15[6] + 4) * 2 + *(int *)(piVar15[7] + 4) * 2 + 0x60;
          piVar15 = piVar15 + 8;
        } while (uVar14 < uVar18 - 8);
      }
      if (uVar14 < uVar18) {
        piVar15 = piVar4 + uVar14;
        do {
          iVar1 = *piVar15;
          uVar14 = uVar14 + 1;
          piVar15 = piVar15 + 1;
          iVar17 = iVar17 + *(int *)(iVar1 + 4);
          iVar11 = iVar11 + *(int *)(iVar1 + 4) * 2 + 0xc;
        } while (uVar14 < uVar18);
      }
    }
    uVar10 = FUN_003c54a0(iVar11);
    puVar9 = (unsigned int *)uVar10;
    *puVar9 = 1;
    *(short *)(puVar9 + 1) = (short)uVar18;
    puVar16 = puVar9 + 4;
    puVar7 = puVar16 + uVar18 * 3;
    uVar14 = 0;
    *(unsigned short *)((int)puVar9 + 6) = *(unsigned short *)((int)&DAT_008872e0 + DAT_007647e0);
    puVar9[3] = 0;
    puVar9[2] = iVar17;
    *(short *)((int)&DAT_008872e0 + DAT_007647e0) =
         *(short *)((int)&DAT_008872e0 + DAT_007647e0) + 1;
    piVar15 = piVar4;
    if (uVar18 != 0) {
      do {
        *puVar16 = puVar7;
        puVar16[1] = *(unsigned int *)(*piVar15 + 4);
        puVar16[2] = *(unsigned int *)(*piVar15 + 8);
        FUN_0043f810(puVar7,*(unsigned int *)*piVar15,((unsigned int *)*piVar15)[1] << 1);
        piVar8 = puVar16 + 1;
        puVar16 = puVar16 + 3;
        puVar7 = (unsigned int *)((int)puVar7 + *piVar8 * 2);
        (*DAT_008873ec)(*piVar15);
        uVar14 = uVar14 + 1;
        *piVar15 = 0;
        piVar15 = piVar15 + 1;
      } while (uVar14 < uVar18);
    }
    (*DAT_008873ec)(piVar3);
    (*DAT_008873ec)(piVar4);
    (*DAT_008873ec)(puVar5);
  }
  else {
    uVar10 = 0;
  }
  return uVar10;
}
