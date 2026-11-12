/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003adc40 rpPTankSkyInsPosCCCSPR src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003adc40_arg4 fn_003adc40_arg4;
struct fn_003adc40_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x28];
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
    unsigned char pad_0x3c[0x2c];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
    float field_0x70; /* +0x70 */
    float field_0x74; /* +0x74 */
};


/* WARNING: Removing unreachable block (ram,0x003ae550) */
/* WARNING: Removing unreachable block (ram,0x003ae58c) */
/* WARNING: Removing unreachable block (ram,0x003ae5ac) */
/* WARNING: Removing unreachable block (ram,0x003ae5b4) */
/* WARNING: Removing unreachable block (ram,0x003ae5b8) */
/* WARNING: Removing unreachable block (ram,0x003ae128) */
/* WARNING: Removing unreachable block (ram,0x003ae154) */
/* WARNING: Removing unreachable block (ram,0x003ae174) */
/* WARNING: Removing unreachable block (ram,0x003ae180) */
/* WARNING: Removing unreachable block (ram,0x003ae1a0) */
/* WARNING: Removing unreachable block (ram,0x003ae1b0) */
/* WARNING: Removing unreachable block (ram,0x003ae1d0) */
/* WARNING: Removing unreachable block (ram,0x003ae1ec) */
/* WARNING: Removing unreachable block (ram,0x003ae21c) */
/* WARNING: Removing unreachable block (ram,0x003ae220) */
/* WARNING: Removing unreachable block (ram,0x003ae324) */
/* WARNING: Removing unreachable block (ram,0x003ae334) */
/* WARNING: Removing unreachable block (ram,0x003ae354) */
/* WARNING: Removing unreachable block (ram,0x003ae360) */
/* WARNING: Removing unreachable block (ram,0x003ae370) */
/* WARNING: Removing unreachable block (ram,0x003ae394) */
/* WARNING: Removing unreachable block (ram,0x003ae3a0) */
/* WARNING: Removing unreachable block (ram,0x003ae3c0) */
/* WARNING: Removing unreachable block (ram,0x003ae5d8) */
/* WARNING: Removing unreachable block (ram,0x003ae5f0) */
/* WARNING: Removing unreachable block (ram,0x003ae5f4) */
/* WARNING: Removing unreachable block (ram,0x003ae618) */
/* WARNING: Removing unreachable block (ram,0x003ae634) */
/* WARNING: Removing unreachable block (ram,0x003ae664) */
/* WARNING: Removing unreachable block (ram,0x003ae668) */
/* WARNING: Removing unreachable block (ram,0x003ae770) */
/* WARNING: Removing unreachable block (ram,0x003ae784) */
/* WARNING: Removing unreachable block (ram,0x003ae7a8) */
/* WARNING: Removing unreachable block (ram,0x003ae7b0) */
/* WARNING: Removing unreachable block (ram,0x003ae7c4) */
/* WARNING: Removing unreachable block (ram,0x003ae7e8) */
/* WARNING: Removing unreachable block (ram,0x003ae7f0) */
/* WARNING: Removing unreachable block (ram,0x003ae7f4) */
/* WARNING: Removing unreachable block (ram,0x003ae810) */
/* WARNING: Removing unreachable block (ram,0x003ae854) */
/* WARNING: Removing unreachable block (ram,0x003ae85c) */

