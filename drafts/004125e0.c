/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004125e0 func_004125e0 src/promoted/code1_0041.c - romwright draft, unreviewed */
typedef struct fn_004125e0_arg0 fn_004125e0_arg0;
struct fn_004125e0_arg0 {
    int field_0x0; /* +0x0 */
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x14];
    int field_0x20; /* +0x20 */
    int field_0x24; /* +0x24 */
    int field_0x28; /* +0x28 */
};


long FUN_004125e0(fn_004125e0_arg0 *param_1)

{
  int iVar1;
  code *pcVar2;
  unsigned char bVar3;
  int iVar4;
  unsigned int uVar5;
  long lVar6;
  long lVar7;
  unsigned int uVar8;
  unsigned int uVar9;
  int iVar10;
  int iVar11;
  int *piVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  int uStack_18;
  unsigned int uStack_14;
  int uStack_10;
  unsigned int uStack_c;
  int uStack_8;
  unsigned int uStack_4;
  
  lVar7 = (long)(int)param_1;
  if ((lVar7 == 0) || (param_1->field_0x0 == 0)) {
    if (lVar7 != 0) {
      uStack_18 = 1;
      uStack_14 = FUN_003df590(0x34);
      FUN_003df4d0(&uStack_18);
    }
    else {
      uStack_10 = 1;
      uStack_c = FUN_003df590(0xffffffff80000016);
      FUN_003df4d0(&uStack_10);
    }
    return 0;
  }
  if (param_1->field_0x4 != 0) {
    iVar16 = 0;
    bVar3 = 0;
    iVar17 = 0;
    if (((unsigned int)param_1->field_0x4 <= (unsigned int)param_1->field_0x28) ||
       (*(int *)(param_1->field_0x8 + param_1->field_0x28 * 0x28) == 0)) {
      uStack_8 = 1;
      uStack_4 = FUN_003df590(0x24);
      FUN_003df4d0(&uStack_8);
      return 0;
    }
    iVar4 = FUN_00411820();
    uVar5 = param_1->field_0x4;
    uVar9 = 0;
    iVar11 = uVar5 * 0x28;
    uVar8 = *(int *)(DAT_00764aa8 + 0x887318) * 0xb4;
    if (uVar5 != 0) {
      if (8 < uVar5) {
        do {
          iVar11 = iVar11 + *(int *)(param_1->field_0x8 + 4) * 0x20;
          uVar9 = uVar9 + 8;
        } while (uVar9 < uVar5 - 8);
      }
      if (uVar9 < uVar5) {
        do {
          uVar9 = uVar9 + 1;
          iVar11 = iVar11 + *(int *)(param_1->field_0x8 + 4) * 4;
        } while (uVar9 < uVar5);
      }
    }
    uVar9 = 0;
    iVar10 = iVar4 * 0x14 + uVar5 * iVar4 * 4 + uVar5 * (iVar4 + 1) * 4;
    if (uVar5 != 0) {
      piVar12 = (int *)param_1->field_0x8;
      do {
        iVar13 = *piVar12;
        iVar15 = *(int *)(iVar13 + 0x34);
        if (iVar15 != 0) {
          iVar10 = iVar10 + iVar15;
        }
        uVar9 = uVar9 + 1;
        piVar12 = piVar12 + 10;
        iVar10 = iVar10 + *(int *)(iVar13 + 0x20) * 4;
      } while (uVar9 < uVar5);
    }
    uVar5 = *(int *)(DAT_00764aa8 + 0x887318) * 0xc + iVar11 +
            uVar5 * (iVar4 * 0x24 + 0x14) + uVar5 * iVar4 * 0x10 +
            iVar10 + (iVar4 - 1) * 0x1c + 0x30;
    if (uVar8 < uVar5) {
      uVar8 = uVar5;
    }
    if (((unsigned int)param_1->field_0x24 < uVar8) && (lVar6 = FUN_004118c0(lVar7,uVar8), lVar6 == 0)) {
      return 0;
    }
    DAT_00764ab8 = param_1->field_0x20 + uVar8;
    DAT_00764abc = 0;
    lVar7 = FUN_004123c0(lVar7);
    if (lVar7 == 0) {
      return 0;
    }
    iVar15 = (int)lVar7;
    iVar11 = *(int *)(iVar15 + 4) - 1;
    iVar4 = *(int *)(iVar15 + 0x20) + uVar8;
    iVar10 = *(int *)(iVar15 + 8) + *(int *)(DAT_00764aa8 + 0x887318) * 0x28;
    iVar13 = iVar10 + *(int *)(DAT_00764aa8 + 0x887318) * 0x80 + iVar11 * 0xc;
    if (iVar11 < 0) {
      iVar4 = *(int *)(iVar15 + 4);
    }
    else {
      iVar14 = iVar11 * 0x28;
      do {
        iVar4 = iVar4 - 0xc;
        FUN_0043f810(iVar4,iVar13,0xc);
        iVar11 = iVar11 - 1;
        iVar13 = iVar13 - 0xc;
        *(int *)(*(int *)(iVar15 + 8) + iVar14 + 0x1c) = iVar4;
        iVar14 = iVar14 - 0x28;
      } while (-1 < iVar11);
      iVar4 = *(int *)(iVar15 + 4);
    }
    uVar5 = 0;
    iVar11 = *(int *)(iVar15 + 8);
    iVar14 = iVar11 + iVar4 * 0x28;
    if (iVar4 != 0) {
      iVar4 = 0;
      do {
        iVar1 = *(int *)(iVar11 + iVar4 + 4);
        if (iVar1 == 0) {
          *(unsigned int *)(iVar11 + iVar4 + 8) = 0;
        }
        else {
          FUN_0043f810(iVar14,iVar10,iVar1 << 2);
          *(int *)(*(int *)(iVar15 + 8) + iVar4 + 8) = iVar14;
        }
        iVar11 = *(int *)(iVar15 + 8);
        uVar5 = uVar5 + 1;
        iVar10 = iVar10 + 0x80;
        iVar1 = *(int *)(iVar11 + iVar4 + 4);
        iVar4 = iVar4 + 0x28;
        iVar16 = iVar16 + iVar1;
        iVar14 = iVar14 + iVar1 * 4;
      } while (uVar5 < *(unsigned int *)(iVar15 + 4));
    }
    DAT_00764abc = iVar10 + iVar16 * 4;
    DAT_00764ab8 = iVar13;
    lVar6 = FUN_004147a0(lVar7);
    if (lVar6 != 0) {
      return 0;
    }
    lVar6 = FUN_004118c0(lVar7,DAT_00764abc - *(int *)(iVar15 + 0x20));
    if (lVar6 != 0) {
      uVar8 = 0;
      uVar5 = 0;
      if (*(int *)(iVar15 + 4) != 0) {
        iVar16 = 0;
        do {
          uVar8 = uVar8 + 1;
          *(unsigned int *)(*(int *)(iVar15 + 8) + iVar16 + 0x1c) = 0;
          uVar5 = *(unsigned int *)(iVar15 + 4);
          iVar16 = iVar16 + 0x28;
        } while (uVar8 < uVar5);
      }
      iVar16 = uVar5 - 1;
      if (-1 < iVar16) {
        iVar4 = iVar16 * 0x28;
        do {
          piVar12 = (int *)(*(int *)(iVar15 + 8) + iVar4);
          iVar11 = *piVar12;
          iVar10 = *(int *)(iVar11 + 0x3c);
          *(int *)(iVar11 + 0x3c) = iVar10 + 1;
          if (((iVar10 == 0) && (*(code **)(iVar11 + 8) != (code *)0x0)) &&
             (lVar6 = (**(code **)(iVar11 + 8))(iVar11), lVar6 == 0)) {
            bVar3 = 1;
            iVar17 = (*(int *)(iVar15 + 4) - 1) - iVar16;
            break;
          }
          if ((*(code **)(iVar11 + 0x10) != (code *)0x0) &&
             (lVar6 = (**(code **)(iVar11 + 0x10))(piVar12), lVar6 == 0)) {
            *(int *)(iVar11 + 0x3c) = *(int *)(iVar11 + 0x3c) - 1;
            if ((*(int *)(iVar11 + 0x3c) == 0) && (*(code **)(iVar11 + 0xc) != (code *)0x0)) {
              (**(code **)(iVar11 + 0xc))(iVar11);
            }
            bVar3 = 1;
            iVar17 = (*(int *)(iVar15 + 4) - 1) - iVar16;
            break;
          }
          iVar16 = iVar16 - 1;
          iVar4 = iVar4 - 0x28;
        } while (-1 < iVar16);
      }
      if ((!bVar3) && (iVar16 = *(int *)(iVar15 + 4) - 1, -1 < iVar16)) {
        iVar4 = iVar16 * 0x28;
        do {
          piVar12 = (int *)(*(int *)(iVar15 + 8) + iVar4);
          pcVar2 = *(code **)(*piVar12 + 0x18);
          if ((pcVar2 != (code *)0x0) && (lVar6 = (*pcVar2)(piVar12,lVar7), lVar6 == 0)) {
            iVar17 = *(int *)(iVar15 + 4);
            bVar3 = 1;
            break;
          }
          iVar16 = iVar16 - 1;
          iVar4 = iVar4 - 0x28;
        } while (-1 < iVar16);
      }
      if (!bVar3) {
        *(unsigned int *)lVar7 = 0;
        return lVar7;
      }
      uVar5 = *(unsigned int *)(iVar15 + 4) - iVar17;
      if (uVar5 < *(unsigned int *)(iVar15 + 4)) {
        iVar17 = uVar5 * 0x28;
        do {
          iVar16 = *(int *)(*(int *)(iVar15 + 8) + iVar17);
          if (*(code **)(iVar16 + 0x14) != (code *)0x0) {
            (**(code **)(iVar16 + 0x14))();
          }
          *(int *)(iVar16 + 0x3c) = *(int *)(iVar16 + 0x3c) - 1;
          if ((*(int *)(iVar16 + 0x3c) == 0) && (*(code **)(iVar16 + 0xc) != (code *)0x0)) {
            (**(code **)(iVar16 + 0xc))(iVar16);
          }
          uVar5 = uVar5 + 1;
          iVar17 = iVar17 + 0x28;
        } while (uVar5 < *(unsigned int *)(iVar15 + 4));
      }
    }
    FUN_00411a70(lVar7,lVar7);
    lVar7 = 0;
  }
  else {
    *(unsigned int *)lVar7 = 0;
  }
  return lVar7;
}
