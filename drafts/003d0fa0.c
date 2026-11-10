/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d0fa0 func_003d0fa0 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d0fa0_arg0 fn_003d0fa0_arg0;
struct fn_003d0fa0_arg0 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x1c];
    int field_0x2c; /* +0x2c */
    int field_0x30; /* +0x30 */
    int field_0x34; /* +0x34 */
};


unsigned int FUN_003d0fa0(fn_003d0fa0_arg0 *param_1)

{
  unsigned char bVar1;
  short sVar3;
  int iVar4;
  int iVar5;
  unsigned char bVar6;
  unsigned char bVar7;
  unsigned char *puVar8;
  unsigned char bVar9;
  int iVar10;
  unsigned int *puVar11;
  unsigned int *puVar12;
  int iVar13;
  int iVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  int iVar18;
  int iVar19;
  unsigned int uVar20;
  unsigned int uVar21;
  unsigned int uVar22;
  unsigned char *pbVar23;
  unsigned int *puVar24;
  unsigned char *puVar25;
  int iVar26;
  int iVar27;
  float *pfVar28;
  unsigned int *puVar29;
  int iVar30;
  int iVar31;
  int iVar32;
  unsigned char bVar2;
  
  iVar32 = param_1->field_0x8;
  iVar4 = *(int *)(iVar32 + 0x18);
  iVar5 = *(int *)param_1->field_0x30;
  uVar22 = *(unsigned int *)param_1->field_0xc;
  iVar10 = *(int *)(iVar5 + 0x24);
  if ((uVar22 & 2) != 0) {
    iVar16 = 0x752ee9;
    iVar15 = 0x752eec;
    iVar14 = 0x752eef;
    bVar1 = DAT_00752ee8;
    bVar2 = DAT_00752ee7;
  }
  else {
    iVar16 = 0x752ee4;
    iVar15 = 0x752ee5;
    iVar14 = 0x752ee6;
    bVar1 = DAT_00752ee3;
    bVar2 = DAT_00752ee2;
  }
  uVar20 = (unsigned int)bVar1;
  if ((uVar22 & 1) == 0) {
    if ((uVar22 & 8) == 0) {
      iVar18 = 0;
    }
    else {
      iVar18 = 1;
    }
  }
  else {
    iVar18 = 2;
  }
  if ((uVar22 & 0x100) == 0) {
    iVar13 = *(int *)param_1->field_0x2c;
    iVar17 = iVar15;
    iVar15 = iVar16;
  }
  else {
    iVar13 = 0x752ee0;
    iVar17 = iVar16;
  }
  uVar22 = param_1->field_0x34 & 0x10;
  bVar9 = 0;
  if (((uVar22 != 0) && (uVar21 = *(unsigned int *)(param_1->field_0x4 + 0x20), (uVar21 & 1) != 0)) &&
     ((uVar21 & 8) == 0)) {
    bVar9 = 1;
  }
  bVar6 = 0;
  if (((uVar22 != 0) && (uVar22 = *(unsigned int *)(param_1->field_0x4 + 0x28), (uVar22 & 1) != 0)) &&
     ((uVar22 & 8) == 0)) {
    bVar6 = 1;
  }
  bVar7 = 0;
  if ((((param_1->field_0x34 & 0x20U) != 0) &&
      (uVar22 = *(unsigned int *)(param_1->field_0x4 + 0x48), (uVar22 & 1) != 0)) && ((uVar22 & 8) == 0)) {
    bVar7 = 1;
  }
  iVar16 = *(int *)(iVar4 + 0x5c) + *(short *)(iVar32 + 0x50) * 0x1c;
  if (bVar9) {
    iVar26 = *(int *)(iVar4 + 0x18);
    iVar31 = *(int *)(iVar16 + 0x14);
    iVar27 = *(int *)(iVar5 + 0xb0);
    if (1 < iVar26) {
      iVar27 = *(int *)(iVar5 + 0xb4);
    }
    puVar12 = (unsigned int *)(*(int *)(iVar5 + 0x20) + iVar27 * 0x10);
    uVar22 = 0;
    iVar27 = iVar10;
    uVar21 = uVar20;
    while (0 < iVar27) {
      if (*(int *)(iVar5 + 0x30) >= iVar27) {
        iVar19 = 0;
        puVar12 = puVar12 + *(short *)(iVar5 + 0xbc) * -4;
        iVar30 = iVar27;
      }
      else {
        iVar30 = *(int *)(iVar5 + 0xa8);
        if (1 < iVar26) {
          iVar30 = *(int *)(iVar5 + 0xac);
        }
        iVar19 = iVar27 - (iVar30 - iVar18);
      }
      puVar11 = puVar12;
      if (iVar30 != 0) {
        do {
          iVar30 = iVar30 - 1;
          iVar27 = uVar21 << (*(unsigned char *)(iVar17 + uVar22) & 0x1f);
          pbVar23 = (unsigned char *)(iVar14 + uVar22);
          iVar26 = uVar21 << (*(unsigned char *)(iVar15 + uVar22) & 0x1f);
          uVar22 = bVar2 >> (uVar22 << 1) & 3;
          uVar21 = uVar21 + *pbVar23;
          puVar24 = (unsigned int *)
                    (iVar31 + ((iVar26 + (unsigned int)*(unsigned short *)(iVar13 + iVar27 * 2)) & 0xffff) * 0xc);
          *puVar11 = *puVar24;
          puVar11[1] = puVar24[1];
          puVar11[2] = puVar24[2];
          puVar11 = puVar11 + 3;
        } while (iVar30 != 0);
        iVar26 = *(int *)(iVar4 + 0x18);
      }
      else {
        iVar26 = *(int *)(iVar4 + 0x18);
      }
      sVar3 = *(short *)(iVar5 + 0xb8);
      uVar21 = uVar21 - iVar18;
      if (1 < iVar26) {
        sVar3 = *(short *)(iVar5 + 0xba);
      }
      puVar12 = puVar12 + sVar3 * 4;
      iVar27 = iVar19;
    }
  }
  if (bVar6) {
    iVar27 = *(int *)(iVar5 + 200);
    iVar26 = *(int *)(iVar4 + 0x18);
    iVar31 = *(int *)(iVar16 + 0x14);
    if (1 < iVar26) {
      iVar27 = *(int *)(iVar5 + 0xcc);
    }
    puVar12 = (unsigned int *)(*(int *)(iVar5 + 0x20) + iVar27 * 0x10);
    uVar21 = 0;
    iVar27 = iVar10;
    uVar22 = uVar20;
    while (0 < iVar27) {
      if (*(int *)(iVar5 + 0x30) >= iVar27) {
        iVar19 = 0;
        puVar12 = puVar12 + *(short *)(iVar5 + 0xd4) * -4;
        iVar30 = iVar27;
      }
      else {
        iVar30 = *(int *)(iVar5 + 0xc0);
        if (1 < iVar26) {
          iVar30 = *(int *)(iVar5 + 0xc4);
        }
        iVar19 = iVar27 - (iVar30 - iVar18);
      }
      puVar11 = puVar12;
      if (iVar30 != 0) {
        do {
          iVar30 = iVar30 - 1;
          iVar27 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
          pbVar23 = (unsigned char *)(iVar14 + uVar21);
          iVar26 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
          uVar21 = bVar2 >> (uVar21 << 1) & 3;
          uVar22 = uVar22 + *pbVar23;
          puVar24 = (unsigned int *)
                    (iVar31 + ((iVar26 + (unsigned int)*(unsigned short *)(iVar13 + iVar27 * 2)) & 0xffff) * 0xc);
          *puVar11 = *puVar24;
          puVar11[1] = puVar24[1];
          puVar11[2] = puVar24[2];
          puVar11[3] = 0;
          puVar11 = puVar11 + 4;
        } while (iVar30 != 0);
        iVar26 = *(int *)(iVar4 + 0x18);
      }
      else {
        iVar26 = *(int *)(iVar4 + 0x18);
      }
      sVar3 = *(short *)(iVar5 + 0xd0);
      uVar22 = uVar22 - iVar18;
      if (1 < iVar26) {
        sVar3 = *(short *)(iVar5 + 0xd2);
      }
      puVar12 = puVar12 + sVar3 * 4;
      iVar27 = iVar19;
    }
  }
  if (bVar7) {
    iVar31 = *(int *)(iVar5 + 0x128);
    iVar26 = *(int *)(iVar4 + 0x18);
    iVar16 = *(int *)(iVar16 + 0x18);
    if (1 < iVar26) {
      iVar31 = *(int *)(iVar5 + 300);
    }
    puVar25 = (unsigned char *)(*(int *)(iVar5 + 0x20) + iVar31 * 0x10);
    uVar21 = 0;
    iVar31 = iVar10;
    uVar22 = uVar20;
    while (0 < iVar31) {
      if (*(int *)(iVar5 + 0x30) >= iVar31) {
        iVar30 = 0;
        puVar25 = puVar25 + *(short *)(iVar5 + 0x134) * -0x10;
        iVar27 = iVar31;
      }
      else {
        iVar27 = *(int *)(iVar5 + 0x120);
        if (1 < iVar26) {
          iVar27 = *(int *)(iVar5 + 0x124);
        }
        iVar30 = iVar31 - (iVar27 - iVar18);
      }
      if ((*(unsigned int *)(iVar4 + 8) & 0x10) != 0) {
        puVar8 = puVar25;
        if ((*(unsigned int *)(param_1->field_0x4 + 0x48) & 0x2000) != 0) {
          for (; iVar27 != 0; iVar27 = iVar27 - 1) {
            iVar31 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
            pbVar23 = (unsigned char *)(iVar14 + uVar21);
            iVar26 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
            uVar21 = bVar2 >> (uVar21 << 1) & 3;
            uVar22 = uVar22 + *pbVar23;
            pfVar28 = (float *)(iVar16 + (iVar26 + (unsigned int)*(unsigned short *)(iVar13 + iVar31 * 2)) * 0xc);
            *puVar8 = (char)(int)(*pfVar28 * 127.0f);
            puVar8[1] = (char)(int)(pfVar28[1] * 127.0f);
            puVar8[2] = (char)(int)(pfVar28[2] * 127.0f);
            puVar8 = puVar8 + 3;
          }
        }
        else {
          for (; iVar27 != 0; iVar27 = iVar27 - 1) {
            iVar31 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
            pbVar23 = (unsigned char *)(iVar14 + uVar21);
            iVar26 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
            uVar21 = bVar2 >> (uVar21 << 1) & 3;
            uVar22 = uVar22 + *pbVar23;
            pfVar28 = (float *)(iVar16 + (iVar26 + (unsigned int)*(unsigned short *)(iVar13 + iVar31 * 2)) * 0xc);
            *puVar8 = (char)(int)(*pfVar28 * 127.0f);
            puVar8[1] = (char)(int)(pfVar28[1] * 127.0f);
            puVar8[2] = (char)(int)(pfVar28[2] * 127.0f);
            puVar8[3] = 0;
            puVar8 = puVar8 + 4;
          }
        }
      }
      else {
        puVar8 = puVar25;
        if ((*(unsigned int *)(param_1->field_0x4 + 0x48) & 0x2000) != 0) {
          for (; iVar27 != 0; iVar27 = iVar27 - 1) {
            *puVar8 = 0;
            puVar8[1] = 0;
            puVar8[2] = 0;
            puVar8 = puVar8 + 3;
          }
        }
        else {
          for (; iVar27 != 0; iVar27 = iVar27 - 1) {
            *puVar8 = 0;
            puVar8[1] = 0;
            puVar8[2] = 0;
            puVar8[3] = 0;
            puVar8 = puVar8 + 4;
          }
        }
      }
      iVar26 = *(int *)(iVar4 + 0x18);
      sVar3 = *(short *)(iVar5 + 0x130);
      uVar22 = uVar22 - iVar18;
      if (1 < iVar26) {
        sVar3 = *(short *)(iVar5 + 0x132);
      }
      puVar25 = puVar25 + sVar3 * 0x10;
      iVar31 = iVar30;
    }
  }
  *(int *)(iVar5 + 0x3c) = (int)*(short *)(iVar32 + 0x50);
  *(int *)(iVar5 + 0x40) = (int)*(short *)(iVar32 + 0x52);
  *(unsigned int *)(iVar5 + 0x44) = *(unsigned int *)(iVar4 + 0x18);
  if (1 < *(int *)(iVar4 + 0x18)) {
    iVar32 = *(int *)(iVar4 + 0x5c) + *(short *)(iVar32 + 0x52) * 0x1c;
    if (bVar9) {
      uVar21 = 0;
      iVar16 = *(int *)(iVar32 + 0x14);
      puVar12 = (unsigned int *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0x1a4) * 0x10);
      iVar26 = iVar10;
      uVar22 = uVar20;
      while (0 < iVar26) {
        if (*(int *)(iVar5 + 0x30) >= iVar26) {
          iVar27 = 0;
          puVar12 = puVar12 + *(short *)(iVar5 + 0x1ac) * -4;
          iVar31 = iVar26;
        }
        else {
          iVar31 = *(int *)(iVar5 + 0x19c);
          iVar27 = iVar26 - (iVar31 - iVar18);
        }
        puVar11 = puVar12;
        if (iVar31 != 0) {
          do {
            iVar31 = iVar31 - 1;
            iVar30 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
            pbVar23 = (unsigned char *)(iVar14 + uVar21);
            iVar26 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
            uVar21 = bVar2 >> (uVar21 << 1) & 3;
            uVar22 = uVar22 + *pbVar23;
            puVar24 = (unsigned int *)
                      (iVar16 + ((iVar26 + (unsigned int)*(unsigned short *)(iVar13 + iVar30 * 2)) & 0xffff) * 0xc);
            *puVar11 = *puVar24;
            puVar11[1] = puVar24[1];
            puVar11[2] = puVar24[2];
            puVar11 = puVar11 + 3;
          } while (iVar31 != 0);
          sVar3 = *(short *)(iVar5 + 0x1aa);
        }
        else {
          sVar3 = *(short *)(iVar5 + 0x1aa);
        }
        uVar22 = uVar22 - iVar18;
        puVar12 = puVar12 + sVar3 * 4;
        iVar26 = iVar27;
      }
    }
    if (bVar7) {
      uVar21 = 0;
      iVar32 = *(int *)(iVar32 + 0x18);
      puVar25 = (unsigned char *)(*(int *)(iVar5 + 0x20) + *(int *)(iVar5 + 0x1bc) * 0x10);
      iVar16 = iVar10;
      uVar22 = uVar20;
      while (0 < iVar16) {
        if (*(int *)(iVar5 + 0x30) >= iVar16) {
          iVar31 = 0;
          puVar25 = puVar25 + *(short *)(iVar5 + 0x1c4) * -0x10;
          iVar26 = iVar16;
        }
        else {
          iVar26 = *(int *)(iVar5 + 0x1b4);
          iVar31 = iVar16 - (iVar26 - iVar18);
        }
        if ((*(unsigned int *)(iVar4 + 8) & 0x10) != 0) {
          puVar8 = puVar25;
          if ((*(unsigned int *)(param_1->field_0x4 + 0x78) & 0x2000) != 0) {
            for (; iVar26 != 0; iVar26 = iVar26 - 1) {
              iVar27 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
              pbVar23 = (unsigned char *)(iVar14 + uVar21);
              iVar16 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
              uVar21 = bVar2 >> (uVar21 << 1) & 3;
              uVar22 = uVar22 + *pbVar23;
              pfVar28 = (float *)(iVar32 + (iVar16 + (unsigned int)*(unsigned short *)(iVar13 + iVar27 * 2)) * 0xc);
              *puVar8 = (char)(int)(*pfVar28 * 127.0f);
              puVar8[1] = (char)(int)(pfVar28[1] * 127.0f);
              puVar8[2] = (char)(int)(pfVar28[2] * 127.0f);
              puVar8 = puVar8 + 3;
            }
          }
          else {
            for (; iVar26 != 0; iVar26 = iVar26 - 1) {
              iVar27 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
              pbVar23 = (unsigned char *)(iVar14 + uVar21);
              iVar16 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
              uVar21 = bVar2 >> (uVar21 << 1) & 3;
              uVar22 = uVar22 + *pbVar23;
              pfVar28 = (float *)(iVar32 + (iVar16 + (unsigned int)*(unsigned short *)(iVar13 + iVar27 * 2)) * 0xc);
              *puVar8 = (char)(int)(*pfVar28 * 127.0f);
              puVar8[1] = (char)(int)(pfVar28[1] * 127.0f);
              puVar8[2] = (char)(int)(pfVar28[2] * 127.0f);
              puVar8[3] = 0;
              puVar8 = puVar8 + 4;
            }
          }
        }
        else {
          puVar8 = puVar25;
          if ((*(unsigned int *)(param_1->field_0x4 + 0x78) & 0x2000) != 0) {
            for (; iVar26 != 0; iVar26 = iVar26 - 1) {
              *puVar8 = 0;
              puVar8[1] = 0;
              puVar8[2] = 0;
              puVar8 = puVar8 + 3;
            }
          }
          else {
            for (; iVar26 != 0; iVar26 = iVar26 - 1) {
              *puVar8 = 0;
              puVar8[1] = 0;
              puVar8[2] = 0;
              puVar8[3] = 0;
              puVar8 = puVar8 + 4;
            }
          }
        }
        uVar22 = uVar22 - iVar18;
        puVar25 = puVar25 + *(short *)(iVar5 + 0x1c2) * 0x10;
        iVar16 = iVar31;
      }
    }
  }
  if (((((param_1->field_0x34 & 0x180U) != 0) &&
       (uVar22 = *(unsigned int *)(param_1->field_0x4 + 0x30), (uVar22 & 1) != 0)) && ((uVar22 & 8) == 0))
     && ((*(unsigned int *)(iVar4 + 8) & 0x84) != 0)) {
    iVar26 = *(int *)(iVar5 + 0xe0);
    iVar32 = *(int *)(iVar4 + 0x18);
    iVar16 = *(int *)(iVar4 + 0x34);
    if (1 < iVar32) {
      iVar26 = *(int *)(iVar5 + 0xe4);
    }
    puVar12 = (unsigned int *)(*(int *)(iVar5 + 0x20) + iVar26 * 0x10);
    uVar21 = 0;
    iVar26 = iVar10;
    uVar22 = uVar20;
    while (0 < iVar26) {
      if (*(int *)(iVar5 + 0x30) >= iVar26) {
        iVar27 = 0;
        puVar12 = puVar12 + *(short *)(iVar5 + 0xec) * -4;
        iVar31 = iVar26;
      }
      else {
        iVar31 = *(int *)(iVar5 + 0xd8);
        if (1 < iVar32) {
          iVar31 = *(int *)(iVar5 + 0xdc);
        }
        iVar27 = iVar26 - (iVar31 - iVar18);
      }
      puVar11 = puVar12;
      if (iVar31 != 0) {
        do {
          iVar31 = iVar31 - 1;
          iVar26 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
          pbVar23 = (unsigned char *)(iVar14 + uVar21);
          iVar32 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
          uVar21 = bVar2 >> (uVar21 << 1) & 3;
          uVar22 = uVar22 + *pbVar23;
          puVar24 = (unsigned int *)
                    (iVar16 + ((iVar32 + (unsigned int)*(unsigned short *)(iVar13 + iVar26 * 2)) & 0xffff) * 8);
          *puVar11 = *puVar24;
          puVar11[1] = puVar24[1];
          puVar11 = puVar11 + 2;
        } while (iVar31 != 0);
        iVar32 = *(int *)(iVar4 + 0x18);
      }
      else {
        iVar32 = *(int *)(iVar4 + 0x18);
      }
      sVar3 = *(short *)(iVar5 + 0xe8);
      uVar22 = uVar22 - iVar18;
      if (1 < iVar32) {
        sVar3 = *(short *)(iVar5 + 0xea);
      }
      puVar12 = puVar12 + sVar3 * 4;
      iVar26 = iVar27;
    }
  }
  if ((((param_1->field_0x34 & 0x180U) != 0) &&
      (uVar22 = *(unsigned int *)(param_1->field_0x4 + 0x38), (uVar22 & 1) != 0)) &&
     (((uVar22 & 8) == 0 && ((*(unsigned int *)(iVar4 + 8) & 0x84) != 0)))) {
    iVar32 = *(int *)(iVar4 + 0x34);
    iVar16 = iVar32;
    if ((*(unsigned int *)(iVar4 + 8) & 0x80) != 0) {
      iVar16 = *(int *)(iVar4 + 0x38);
    }
    iVar31 = *(int *)(iVar5 + 0xf8);
    iVar26 = *(int *)(iVar4 + 0x18);
    if (1 < iVar26) {
      iVar31 = *(int *)(iVar5 + 0xfc);
    }
    puVar12 = (unsigned int *)(*(int *)(iVar5 + 0x20) + iVar31 * 0x10);
    uVar21 = 0;
    iVar31 = iVar10;
    uVar22 = uVar20;
    while (0 < iVar31) {
      if (*(int *)(iVar5 + 0x30) >= iVar31) {
        iVar30 = 0;
        puVar12 = puVar12 + *(short *)(iVar5 + 0x104) * -4;
        iVar27 = iVar31;
      }
      else {
        iVar27 = *(int *)(iVar5 + 0xf0);
        if (1 < iVar26) {
          iVar27 = *(int *)(iVar5 + 0xf4);
        }
        iVar30 = iVar31 - (iVar27 - iVar18);
      }
      puVar11 = puVar12;
      if (iVar27 != 0) {
        do {
          iVar27 = iVar27 - 1;
          iVar31 = uVar22 << (*(unsigned char *)(iVar17 + uVar21) & 0x1f);
          pbVar23 = (unsigned char *)(iVar14 + uVar21);
          iVar26 = uVar22 << (*(unsigned char *)(iVar15 + uVar21) & 0x1f);
          uVar21 = bVar2 >> (uVar21 << 1) & 3;
          uVar22 = uVar22 + *pbVar23;
          iVar26 = ((iVar26 + (unsigned int)*(unsigned short *)(iVar13 + iVar31 * 2)) & 0xffff) * 8;
          puVar29 = (unsigned int *)(iVar32 + iVar26);
          puVar24 = (unsigned int *)(iVar16 + iVar26);
          *puVar11 = *puVar29;
          puVar11[1] = puVar29[1];
          puVar11[2] = *puVar24;
          puVar11[3] = puVar24[1];
          puVar11 = puVar11 + 4;
        } while (iVar27 != 0);
        iVar26 = *(int *)(iVar4 + 0x18);
      }
      else {
        iVar26 = *(int *)(iVar4 + 0x18);
      }
      sVar3 = *(short *)(iVar5 + 0x100);
      uVar22 = uVar22 - iVar18;
      if (1 < iVar26) {
        sVar3 = *(short *)(iVar5 + 0x102);
      }
      puVar12 = puVar12 + sVar3 * 4;
      iVar31 = iVar30;
    }
  }
  if ((((param_1->field_0x34 & 0x40U) != 0) &&
      (uVar22 = *(unsigned int *)(param_1->field_0x4 + 0x40), (uVar22 & 1) != 0)) && ((uVar22 & 8) == 0)) {
    iVar26 = *(int *)(iVar5 + 0x110);
    iVar32 = *(int *)(iVar4 + 0x18);
    iVar16 = *(int *)(iVar4 + 0x30);
    if (1 < iVar32) {
      iVar26 = *(int *)(iVar5 + 0x114);
    }
    puVar12 = (unsigned int *)(*(int *)(iVar5 + 0x20) + iVar26 * 0x10);
    uVar22 = 0;
    while (0 < iVar10) {
      if (*(int *)(iVar5 + 0x30) >= iVar10) {
        iVar31 = 0;
        puVar12 = puVar12 + *(short *)(iVar5 + 0x11c) * -4;
        iVar26 = iVar10;
      }
      else {
        iVar26 = *(int *)(iVar5 + 0x108);
        if (1 < iVar32) {
          iVar26 = *(int *)(iVar5 + 0x10c);
        }
        iVar31 = iVar10 - (iVar26 - iVar18);
      }
      puVar11 = puVar12;
      if ((*(unsigned int *)(iVar4 + 8) & 8) != 0) {
        for (; iVar26 != 0; iVar26 = iVar26 - 1) {
          pbVar23 = (unsigned char *)(iVar14 + uVar22);
          iVar32 = uVar20 << (*(unsigned char *)(iVar17 + uVar22) & 0x1f);
          iVar10 = uVar20 << (*(unsigned char *)(iVar15 + uVar22) & 0x1f);
          uVar22 = bVar2 >> (uVar22 << 1) & 3;
          uVar20 = uVar20 + *pbVar23;
          *puVar11 = *(unsigned int *)
                      (iVar16 + ((iVar10 + (unsigned int)*(unsigned short *)(iVar13 + iVar32 * 2)) & 0xffff) * 4);
          puVar11 = puVar11 + 1;
        }
      }
      else {
        for (; iVar26 != 0; iVar26 = iVar26 - 1) {
          *puVar11 = 0xff000000;
          puVar11 = puVar11 + 1;
        }
      }
      iVar32 = *(int *)(iVar4 + 0x18);
      sVar3 = *(short *)(iVar5 + 0x118);
      uVar20 = uVar20 - iVar18;
      if (1 < iVar32) {
        sVar3 = *(short *)(iVar5 + 0x11a);
      }
      puVar12 = puVar12 + sVar3 * 4;
      iVar10 = iVar31;
    }
  }
  return 1;
}