void FUN_003adc40(int *param_1,unsigned long long param_2,unsigned long long param_3,int param_4,
                 fn_003adc40_arg4 *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  char cVar4;
  int iVar5;
  float *pfVar6;
  float *pfVar7;
  float *pfVar8;
  float *pfVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  float fVar23;
  float fVar24;
  float fVar25;
  float fVar26;
  float fVar27;
  float fVar28;
  float fVar29;
  float fVar30;
  float fVar31;
  float fVar32;
  float fVar33;
  float fVar34;
  float fVar35;
  float fVar36;
  float fVar37;
  float fVar38;
  float fStack_54;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  float fStack_10;
  float fStack_c;
  
  iVar1 = param_1[1];
  iVar2 = param_5->field_0x8;
  pfVar8 = (float *)param_5->field_0x4;
  pfVar7 = (float *)param_5->field_0x34;
  iVar3 = param_5->field_0x38;
  pfVar6 = (float *)*param_1;
  fVar14 = param_5->field_0x70 * 0.5f;
  fVar15 = param_5->field_0x74 * 0.5f;
  if (iVar2 == iVar1) {
    FUN_00421a60(0);
    if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x100;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar4 = getCopCondition(0,0);
      } while (cVar4 == '\0');
    }
    if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x200;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar4 = getCopCondition(0,0);
      } while (cVar4 == '\0');
      do {
      } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
    }
    fVar12 = param_5->field_0x68 * (1.0f / fVar14) - 1.0f;
    fVar10 = param_5->field_0x6c * (1.0f / fVar15) - 1.0f;
    fVar11 = param_5->field_0x68 * (1.0f / fVar14) + 1.0f;
    fVar13 = param_5->field_0x6c * (1.0f / fVar15) + 1.0f;
    pfVar9 = (float *)param_2;
    fRam70003f00 = *pfVar9;
    fRam70003f04 = pfVar9[1];
    fRam70003f08 = pfVar9[2];
    pfVar9 = (float *)param_3;
    fRam70003f10 = *pfVar9;
    fRam70003f14 = pfVar9[1];
    fRam70003f18 = pfVar9[2];
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    fVar22 = fRam70003f00 * fVar14;
    fVar23 = fRam70003f04 * fVar14;
    fVar24 = fRam70003f08 * fVar14;
    fVar28 = fRam70003f10 * fVar14;
    fVar29 = fRam70003f14 * fVar14;
    fVar30 = fRam70003f18 * fVar14;
    fVar25 = fRam70003f00 * fVar15;
    fVar26 = fRam70003f04 * fVar15;
    fVar27 = fRam70003f08 * fVar15;
    fVar31 = fRam70003f10 * fVar15;
    fVar32 = fRam70003f14 * fVar15;
    fVar33 = fRam70003f18 * fVar15;
    EI();
    if (0xbf < param_4) {
      if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar4 = getCopCondition(0,0);
        } while (cVar4 == '\0');
      }
      if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar4 = getCopCondition(0,0);
        } while (cVar4 == '\0');
        do {
        } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
      }
      SYNC(0);
      SYNC(0x10);
      SYNC(0);
      SYNC(0x10);
      do {
        cVar4 = getCopCondition(0,0);
      } while (cVar4 == '\0');
      do {
      } while( 1 );
    }
    if (3 < param_4) {
      if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar4 = getCopCondition(0,0);
        } while (cVar4 == '\0');
      }
      if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar4 = getCopCondition(0,0);
        } while (cVar4 == '\0');
        do {
        } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
      }
      SYNC(0);
      SYNC(0x10);
      SYNC(0);
      SYNC(0x10);
      do {
        cVar4 = getCopCondition(0,0);
      } while (cVar4 == '\0');
      do {
      } while( 1 );
    }
    iVar5 = 0;
    fRam70003f30 = fVar14;
    fRam70003f34 = fVar14;
    fRam70003f38 = fVar14;
    fRam70003f3c = fVar14;
    fRam70003f40 = fVar15;
    fRam70003f44 = fVar15;
    fRam70003f48 = fVar15;
    fRam70003f4c = fVar15;
    fRam70003f70 = fVar12;
    fRam70003f74 = fVar10;
    fRam70003f78 = fVar11;
    fRam70003f7c = fVar13;
    if (0 < param_4) {
      do {
        fRam70003f20 = *pfVar7;
        fRam70003f28 = *pfVar7;
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar35 = *pfVar8;
        fVar36 = pfVar8[1];
        fVar37 = pfVar8[2];
        fVar38 = pfVar8[3];
        fVar21 = 1.5707964f - ABS(fRam70003f20) * 1.0f;
        fVar15 = 3.1415927f - fRam70003f28 * 1.0f;
        fVar16 = 3.1415927f - fRam70003f2c * 1.0f;
        fVar17 = -3.1415927f - fRam70003f28 * 1.0f;
        fVar19 = -3.1415925f - fRam70003f2c * 1.0f;
        fVar14 = fRam70003f28;
        if (fVar15 < fRam70003f28) {
          fVar14 = fVar15;
        }
        fVar15 = fRam70003f2c;
        if (fVar16 < fRam70003f2c) {
          fVar15 = fVar16;
        }
        if (fVar14 < fVar17) {
          fVar14 = fVar17;
        }
        if (fVar15 < fVar19) {
          fVar15 = fVar19;
        }
        fVar16 = fVar21 * fVar21;
        fVar17 = fVar14 * fVar14;
        fVar19 = fVar16 * fVar16;
        fVar18 = fVar17 * fVar17;
        fVar20 = fVar15 * fVar15 * fVar15 * fVar15;
        fVar16 = fVar21 * -0.16666657f * fVar16 + fVar21 * -0.00019807414f * fVar16 * fVar19 +
                 fVar21 * 0.0083330255f * fVar19 + fVar21 * 1.0f +
                 fVar21 * 2.601887e-06f * fVar19 * fVar19;
        fVar14 = fVar14 * -0.16666657f * fVar17 + fVar14 * -0.00019807414f * fVar17 * fVar18 +
                 fVar14 * 0.0083330255f * fVar18 + fVar14 * 1.0f +
                 fVar14 * 2.601887e-06f * fVar18 * fVar18;
        fVar17 = fVar22 * fVar16 - fVar28 * fVar14;
        fVar19 = fVar23 * fVar16 - fVar29 * fVar14;
        fVar21 = fVar24 * fVar16 - fVar30 * fVar14;
        fVar20 = fVar20 * fVar20 - fVar20;
        fVar18 = fVar25 * fVar14 + fVar31 * fVar16;
        fVar34 = fVar26 * fVar14 + fVar32 * fVar16;
        fVar14 = fVar27 * fVar14 + fVar33 * fVar16;
        fVar15 = fVar15 * 2.601887e-06f + fVar15 * 0.0083330255f;
        *pfVar6 = fVar18 * fVar10 + fVar17 * fVar12 + fVar35 * 1.0f;
        pfVar6[1] = fVar34 * fVar10 + fVar19 * fVar12 + fVar36 * 1.0f;
        pfVar6[2] = fVar14 * fVar10 + fVar21 * fVar12 + fVar37 * 1.0f;
        pfVar6[3] = fVar15 * fVar10 + fVar20 * fVar12 + fVar38 * 1.0f;
        pfVar6[4] = fVar18 * fVar10 + fVar17 * fVar11 + fVar35 * 1.0f;
        pfVar6[5] = fVar34 * fVar10 + fVar19 * fVar11 + fVar36 * 1.0f;
        pfVar6[6] = fVar14 * fVar10 + fVar21 * fVar11 + fVar37 * 1.0f;
        pfVar6[7] = fVar15 * fVar10 + fVar20 * fVar11 + fVar38 * 1.0f;
        pfVar6[8] = fVar18 * fVar13 + fVar17 * fVar11 + fVar35 * 1.0f;
        pfVar6[9] = fVar34 * fVar13 + fVar19 * fVar11 + fVar36 * 1.0f;
        pfVar6[10] = fVar14 * fVar13 + fVar21 * fVar11 + fVar37 * 1.0f;
        pfVar6[0xb] = fVar15 * fVar13 + fVar20 * fVar11 + fVar38 * 1.0f;
        pfVar6[0xc] = fVar18 * fVar13 + fVar17 * fVar12 + fVar35 * 1.0f;
        pfVar6[0xd] = fVar34 * fVar13 + fVar19 * fVar12 + fVar36 * 1.0f;
        pfVar6[0xe] = fVar14 * fVar13 + fVar21 * fVar12 + fVar37 * 1.0f;
        pfVar6[0xf] = fVar15 * fVar13 + fVar20 * fVar12 + fVar38 * 1.0f;
        EI();
        iVar5 = iVar5 + 1;
        pfVar8 = (float *)((int)pfVar8 + iVar2);
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar6 = pfVar6 + iVar1;
      } while (iVar5 < param_4);
    }
  }
  else {
    fVar11 = 1.0f / fVar14;
    fStack_10 = param_5->field_0x68 * fVar11;
    fVar13 = 1.0f / fVar15;
    fStack_c = param_5->field_0x6c * fVar13;
    FUN_0043f810(&fStack_20,param_2,0xc);
    FUN_0043f810(&fStack_30,param_3,0xc);
    fVar10 = param_5->field_0x68 * fVar11 + 1.0f;
    fVar11 = param_5->field_0x68 * fVar11 - 1.0f;
    fVar12 = param_5->field_0x6c * fVar13 - 1.0f;
    fVar13 = param_5->field_0x6c * fVar13 + 1.0f;
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    iVar5 = 0;
    if (0 < param_4) {
      do {
        fVar22 = *pfVar7;
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar28 = *pfVar8;
        fVar29 = pfVar8[1];
        fVar30 = pfVar8[2];
        fVar31 = pfVar8[3];
        fVar27 = 1.5707964f - ABS(fVar22) * 1.0f;
        fVar23 = 3.1415927f - fVar22 * 1.0f;
        fVar24 = 3.1415927f - fStack_54 * 1.0f;
        fVar25 = -3.1415927f - fVar22 * 1.0f;
        fVar26 = -3.1415925f - fStack_54 * 1.0f;
        if (fVar23 < fVar22) {
          fVar22 = fVar23;
        }
        fVar23 = fStack_54;
        if (fVar24 < fStack_54) {
          fVar23 = fVar24;
        }
        if (fVar22 < fVar25) {
          fVar22 = fVar25;
        }
        if (fVar23 < fVar26) {
          fVar23 = fVar26;
        }
        fVar24 = fVar27 * fVar27;
        fVar25 = fVar22 * fVar22;
        fVar26 = fVar24 * fVar24;
        fVar32 = fVar25 * fVar25;
        fVar33 = fVar23 * fVar23 * fVar23 * fVar23;
        fVar24 = fVar27 * -0.16666657f * fVar24 + fVar27 * -0.00019807414f * fVar24 * fVar26 +
                 fVar27 * 0.0083330255f * fVar26 + fVar27 * 1.0f +
                 fVar27 * 2.601887e-06f * fVar26 * fVar26;
        fVar22 = fVar22 * -0.16666657f * fVar25 + fVar22 * -0.00019807414f * fVar25 * fVar32 +
                 fVar22 * 0.0083330255f * fVar32 + fVar22 * 1.0f +
                 fVar22 * 2.601887e-06f * fVar32 * fVar32;
        fVar25 = fStack_20 * fVar14 * fVar24 - fStack_30 * fVar14 * fVar22;
        fVar26 = fStack_1c * fVar14 * fVar24 - fStack_2c * fVar14 * fVar22;
        fVar27 = fStack_18 * fVar14 * fVar24 - fStack_28 * fVar14 * fVar22;
        fVar33 = fVar33 * fVar33 - fVar33;
        fVar32 = fStack_20 * fVar15 * fVar22 + fStack_30 * fVar15 * fVar24;
        fVar16 = fStack_1c * fVar15 * fVar22 + fStack_2c * fVar15 * fVar24;
        fVar22 = fStack_18 * fVar15 * fVar22 + fStack_28 * fVar15 * fVar24;
        fVar23 = fVar23 * 2.601887e-06f + fVar23 * 0.0083330255f;
        *pfVar6 = fVar32 * fVar12 + fVar25 * fVar11 + fVar28 * 1.0f;
        pfVar6[1] = fVar16 * fVar12 + fVar26 * fVar11 + fVar29 * 1.0f;
        pfVar6[2] = fVar22 * fVar12 + fVar27 * fVar11 + fVar30 * 1.0f;
        pfVar6[3] = fVar23 * fVar12 + fVar33 * fVar11 + fVar31 * 1.0f;
        pfVar6[4] = fVar32 * fVar12 + fVar25 * fVar10 + fVar28 * 1.0f;
        pfVar6[5] = fVar16 * fVar12 + fVar26 * fVar10 + fVar29 * 1.0f;
        pfVar6[6] = fVar22 * fVar12 + fVar27 * fVar10 + fVar30 * 1.0f;
        pfVar6[7] = fVar23 * fVar12 + fVar33 * fVar10 + fVar31 * 1.0f;
        pfVar6[8] = fVar32 * fVar13 + fVar25 * fVar10 + fVar28 * 1.0f;
        pfVar6[9] = fVar16 * fVar13 + fVar26 * fVar10 + fVar29 * 1.0f;
        pfVar6[10] = fVar22 * fVar13 + fVar27 * fVar10 + fVar30 * 1.0f;
        pfVar6[0xb] = fVar23 * fVar13 + fVar33 * fVar10 + fVar31 * 1.0f;
        pfVar6[0xc] = fVar32 * fVar13 + fVar25 * fVar11 + fVar28 * 1.0f;
        pfVar6[0xd] = fVar16 * fVar13 + fVar26 * fVar11 + fVar29 * 1.0f;
        pfVar6[0xe] = fVar22 * fVar13 + fVar27 * fVar11 + fVar30 * 1.0f;
        pfVar6[0xf] = fVar23 * fVar13 + fVar33 * fVar11 + fVar31 * 1.0f;
        EI();
        iVar5 = iVar5 + 1;
        pfVar8 = (float *)((int)pfVar8 + iVar2);
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar6 = pfVar6 + iVar1;
      } while (iVar5 < param_4);
    }
  }
  return;
}
