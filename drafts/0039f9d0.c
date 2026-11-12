/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039f9d0 func_0039f9d0 src/promoted/code1_0039.c - romwright draft, unreviewed */
typedef struct fn_0039f9d0_arg0 fn_0039f9d0_arg0;
struct fn_0039f9d0_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    int field_0x10; /* +0x10 */
    int field_0x14; /* +0x14 */
    int field_0x18; /* +0x18 */
    float field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
    unsigned char pad_0x24[0x4];
    byte field_0x28; /* +0x28 */
    undefined1 field_0x29; /* +0x29 */
    undefined1 field_0x2a; /* +0x2a */
};


unsigned long long FUN_0039f9d0(fn_0039f9d0_arg0 *param_1,unsigned int *param_2)

{
  unsigned char bVar1;
  int iVar2;
  unsigned int *puVar3;
  unsigned int uVar4;
  unsigned int uVar5;
  unsigned int *puVar6;
  unsigned int *puVar7;
  unsigned long long uVar8;
  unsigned char auVar9 [16];
  unsigned int uVar10;
  unsigned int uVar11;
  unsigned int uVar12;
  int *piVar13;
  unsigned int uVar14;
  float *pfVar15;
  unsigned long long *puVar16;
  int iVar17;
  float *pfVar18;
  unsigned long long uVar19;
  unsigned long long extraout_v0_udw;
  unsigned char auVar20 [16];
  unsigned char auVar21 [16];
  unsigned long long in_a0_udw;
  unsigned int in_a1_udw;
  unsigned int in_a1_udw_00;
  unsigned int uVar22;
  unsigned int uVar23;
  unsigned char auVar24 [16];
  unsigned int uVar25;
  unsigned int uVar26;
  unsigned int uVar27;
  unsigned int uVar28;
  unsigned int uVar29;
  unsigned int uVar30;
  int iVar31;
  unsigned int uVar32;
  int iVar33;
  float fVar34;
  float fVar35;
  float fVar36;
  float fVar37;
  unsigned char uStack00000000;
  int aiStack_70 [12];
  float fStack_40;
  float fStack_3c;
  float fStack_38;
  float fStack_20;
  float fStack_1c;
  
  iVar2 = param_1->field_0x8;
  iVar33 = *(int *)(iVar2 + 0x18);
  puVar3 = *(unsigned int **)(iVar33 + DAT_00886498);
  uVar4 = *puVar3;
  param_1->field_0xc = *(int *)(iVar33 + 0x54);
  if (*(int *)(iVar33 + 0x18) == 1) {
    piVar13 = (int *)(iVar33 + DAT_00764820);
    bVar1 = 0;
    if (((unsigned int *)*piVar13 != (unsigned int *)0x0) &&
       ((unsigned int)*(unsigned short *)(*(int *)(iVar33 + 0x54) + 4) == *(unsigned int *)*piVar13)) {
      bVar1 = 1;
    }
    if (bVar1) {
      iVar33 = *piVar13;
    }
    else {
      piVar13 = (int *)FUN_003ce050((short)piVar13,*(unsigned short *)(*(int *)(iVar33 + 0x54) + 4));
      iVar33 = *piVar13;
    }
    param_1->field_0x10 = iVar33;
  }
  else {
    piVar13 = (int *)(iVar2 + DAT_0076481c);
    bVar1 = 0;
    if (((unsigned int *)*piVar13 != (unsigned int *)0x0) &&
       ((unsigned int)*(unsigned short *)(*(int *)(iVar33 + 0x54) + 4) == *(unsigned int *)*piVar13)) {
      bVar1 = 1;
    }
    if (bVar1) {
      iVar33 = *piVar13;
    }
    else {
      piVar13 = (int *)FUN_003ce050((short)piVar13,*(unsigned short *)(*(int *)(iVar33 + 0x54) + 4));
      iVar33 = *piVar13;
    }
    param_1->field_0x10 = iVar33;
  }
  if (*(short *)(param_1->field_0xc + 4) != 0) {
    param_1->field_0x20 = *(int *)(*(int *)(iVar2 + 0x18) + 0x18);
    if (1 < param_1->field_0x20) {
      param_1->field_0x1c = *(float *)(iVar2 + 0x58) * *(float *)(iVar2 + 0x5c);
    }
    iVar33 = *(int *)(iVar2 + 0x18);
    if ((*(unsigned int *)(iVar33 + 8) & 0x1000000) == 0) {
      param_1->field_0x18 =
           CONCAT22((unsigned short)(((unsigned long long)*(unsigned char *)(iVar33 + 0x18) << 0x18) >> 0x10) |
                    ((unsigned short)*(unsigned int *)(iVar33 + 8) & 0x9f),*(unsigned short *)(param_1->field_0xc + 6));
      param_1->field_0x14 = param_1->field_0x14 & 0xfffffffe;
      iVar31 = *(int *)(param_1->field_0x10 + 4);
      if (iVar31 != 0) {
        uVar32 = *(unsigned int *)(iVar31 + 0x28);
        uVar5 = param_1->field_0x18;
        if (uVar5 != uVar32) {
          if ((((uVar5 >> 0x10) & 0xff) == ((uVar32 >> 0x10) & 0xff)) &&
             (uVar5 >> 0x18 == uVar32 >> 0x18)) {
            param_1->field_0x14 = param_1->field_0x14 | 4;
          }
          else {
            param_1->field_0x14 = param_1->field_0x14 | 8;
          }
        }
        if (*(unsigned short *)(iVar33 + 0xc) != 0) {
          if ((*(unsigned short *)(iVar33 + 0xc) & 1) != 0) {
            param_1->field_0x14 = param_1->field_0x14 | 8;
          }
          else {
            param_1->field_0x14 = param_1->field_0x14 | 2;
            param_1->field_0x14 = param_1->field_0x14 | ((unsigned int)*(unsigned short *)(iVar33 + 0xc) << 3);
          }
        }
        if (((*(unsigned int *)(iVar2 + 0x4c) & 1) != 0) &&
           (((long)*(int *)(iVar31 + 0x40) != (long)*(short *)(iVar2 + 0x52) ||
            ((long)*(int *)(iVar31 + 0x3c) != (long)*(short *)(iVar2 + 0x50))))) {
          param_1->field_0x14 = param_1->field_0x14 | 4;
        }
      }
    }
    *(unsigned short *)(*(int *)(iVar2 + 0x18) + 0xc) = 0;
    *(unsigned int *)(iVar2 + 0x4c) = *(unsigned int *)(iVar2 + 0x4c) & 0xfffffffe;
    iVar33 = DAT_008872e0 + 0x20;
    uVar14 = FUN_003e9700((short)*(unsigned int *)(iVar2 + 4));
    FUN_003e05f0((short)*param_2,uVar14,iVar33);
    iVar31 = 2;
    pfVar15 = (float *)FUN_003bfae0((short)iVar2);
    iVar33 = 5;
    pfVar18 = (float *)(DAT_008872e0 + DAT_00764a24 + 0x60);
    do {
      if (-pfVar15[3] <
          (pfVar15[2] * pfVar18[2] + *pfVar15 * *pfVar18 + pfVar15[1] * pfVar18[1]) - pfVar18[3]) {
        iVar31 = 1;
        break;
      }
      pfVar18 = pfVar18 + 5;
      bVar1 = iVar33 != 0;
      iVar33 = iVar33 - 1;
    } while (bVar1);
    uVar32 = *(unsigned int *)param_1->field_0xc;
    if ((uVar32 & 1) != 1) {
      if (((uVar32 & 0xff) != 0) && ((uVar32 & 2) != 2)) {
        if ((uVar32 & 0x10) != 0x10) {
          if ((uVar32 & 8) == 8) {
            param_1->field_0x29 = 2;
            param_1->field_0x28 = DAT_00764a74 | 0x10;
          }
          else {
            param_1->field_0x29 = 1;
            param_1->field_0x28 = DAT_00764a74 | 0x14;
          }
        }
        else {
          param_1->field_0x29 = 4;
          param_1->field_0x28 = DAT_00764a74;
        }
      }
      else {
        param_1->field_0x29 = 3;
        param_1->field_0x28 = DAT_00764a74 | 4;
      }
    }
    else {
      param_1->field_0x29 = 4;
      param_1->field_0x28 = DAT_00764a74;
    }
    if (iVar31 != 2) {
      param_1->field_0x28 = param_1->field_0x28 | 2;
    }
    param_1->field_0x2a = (*(unsigned int *)(*(int *)(iVar2 + 0x18) + 8) & 0x40) != 0;
    if (DAT_008864b4 != (code *)0x0) {
      (*DAT_008864b4)(1,param_1->field_0x8);
    }
    else {
      iVar33 = param_1->field_0x8;
      iVar31 = *(int *)(iVar33 + 0x18);
      if ((*(unsigned int *)(iVar31 + 8) & 0x20) != 0) {
        DAT_008872ea = DAT_008872ea + 1;
        iVar17 = FUN_003e9700((short)*(unsigned int *)(iVar33 + 4));
        FUN_003e0960((short)&uStack00000000 - 0x60,iVar17);
        if ((*(unsigned int *)(iVar17 + 0xc) & 3) == 3) {
          fStack_20 = 1.0f;
          fStack_1c = 1.0f;
        }
        else {
          fStack_20 = SQRT(fStack_38 * fStack_38 + fStack_40 * fStack_40 + fStack_3c * fStack_3c);
          fStack_1c = fStack_20;
          if (0.0f < fStack_20) {
            fStack_1c = 1.0f / fStack_20;
          }
        }
        iVar31 = *(int *)(iVar31 + 0x54);
        aiStack_70[0] = *(int *)(*(int *)(iVar31 + 0xc) + iVar31 + 0x18) + 0xc;
        FUN_003c96d0(0xffa0,aiStack_70);
        for (puVar6 = *(unsigned int **)(iVar33 + 100); puVar6 != (unsigned int *)(iVar33 + 100);
            puVar6 = (unsigned int *)*puVar6) {
          iVar31 = puVar6[2];
          for (puVar7 = *(unsigned int **)(iVar31 + 0x40); puVar7 != (unsigned int *)(iVar31 + 0x40);
              puVar7 = (unsigned int *)*puVar7) {
            iVar17 = puVar7[2];
            if (((iVar17 != 0) && (*(short *)(iVar17 + 0x3c) != DAT_008872ea)) &&
               ((*(unsigned char *)(iVar17 + 2) & 1) != 0)) {
              *(short *)(iVar17 + 0x3c) = DAT_008872ea;
              iVar17 = FUN_003e9700((short)*(unsigned int *)(puVar7[2] + 4));
              pfVar18 = (float *)FUN_003bfae0((short)iVar33);
              fVar36 = pfVar18[1] - *(float *)(iVar17 + 0x34);
              fVar37 = *pfVar18 - *(float *)(iVar17 + 0x30);
              fVar34 = pfVar18[2] - *(float *)(iVar17 + 0x38);
              fVar35 = pfVar18[3] + *(float *)(puVar7[2] + 0x14);
              if (fVar34 * fVar34 + fVar37 * fVar37 + fVar36 * fVar36 < fVar35 * fVar35) {
                FUN_003cffa0((short)puVar7[2],aiStack_70);
              }
            }
          }
        }
      }
    }
    puVar16 = (unsigned long long *)FUN_0039c360((short)iVar2,puVar3);
    if (puVar3[0xc] != 0) {
      DAT_008864b8 = puVar3 + 0xb;
    }
    else {
      uVar32 = uVar4 * 4;
      FUN_003f3eb0(0,uVar32 + 2);
      auVar24._8_8_ = extraout_v0_udw;
      auVar24._0_8_ = 0x1000101;
      auVar21._2_2_ = ((unsigned short)uVar32 & 0xff) | 0x6c00;
      auVar21._0_2_ = 700;
      auVar21._4_4_ = 0;
      auVar21._8_8_ = in_a0_udw;
      auVar24 = _ppacw(auVar21,auVar24);
      auVar20._0_8_ = (unsigned long long)uVar32 | 0x10000000;
      auVar20._8_8_ = extraout_v0_udw;
      auVar9._4_4_ = in_a1_udw_00;
      auVar9._0_4_ = in_a1_udw;
      auVar9._8_8_ = 0;
      auVar21 = _ppacw(auVar9 << 0x40,auVar20);
      auVar21 = _ppacw(auVar24,auVar21);
      puVar6 = DAT_00764974 + 4;
      uVar32 = 0;
      *DAT_00764974 = auVar21._0_4_;
      DAT_00764974[1] = auVar21._4_4_;
      DAT_00764974[2] = auVar21._8_4_;
      DAT_00764974[3] = auVar21._12_4_;
      if (uVar4 != 0) {
        if (8 < uVar4) {
          do {
            uVar27 = *(unsigned int *)(puVar16 + 2);
            uVar28 = *(unsigned int *)((int)puVar16 + 0x14);
            uVar29 = *(unsigned int *)(puVar16 + 3);
            uVar30 = *(unsigned int *)((int)puVar16 + 0x1c);
            uVar32 = uVar32 + 8;
            uVar19 = puVar16[4];
            uVar25 = *(unsigned int *)(puVar16 + 5);
            uVar26 = *(unsigned int *)((int)puVar16 + 0x2c);
            uVar14 = *(unsigned int *)(puVar16 + 6);
            uVar10 = *(unsigned int *)((int)puVar16 + 0x34);
            uVar11 = *(unsigned int *)(puVar16 + 7);
            uVar12 = *(unsigned int *)((int)puVar16 + 0x3c);
            uVar8 = *puVar16;
            uVar22 = *(unsigned int *)(puVar16 + 1);
            uVar23 = *(unsigned int *)((int)puVar16 + 0xc);
            *puVar6 = (int)uVar8;
            puVar6[1] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[2] = uVar22;
            puVar6[3] = uVar23;
            puVar6[4] = uVar27;
            puVar6[5] = uVar28;
            puVar6[6] = uVar29;
            puVar6[7] = uVar30;
            puVar6[8] = (int)uVar19;
            puVar6[9] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[10] = uVar25;
            puVar6[0xb] = uVar26;
            puVar6[0xc] = uVar14;
            puVar6[0xd] = uVar10;
            puVar6[0xe] = uVar11;
            puVar6[0xf] = uVar12;
            uVar27 = *(unsigned int *)(puVar16 + 10);
            uVar28 = *(unsigned int *)((int)puVar16 + 0x54);
            uVar29 = *(unsigned int *)(puVar16 + 0xb);
            uVar30 = *(unsigned int *)((int)puVar16 + 0x5c);
            uVar19 = puVar16[0xc];
            uVar25 = *(unsigned int *)(puVar16 + 0xd);
            uVar26 = *(unsigned int *)((int)puVar16 + 0x6c);
            uVar14 = *(unsigned int *)(puVar16 + 0xe);
            uVar10 = *(unsigned int *)((int)puVar16 + 0x74);
            uVar11 = *(unsigned int *)(puVar16 + 0xf);
            uVar12 = *(unsigned int *)((int)puVar16 + 0x7c);
            uVar8 = puVar16[8];
            uVar22 = *(unsigned int *)(puVar16 + 9);
            uVar23 = *(unsigned int *)((int)puVar16 + 0x4c);
            puVar6[0x10] = (int)uVar8;
            puVar6[0x11] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[0x12] = uVar22;
            puVar6[0x13] = uVar23;
            puVar6[0x14] = uVar27;
            puVar6[0x15] = uVar28;
            puVar6[0x16] = uVar29;
            puVar6[0x17] = uVar30;
            puVar6[0x18] = (int)uVar19;
            puVar6[0x19] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[0x1a] = uVar25;
            puVar6[0x1b] = uVar26;
            puVar6[0x1c] = uVar14;
            puVar6[0x1d] = uVar10;
            puVar6[0x1e] = uVar11;
            puVar6[0x1f] = uVar12;
            uVar27 = *(unsigned int *)(puVar16 + 0x12);
            uVar28 = *(unsigned int *)((int)puVar16 + 0x94);
            uVar29 = *(unsigned int *)(puVar16 + 0x13);
            uVar30 = *(unsigned int *)((int)puVar16 + 0x9c);
            uVar19 = puVar16[0x14];
            uVar25 = *(unsigned int *)(puVar16 + 0x15);
            uVar26 = *(unsigned int *)((int)puVar16 + 0xac);
            uVar14 = *(unsigned int *)(puVar16 + 0x16);
            uVar10 = *(unsigned int *)((int)puVar16 + 0xb4);
            uVar11 = *(unsigned int *)(puVar16 + 0x17);
            uVar12 = *(unsigned int *)((int)puVar16 + 0xbc);
            uVar8 = puVar16[0x10];
            uVar22 = *(unsigned int *)(puVar16 + 0x11);
            uVar23 = *(unsigned int *)((int)puVar16 + 0x8c);
            puVar6[0x20] = (int)uVar8;
            puVar6[0x21] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[0x22] = uVar22;
            puVar6[0x23] = uVar23;
            puVar6[0x24] = uVar27;
            puVar6[0x25] = uVar28;
            puVar6[0x26] = uVar29;
            puVar6[0x27] = uVar30;
            puVar6[0x28] = (int)uVar19;
            puVar6[0x29] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[0x2a] = uVar25;
            puVar6[0x2b] = uVar26;
            puVar6[0x2c] = uVar14;
            puVar6[0x2d] = uVar10;
            puVar6[0x2e] = uVar11;
            puVar6[0x2f] = uVar12;
            uVar27 = *(unsigned int *)(puVar16 + 0x1a);
            uVar28 = *(unsigned int *)((int)puVar16 + 0xd4);
            uVar29 = *(unsigned int *)(puVar16 + 0x1b);
            uVar30 = *(unsigned int *)((int)puVar16 + 0xdc);
            uVar19 = puVar16[0x1c];
            uVar25 = *(unsigned int *)(puVar16 + 0x1d);
            uVar26 = *(unsigned int *)((int)puVar16 + 0xec);
            uVar14 = *(unsigned int *)(puVar16 + 0x1e);
            uVar10 = *(unsigned int *)((int)puVar16 + 0xf4);
            uVar11 = *(unsigned int *)(puVar16 + 0x1f);
            uVar12 = *(unsigned int *)((int)puVar16 + 0xfc);
            uVar8 = puVar16[0x18];
            uVar22 = *(unsigned int *)(puVar16 + 0x19);
            uVar23 = *(unsigned int *)((int)puVar16 + 0xcc);
            puVar6[0x30] = (int)uVar8;
            puVar6[0x31] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[0x32] = uVar22;
            puVar6[0x33] = uVar23;
            puVar6[0x34] = uVar27;
            puVar6[0x35] = uVar28;
            puVar6[0x36] = uVar29;
            puVar6[0x37] = uVar30;
            puVar6[0x38] = (int)uVar19;
            puVar6[0x39] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[0x3a] = uVar25;
            puVar6[0x3b] = uVar26;
            puVar6[0x3c] = uVar14;
            puVar6[0x3d] = uVar10;
            puVar6[0x3e] = uVar11;
            puVar6[0x3f] = uVar12;
            uVar27 = *(unsigned int *)(puVar16 + 0x22);
            uVar28 = *(unsigned int *)((int)puVar16 + 0x114);
            uVar29 = *(unsigned int *)(puVar16 + 0x23);
            uVar30 = *(unsigned int *)((int)puVar16 + 0x11c);
            uVar19 = puVar16[0x24];
            uVar25 = *(unsigned int *)(puVar16 + 0x25);
            uVar26 = *(unsigned int *)((int)puVar16 + 300);
            uVar14 = *(unsigned int *)(puVar16 + 0x26);
            uVar10 = *(unsigned int *)((int)puVar16 + 0x134);
            uVar11 = *(unsigned int *)(puVar16 + 0x27);
            uVar12 = *(unsigned int *)((int)puVar16 + 0x13c);
            uVar8 = puVar16[0x20];
            uVar22 = *(unsigned int *)(puVar16 + 0x21);
            uVar23 = *(unsigned int *)((int)puVar16 + 0x10c);
            puVar6[0x40] = (int)uVar8;
            puVar6[0x41] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[0x42] = uVar22;
            puVar6[0x43] = uVar23;
            puVar6[0x44] = uVar27;
            puVar6[0x45] = uVar28;
            puVar6[0x46] = uVar29;
            puVar6[0x47] = uVar30;
            puVar6[0x48] = (int)uVar19;
            puVar6[0x49] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[0x4a] = uVar25;
            puVar6[0x4b] = uVar26;
            puVar6[0x4c] = uVar14;
            puVar6[0x4d] = uVar10;
            puVar6[0x4e] = uVar11;
            puVar6[0x4f] = uVar12;
            uVar27 = *(unsigned int *)(puVar16 + 0x2a);
            uVar28 = *(unsigned int *)((int)puVar16 + 0x154);
            uVar29 = *(unsigned int *)(puVar16 + 0x2b);
            uVar30 = *(unsigned int *)((int)puVar16 + 0x15c);
            uVar19 = puVar16[0x2c];
            uVar25 = *(unsigned int *)(puVar16 + 0x2d);
            uVar26 = *(unsigned int *)((int)puVar16 + 0x16c);
            uVar14 = *(unsigned int *)(puVar16 + 0x2e);
            uVar10 = *(unsigned int *)((int)puVar16 + 0x174);
            uVar11 = *(unsigned int *)(puVar16 + 0x2f);
            uVar12 = *(unsigned int *)((int)puVar16 + 0x17c);
            uVar8 = puVar16[0x28];
            uVar22 = *(unsigned int *)(puVar16 + 0x29);
            uVar23 = *(unsigned int *)((int)puVar16 + 0x14c);
            puVar6[0x50] = (int)uVar8;
            puVar6[0x51] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[0x52] = uVar22;
            puVar6[0x53] = uVar23;
            puVar6[0x54] = uVar27;
            puVar6[0x55] = uVar28;
            puVar6[0x56] = uVar29;
            puVar6[0x57] = uVar30;
            puVar6[0x58] = (int)uVar19;
            puVar6[0x59] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[0x5a] = uVar25;
            puVar6[0x5b] = uVar26;
            puVar6[0x5c] = uVar14;
            puVar6[0x5d] = uVar10;
            puVar6[0x5e] = uVar11;
            puVar6[0x5f] = uVar12;
            uVar27 = *(unsigned int *)(puVar16 + 0x32);
            uVar28 = *(unsigned int *)((int)puVar16 + 0x194);
            uVar29 = *(unsigned int *)(puVar16 + 0x33);
            uVar30 = *(unsigned int *)((int)puVar16 + 0x19c);
            uVar19 = puVar16[0x34];
            uVar25 = *(unsigned int *)(puVar16 + 0x35);
            uVar26 = *(unsigned int *)((int)puVar16 + 0x1ac);
            uVar14 = *(unsigned int *)(puVar16 + 0x36);
            uVar10 = *(unsigned int *)((int)puVar16 + 0x1b4);
            uVar11 = *(unsigned int *)(puVar16 + 0x37);
            uVar12 = *(unsigned int *)((int)puVar16 + 0x1bc);
            uVar8 = puVar16[0x30];
            uVar22 = *(unsigned int *)(puVar16 + 0x31);
            uVar23 = *(unsigned int *)((int)puVar16 + 0x18c);
            puVar6[0x60] = (int)uVar8;
            puVar6[0x61] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[0x62] = uVar22;
            puVar6[99] = uVar23;
            puVar6[100] = uVar27;
            puVar6[0x65] = uVar28;
            puVar6[0x66] = uVar29;
            puVar6[0x67] = uVar30;
            puVar6[0x68] = (int)uVar19;
            puVar6[0x69] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[0x6a] = uVar25;
            puVar6[0x6b] = uVar26;
            puVar6[0x6c] = uVar14;
            puVar6[0x6d] = uVar10;
            puVar6[0x6e] = uVar11;
            puVar6[0x6f] = uVar12;
            uVar27 = *(unsigned int *)(puVar16 + 0x3a);
            uVar28 = *(unsigned int *)((int)puVar16 + 0x1d4);
            uVar29 = *(unsigned int *)(puVar16 + 0x3b);
            uVar30 = *(unsigned int *)((int)puVar16 + 0x1dc);
            uVar19 = puVar16[0x3c];
            uVar25 = *(unsigned int *)(puVar16 + 0x3d);
            uVar26 = *(unsigned int *)((int)puVar16 + 0x1ec);
            uVar14 = *(unsigned int *)(puVar16 + 0x3e);
            uVar10 = *(unsigned int *)((int)puVar16 + 500);
            uVar11 = *(unsigned int *)(puVar16 + 0x3f);
            uVar12 = *(unsigned int *)((int)puVar16 + 0x1fc);
            uVar8 = puVar16[0x38];
            uVar22 = *(unsigned int *)(puVar16 + 0x39);
            uVar23 = *(unsigned int *)((int)puVar16 + 0x1cc);
            puVar6[0x70] = (int)uVar8;
            puVar6[0x71] = (int)((unsigned long long)uVar8 >> 0x20);
            puVar6[0x72] = uVar22;
            puVar6[0x73] = uVar23;
            puVar16 = puVar16 + 0x40;
            puVar6[0x74] = uVar27;
            puVar6[0x75] = uVar28;
            puVar6[0x76] = uVar29;
            puVar6[0x77] = uVar30;
            puVar6[0x78] = (int)uVar19;
            puVar6[0x79] = (int)((unsigned long long)uVar19 >> 0x20);
            puVar6[0x7a] = uVar25;
            puVar6[0x7b] = uVar26;
            puVar6[0x7c] = uVar14;
            puVar6[0x7d] = uVar10;
            puVar6[0x7e] = uVar11;
            puVar6[0x7f] = uVar12;
            puVar6 = puVar6 + 0x80;
          } while (uVar32 < uVar4 - 8);
        }
        for (; uVar32 < uVar4; uVar32 = uVar32 + 1) {
          uVar19 = puVar16[2];
          uVar28 = *(unsigned int *)(puVar16 + 3);
          uVar29 = *(unsigned int *)((int)puVar16 + 0x1c);
          uVar14 = *(unsigned int *)(puVar16 + 4);
          uVar10 = *(unsigned int *)((int)puVar16 + 0x24);
          uVar11 = *(unsigned int *)(puVar16 + 5);
          uVar12 = *(unsigned int *)((int)puVar16 + 0x2c);
          uVar8 = puVar16[6];
          uVar26 = *(unsigned int *)(puVar16 + 7);
          uVar27 = *(unsigned int *)((int)puVar16 + 0x3c);
          uVar22 = *(unsigned int *)((int)puVar16 + 4);
          uVar23 = *(unsigned int *)(puVar16 + 1);
          uVar25 = *(unsigned int *)((int)puVar16 + 0xc);
          *puVar6 = *(unsigned int *)puVar16;
          puVar6[1] = uVar22;
          puVar6[2] = uVar23;
          puVar6[3] = uVar25;
          puVar16 = puVar16 + 8;
          puVar6[4] = (int)uVar19;
          puVar6[5] = (int)((unsigned long long)uVar19 >> 0x20);
          puVar6[6] = uVar28;
          puVar6[7] = uVar29;
          puVar6[8] = uVar14;
          puVar6[9] = uVar10;
          puVar6[10] = uVar11;
          puVar6[0xb] = uVar12;
          puVar6[0xc] = (int)uVar8;
          puVar6[0xd] = (int)((unsigned long long)uVar8 >> 0x20);
          puVar6[0xe] = uVar26;
          puVar6[0xf] = uVar27;
          puVar6 = puVar6 + 0x10;
        }
      }
      DAT_008864b8 = (unsigned int *)0x0;
      DAT_00764974 = puVar6;
    }
    uVar19 = 1;
  }
  else {
    uVar19 = 0;
  }
  return uVar19;
}
