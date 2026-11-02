/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d3e60 func_003d3e60 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d3e60_arg0 fn_003d3e60_arg0;
struct fn_003d3e60_arg0 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x1c];
    int field_0x2c; /* +0x2c */
    int field_0x30; /* +0x30 */
    int field_0x34; /* +0x34 */
};


unsigned int FUN_003d3e60(fn_003d3e60_arg0 *param_1)

{
  unsigned char bVar1;
  short sVar3;
  int iVar4;
  int iVar5;
  unsigned char *puVar6;
  int iVar7;
  int iVar8;
  unsigned int *puVar9;
  unsigned int *puVar10;
  unsigned int uVar11;
  unsigned int uVar12;
  int iVar13;
  unsigned char *puVar14;
  unsigned char *pbVar15;
  unsigned char *puVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  int iVar20;
  int iVar21;
  unsigned int *puVar22;
  int iVar23;
  int iVar24;
  int iVar25;
  unsigned int *puVar26;
  int iVar27;
  int iVar28;
  unsigned int uVar29;
  unsigned char bVar2;
  
  iVar7 = DAT_008872e4;
  iVar4 = param_1->field_0x8;
  iVar5 = *(int *)param_1->field_0x30;
  uVar12 = *(unsigned int *)param_1->field_0xc;
  iVar8 = *(int *)(iVar5 + 0x24);
  if ((uVar12 & 2) != 0) {
    iVar20 = 0x752f79;
    iVar19 = 0x752f7c;
    iVar18 = 0x752f7f;
    bVar1 = DAT_00752f77;
    bVar2 = DAT_00752f78;
  }
  else {
    iVar20 = 0x752f74;
    iVar19 = 0x752f75;
    iVar18 = 0x752f76;
    bVar1 = DAT_00752f72;
    bVar2 = DAT_00752f73;
  }
  uVar29 = (unsigned int)bVar2;
  if ((uVar12 & 1) == 0) {
    if ((uVar12 & 8) == 0) {
      iVar28 = 0;
    }
    else {
      iVar28 = 1;
    }
  }
  else {
    iVar28 = 2;
  }
  if ((uVar12 & 0x100) == 0) {
    iVar17 = *(int *)param_1->field_0x2c;
    iVar21 = iVar19;
    iVar19 = iVar20;
  }
  else {
    iVar17 = 0x752f70;
    iVar21 = iVar20;
  }
  if ((((param_1->field_0x34 & 0x10U) != 0) &&
      (uVar12 = *(unsigned int *)(param_1->field_0x4 + 0x20), (uVar12 & 1) != 0)) && ((uVar12 & 8) == 0)) {
    uVar12 = 0;
    iVar20 = *(int *)(iVar4 + 8);
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0xb0) * 0x10);
    iVar23 = iVar8;
    uVar11 = uVar29;
    while (0 < iVar23) {
      if (*(int *)(iVar5 + 0x30) >= iVar23) {
        iVar24 = 0;
        puVar10 = puVar10 + *(short *)(iVar5 + 0xbc) * -4;
        iVar13 = iVar23;
      }
      else {
        iVar13 = *(int *)(iVar5 + 0xa8);
        iVar24 = iVar23 - (iVar13 - iVar28);
      }
      puVar9 = puVar10;
      if (iVar13 != 0) {
        do {
          iVar13 = iVar13 - 1;
          iVar25 = uVar11 << (*(unsigned char *)(iVar21 + uVar12) & 0x1f);
          pbVar15 = (unsigned char *)(iVar18 + uVar12);
          iVar23 = uVar11 << (*(unsigned char *)(iVar19 + uVar12) & 0x1f);
          uVar12 = bVar1 >> (uVar12 << 1) & 3;
          uVar11 = uVar11 + *pbVar15;
          puVar22 = (unsigned int *)
                    (iVar20 + ((iVar23 + (unsigned int)*(unsigned short *)(iVar17 + iVar25 * 2)) & 0xffff) * 0xc);
          *puVar9 = *puVar22;
          puVar9[1] = puVar22[1];
          puVar9[2] = puVar22[2];
          puVar9 = puVar9 + 3;
        } while (iVar13 != 0);
        sVar3 = *(short *)(iVar5 + 0xb8);
      }
      else {
        sVar3 = *(short *)(iVar5 + 0xb8);
      }
      uVar11 = uVar11 - iVar28;
      puVar10 = puVar10 + sVar3 * 4;
      iVar23 = iVar24;
    }
  }
  if ((((param_1->field_0x34 & 0x10U) != 0) &&
      (uVar12 = *(unsigned int *)(param_1->field_0x4 + 0x28), (uVar12 & 1) != 0)) && ((uVar12 & 8) == 0)) {
    uVar11 = 0;
    iVar20 = *(int *)(iVar4 + 8);
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 200) * 0x10);
    iVar23 = iVar8;
    uVar12 = uVar29;
    while (0 < iVar23) {
      if (*(int *)(iVar5 + 0x30) >= iVar23) {
        puVar10 = puVar10 + *(short *)(iVar5 + 0xd4) * -4;
        iVar13 = iVar23;
        puVar9 = puVar10;
        iVar23 = 0;
      }
      else {
        iVar13 = *(int *)(iVar5 + 0xc0);
        iVar23 = iVar23 - (iVar13 - iVar28);
        puVar9 = puVar10;
      }
      for (; iVar13 != 0; iVar13 = iVar13 - 1) {
        iVar25 = uVar12 << (*(unsigned char *)(iVar21 + uVar11) & 0x1f);
        pbVar15 = (unsigned char *)(iVar18 + uVar11);
        iVar24 = uVar12 << (*(unsigned char *)(iVar19 + uVar11) & 0x1f);
        uVar11 = bVar1 >> (uVar11 << 1) & 3;
        uVar12 = uVar12 + *pbVar15;
        puVar22 = (unsigned int *)
                  (iVar20 + ((iVar24 + (unsigned int)*(unsigned short *)(iVar17 + iVar25 * 2)) & 0xffff) * 0xc);
        *puVar10 = *puVar22;
        puVar10[1] = puVar22[1];
        puVar10[2] = puVar22[2];
        puVar10[3] = 0;
        puVar10 = puVar10 + 4;
      }
      uVar12 = uVar12 - iVar28;
      puVar10 = puVar9 + *(short *)(iVar5 + 0xd0) * 4;
    }
  }
  if ((((param_1->field_0x34 & 0x180U) != 0) &&
      (uVar12 = *(unsigned int *)(param_1->field_0x4 + 0x30), (uVar12 & 1) != 0)) &&
     (((uVar12 & 8) == 0 && ((*(unsigned int *)(iVar7 + 8) & 0x84) != 0)))) {
    uVar11 = 0;
    iVar20 = *(int *)(iVar4 + 0x10);
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0xe0) * 0x10);
    iVar23 = iVar8;
    uVar12 = uVar29;
    while (0 < iVar23) {
      if (*(int *)(iVar5 + 0x30) >= iVar23) {
        puVar10 = puVar10 + *(short *)(iVar5 + 0xec) * -4;
        iVar13 = iVar23;
        puVar9 = puVar10;
        iVar23 = 0;
      }
      else {
        iVar13 = *(int *)(iVar5 + 0xd8);
        iVar23 = iVar23 - (iVar13 - iVar28);
        puVar9 = puVar10;
      }
      for (; iVar13 != 0; iVar13 = iVar13 - 1) {
        iVar25 = uVar12 << (*(unsigned char *)(iVar21 + uVar11) & 0x1f);
        pbVar15 = (unsigned char *)(iVar18 + uVar11);
        iVar24 = uVar12 << (*(unsigned char *)(iVar19 + uVar11) & 0x1f);
        uVar11 = bVar1 >> (uVar11 << 1) & 3;
        uVar12 = uVar12 + *pbVar15;
        puVar22 = (unsigned int *)
                  (iVar20 + ((iVar24 + (unsigned int)*(unsigned short *)(iVar17 + iVar25 * 2)) & 0xffff) * 8);
        *puVar10 = *puVar22;
        puVar10[1] = puVar22[1];
        puVar10 = puVar10 + 2;
      }
      uVar12 = uVar12 - iVar28;
      puVar10 = puVar9 + *(short *)(iVar5 + 0xe8) * 4;
    }
  }
  if (((((param_1->field_0x34 & 0x180U) != 0) &&
       (uVar12 = *(unsigned int *)(param_1->field_0x4 + 0x38), (uVar12 & 1) != 0)) && ((uVar12 & 8) == 0))
     && ((*(unsigned int *)(iVar7 + 8) & 0x84) != 0)) {
    iVar20 = *(int *)(iVar4 + 0x10);
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0xf8) * 0x10);
    iVar23 = iVar20;
    if ((*(unsigned int *)(iVar7 + 8) & 0x80) != 0) {
      iVar23 = *(int *)(iVar4 + 0x14);
    }
    uVar11 = 0;
    iVar13 = iVar8;
    uVar12 = uVar29;
    while (0 < iVar13) {
      if (*(int *)(iVar5 + 0x30) >= iVar13) {
        iVar25 = 0;
        puVar10 = puVar10 + *(short *)(iVar5 + 0x104) * -4;
        iVar24 = iVar13;
      }
      else {
        iVar24 = *(int *)(iVar5 + 0xf0);
        iVar25 = iVar13 - (iVar24 - iVar28);
      }
      puVar9 = puVar10;
      if (iVar24 != 0) {
        do {
          iVar24 = iVar24 - 1;
          iVar27 = uVar12 << (*(unsigned char *)(iVar21 + uVar11) & 0x1f);
          pbVar15 = (unsigned char *)(iVar18 + uVar11);
          iVar13 = uVar12 << (*(unsigned char *)(iVar19 + uVar11) & 0x1f);
          uVar11 = bVar1 >> (uVar11 << 1) & 3;
          uVar12 = uVar12 + *pbVar15;
          iVar13 = ((iVar13 + (unsigned int)*(unsigned short *)(iVar17 + iVar27 * 2)) & 0xffff) * 8;
          puVar26 = (unsigned int *)(iVar20 + iVar13);
          puVar22 = (unsigned int *)(iVar23 + iVar13);
          *puVar9 = *puVar26;
          puVar9[1] = puVar26[1];
          puVar9[2] = *puVar22;
          puVar9[3] = puVar22[1];
          puVar9 = puVar9 + 4;
        } while (iVar24 != 0);
        sVar3 = *(short *)(iVar5 + 0x100);
      }
      else {
        sVar3 = *(short *)(iVar5 + 0x100);
      }
      uVar12 = uVar12 - iVar28;
      puVar10 = puVar10 + sVar3 * 4;
      iVar13 = iVar25;
    }
  }
  if ((((param_1->field_0x34 & 0x40U) != 0) &&
      (uVar12 = *(unsigned int *)(param_1->field_0x4 + 0x40), (uVar12 & 1) != 0)) && ((uVar12 & 8) == 0)) {
    uVar11 = 0;
    iVar20 = *(int *)(iVar4 + 0x30);
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0x110) * 0x10);
    iVar23 = iVar8;
    uVar12 = uVar29;
    while (0 < iVar23) {
      if (*(int *)(iVar5 + 0x30) >= iVar23) {
        iVar24 = 0;
        puVar10 = puVar10 + *(short *)(iVar5 + 0x11c) * -4;
        iVar13 = iVar23;
      }
      else {
        iVar13 = *(int *)(iVar5 + 0x108);
        iVar24 = iVar23 - (iVar13 - iVar28);
      }
      puVar9 = puVar10;
      if ((*(unsigned int *)(iVar7 + 8) & 8) != 0) {
        for (; iVar13 != 0; iVar13 = iVar13 - 1) {
          iVar25 = uVar12 << (*(unsigned char *)(iVar21 + uVar11) & 0x1f);
          pbVar15 = (unsigned char *)(iVar18 + uVar11);
          iVar23 = uVar12 << (*(unsigned char *)(iVar19 + uVar11) & 0x1f);
          uVar11 = bVar1 >> (uVar11 << 1) & 3;
          uVar12 = uVar12 + *pbVar15;
          *puVar9 = *(unsigned int *)
                     (iVar20 + ((iVar23 + (unsigned int)*(unsigned short *)(iVar17 + iVar25 * 2)) & 0xffff) * 4);
          puVar9 = puVar9 + 1;
        }
      }
      else {
        for (; iVar13 != 0; iVar13 = iVar13 - 1) {
          *puVar9 = 0xff000000;
          puVar9 = puVar9 + 1;
        }
      }
      uVar12 = uVar12 - iVar28;
      puVar10 = puVar10 + *(short *)(iVar5 + 0x118) * 4;
      iVar23 = iVar24;
    }
  }
  if ((((param_1->field_0x34 & 0x20U) != 0) &&
      (uVar12 = *(unsigned int *)(param_1->field_0x4 + 0x48), (uVar12 & 1) != 0)) && ((uVar12 & 8) == 0)) {
    uVar12 = 0;
    iVar4 = *(int *)(iVar4 + 0xc);
    puVar14 = (unsigned char *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0x128) * 0x10);
    while (0 < iVar8) {
      if (*(int *)(iVar5 + 0x30) >= iVar8) {
        iVar23 = 0;
        puVar14 = puVar14 + *(short *)(iVar5 + 0x134) * -0x10;
        iVar20 = iVar8;
      }
      else {
        iVar20 = *(int *)(iVar5 + 0x120);
        iVar23 = iVar8 - (iVar20 - iVar28);
      }
      if ((*(unsigned int *)(iVar7 + 8) & 0x10) != 0) {
        puVar6 = puVar14;
        if ((*(unsigned int *)(param_1->field_0x4 + 0x48) & 0x2000) != 0) {
          for (; iVar20 != 0; iVar20 = iVar20 - 1) {
            pbVar15 = (unsigned char *)(iVar18 + uVar12);
            iVar8 = uVar29 << (*(unsigned char *)(iVar21 + uVar12) & 0x1f);
            iVar13 = uVar29 << (*(unsigned char *)(iVar19 + uVar12) & 0x1f);
            uVar12 = bVar1 >> (uVar12 << 1) & 3;
            uVar29 = uVar29 + *pbVar15;
            puVar16 = (unsigned char *)(iVar4 + (iVar13 + (unsigned int)*(unsigned short *)(iVar17 + iVar8 * 2)) * 4);
            *puVar6 = *puVar16;
            puVar6[1] = puVar16[1];
            puVar6[2] = puVar16[2];
            puVar6 = puVar6 + 3;
          }
        }
        else {
          for (; iVar20 != 0; iVar20 = iVar20 - 1) {
            pbVar15 = (unsigned char *)(iVar18 + uVar12);
            iVar8 = uVar29 << (*(unsigned char *)(iVar21 + uVar12) & 0x1f);
            iVar13 = uVar29 << (*(unsigned char *)(iVar19 + uVar12) & 0x1f);
            uVar12 = bVar1 >> (uVar12 << 1) & 3;
            uVar29 = uVar29 + *pbVar15;
            puVar16 = (unsigned char *)(iVar4 + (iVar13 + (unsigned int)*(unsigned short *)(iVar17 + iVar8 * 2)) * 4);
            *puVar6 = *puVar16;
            puVar6[1] = puVar16[1];
            puVar6[2] = puVar16[2];
            puVar6[3] = 0;
            puVar6 = puVar6 + 4;
          }
        }
      }
      else {
        puVar6 = puVar14;
        if ((*(unsigned int *)(param_1->field_0x4 + 0x48) & 0x2000) != 0) {
          for (; iVar20 != 0; iVar20 = iVar20 - 1) {
            *puVar6 = 0;
            puVar6[1] = 0;
            puVar6[2] = 0;
            puVar6 = puVar6 + 3;
          }
        }
        else {
          for (; iVar20 != 0; iVar20 = iVar20 - 1) {
            *puVar6 = 0;
            puVar6[1] = 0;
            puVar6[2] = 0;
            puVar6[3] = 0;
            puVar6 = puVar6 + 4;
          }
        }
      }
      uVar29 = uVar29 - iVar28;
      puVar14 = puVar14 + *(short *)(iVar5 + 0x130) * 0x10;
      iVar8 = iVar23;
    }
  }
  return 1;
}
