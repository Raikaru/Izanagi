/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d2720 func_003d2720 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d2720_arg0 fn_003d2720_arg0;
struct fn_003d2720_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x1c];
    int field_0x2c; /* +0x2c */
    int field_0x30; /* +0x30 */
    int field_0x34; /* +0x34 */
};


unsigned int FUN_003d2720(fn_003d2720_arg0 *param_1)

{
  unsigned char bVar1;
  short sVar3;
  unsigned int *puVar4;
  int iVar5;
  unsigned int uVar6;
  int iVar7;
  int iVar8;
  unsigned int *puVar9;
  unsigned int *puVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  unsigned int uVar17;
  int iVar18;
  unsigned char *pbVar19;
  unsigned int uVar20;
  unsigned int *puVar21;
  unsigned char *pbVar22;
  int iVar23;
  unsigned int uVar24;
  unsigned int uVar25;
  unsigned int uVar26;
  unsigned char bVar2;
  
  puVar4 = (unsigned int *)param_1->field_0x8;
  iVar5 = *(int *)param_1->field_0x30;
  uVar6 = *(unsigned int *)param_1->field_0xc;
  iVar7 = *(int *)(iVar5 + 0x24);
  if ((uVar6 & 2) != 0) {
    iVar14 = 0x752f09;
    iVar13 = 0x752f0c;
    iVar12 = 0x752f0f;
    bVar1 = DAT_00752f08;
    bVar2 = DAT_00752f07;
  }
  else {
    iVar14 = 0x752f04;
    iVar13 = 0x752f05;
    iVar12 = 0x752f06;
    bVar1 = DAT_00752f03;
    bVar2 = DAT_00752f02;
  }
  uVar17 = (unsigned int)bVar1;
  if ((uVar6 & 1) == 0) {
    if ((uVar6 & 8) == 0) {
      iVar16 = 0;
    }
    else {
      iVar16 = 1;
    }
  }
  else {
    iVar16 = 2;
  }
  if ((uVar6 & 0x100) == 0) {
    iVar11 = *(int *)param_1->field_0x2c;
    iVar15 = iVar13;
    iVar13 = iVar14;
  }
  else {
    iVar11 = 0x752f00;
    iVar15 = iVar14;
  }
  if (((param_1->field_0x34 & 0x10U) != 0) && ((*puVar4 & 8) != 0)) {
    uVar20 = 0;
    uVar6 = puVar4[3];
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0xb0) * 0x10);
    iVar14 = iVar7;
    uVar24 = uVar17;
    while (0 < iVar14) {
      if (*(int *)(iVar5 + 0x30) >= iVar14) {
        iVar8 = 0;
        puVar10 = puVar10 + *(short *)(iVar5 + 0xbc) * -4;
        iVar18 = iVar14;
      }
      else {
        iVar18 = *(int *)(iVar5 + 0xa8);
        iVar8 = iVar14 - (iVar18 - iVar16);
      }
      puVar9 = puVar10;
      if (iVar18 != 0) {
        do {
          iVar18 = iVar18 - 1;
          iVar23 = uVar24 << (*(unsigned char *)(iVar15 + uVar20) & 0x1f);
          pbVar19 = (unsigned char *)(iVar12 + uVar20);
          iVar14 = uVar24 << (*(unsigned char *)(iVar13 + uVar20) & 0x1f);
          uVar20 = bVar2 >> (uVar20 << 1) & 3;
          uVar24 = uVar24 + *pbVar19;
          puVar21 = (unsigned int *)
                    (uVar6 + ((iVar14 + (unsigned int)*(unsigned short *)(iVar11 + iVar23 * 2)) & 0xffff) * 0x24);
          uVar25 = puVar21[1];
          uVar26 = puVar21[2];
          *puVar9 = *puVar21;
          puVar9[1] = uVar25;
          puVar9[2] = uVar26;
          puVar9 = puVar9 + 3;
        } while (iVar18 != 0);
        sVar3 = *(short *)(iVar5 + 0xb8);
      }
      else {
        sVar3 = *(short *)(iVar5 + 0xb8);
      }
      uVar24 = uVar24 - iVar16;
      puVar10 = puVar10 + sVar3 * 4;
      iVar14 = iVar8;
    }
  }
  if (((param_1->field_0x34 & 0x80U) != 0) && ((*puVar4 & 1) != 0)) {
    uVar24 = 0;
    uVar6 = puVar4[3];
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0xe0) * 0x10);
    iVar14 = iVar7;
    uVar20 = uVar17;
    while (0 < iVar14) {
      if (*(int *)(iVar5 + 0x30) >= iVar14) {
        iVar8 = 0;
        puVar10 = puVar10 + *(short *)(iVar5 + 0xec) * -4;
        iVar18 = iVar14;
      }
      else {
        iVar18 = *(int *)(iVar5 + 0xd8);
        iVar8 = iVar14 - (iVar18 - iVar16);
      }
      puVar9 = puVar10;
      if (iVar18 != 0) {
        do {
          iVar18 = iVar18 - 1;
          iVar23 = uVar20 << (*(unsigned char *)(iVar15 + uVar24) & 0x1f);
          pbVar19 = (unsigned char *)(iVar12 + uVar24);
          iVar14 = uVar20 << (*(unsigned char *)(iVar13 + uVar24) & 0x1f);
          uVar24 = bVar2 >> (uVar24 << 1) & 3;
          uVar20 = uVar20 + *pbVar19;
          iVar14 = uVar6 + ((iVar14 + (unsigned int)*(unsigned short *)(iVar11 + iVar23 * 2)) & 0xffff) * 0x24;
          *puVar9 = *(unsigned int *)(iVar14 + 0x1c);
          puVar9[1] = *(unsigned int *)(iVar14 + 0x20);
          puVar9 = puVar9 + 2;
        } while (iVar18 != 0);
        sVar3 = *(short *)(iVar5 + 0xe8);
      }
      else {
        sVar3 = *(short *)(iVar5 + 0xe8);
      }
      uVar20 = uVar20 - iVar16;
      puVar10 = puVar10 + sVar3 * 4;
      iVar14 = iVar8;
    }
  }
  if (((param_1->field_0x34 & 0x40U) != 0) && ((*puVar4 & 0x10) != 0)) {
    uVar20 = 0;
    uVar6 = puVar4[3];
    puVar10 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0x110) * 0x10);
    while (0 < iVar7) {
      if (*(int *)(iVar5 + 0x30) >= iVar7) {
        iVar18 = 0;
        puVar10 = puVar10 + *(short *)(iVar5 + 0x11c) * -4;
        iVar14 = iVar7;
      }
      else {
        iVar14 = *(int *)(iVar5 + 0x108);
        iVar18 = iVar7 - (iVar14 - iVar16);
      }
      puVar9 = puVar10;
      if (iVar14 != 0) {
        do {
          iVar14 = iVar14 - 1;
          pbVar22 = (unsigned char *)(iVar12 + uVar20);
          iVar8 = uVar17 << (*(unsigned char *)(iVar15 + uVar20) & 0x1f);
          pbVar19 = (unsigned char *)(iVar13 + uVar20);
          uVar20 = bVar2 >> (uVar20 << 1) & 3;
          iVar7 = uVar17 << (*pbVar19 & 0x1f);
          uVar17 = uVar17 + *pbVar22;
          *puVar9 = *(unsigned int *)
                     (uVar6 + ((iVar7 + (unsigned int)*(unsigned short *)(iVar11 + iVar8 * 2)) & 0xffff) * 0x24 +
                     0xc);
          puVar9 = puVar9 + 1;
        } while (iVar14 != 0);
        sVar3 = *(short *)(iVar5 + 0x118);
      }
      else {
        sVar3 = *(short *)(iVar5 + 0x118);
      }
      uVar17 = uVar17 - iVar16;
      puVar10 = puVar10 + sVar3 * 4;
      iVar7 = iVar18;
    }
  }
  return 1;
}
