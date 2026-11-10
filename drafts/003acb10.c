/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003acb10 rpPTankSkyInsPosNCPSPR src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003acb10_arg4 fn_003acb10_arg4;
struct fn_003acb10_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x10];
    int field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
    unsigned char pad_0x24[0x10];
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
};


/* WARNING: Removing unreachable block (ram,0x003ad4a0) */
/* WARNING: Removing unreachable block (ram,0x003ad4dc) */
/* WARNING: Removing unreachable block (ram,0x003ad4fc) */
/* WARNING: Removing unreachable block (ram,0x003ad504) */
/* WARNING: Removing unreachable block (ram,0x003ad508) */
/* WARNING: Removing unreachable block (ram,0x003ad400) */
/* WARNING: Removing unreachable block (ram,0x003ad420) */
/* WARNING: Removing unreachable block (ram,0x003ad428) */
/* WARNING: Removing unreachable block (ram,0x003ad450) */
/* WARNING: Removing unreachable block (ram,0x003ad474) */
/* WARNING: Removing unreachable block (ram,0x003ad47c) */
/* WARNING: Removing unreachable block (ram,0x003ad480) */
/* WARNING: Removing unreachable block (ram,0x003acf58) */
/* WARNING: Removing unreachable block (ram,0x003acf84) */
/* WARNING: Removing unreachable block (ram,0x003acfa4) */
/* WARNING: Removing unreachable block (ram,0x003acfb0) */
/* WARNING: Removing unreachable block (ram,0x003acfd0) */
/* WARNING: Removing unreachable block (ram,0x003ad000) */
/* WARNING: Removing unreachable block (ram,0x003ad024) */
/* WARNING: Removing unreachable block (ram,0x003ad030) */
/* WARNING: Removing unreachable block (ram,0x003ad050) */
/* WARNING: Removing unreachable block (ram,0x003ad068) */
/* WARNING: Removing unreachable block (ram,0x003ad088) */
/* WARNING: Removing unreachable block (ram,0x003ad0a8) */
/* WARNING: Removing unreachable block (ram,0x003ad0d0) */
/* WARNING: Removing unreachable block (ram,0x003ad0d4) */
/* WARNING: Removing unreachable block (ram,0x003ad1c8) */
/* WARNING: Removing unreachable block (ram,0x003ad1d8) */
/* WARNING: Removing unreachable block (ram,0x003ad1fc) */
/* WARNING: Removing unreachable block (ram,0x003ad208) */
/* WARNING: Removing unreachable block (ram,0x003ad218) */
/* WARNING: Removing unreachable block (ram,0x003ad23c) */
/* WARNING: Removing unreachable block (ram,0x003ad248) */
/* WARNING: Removing unreachable block (ram,0x003ad268) */
/* WARNING: Removing unreachable block (ram,0x003ad528) */
/* WARNING: Removing unreachable block (ram,0x003ad544) */
/* WARNING: Removing unreachable block (ram,0x003ad548) */
/* WARNING: Removing unreachable block (ram,0x003ad568) */
/* WARNING: Removing unreachable block (ram,0x003ad584) */
/* WARNING: Removing unreachable block (ram,0x003ad5ac) */
/* WARNING: Removing unreachable block (ram,0x003ad5b0) */
/* WARNING: Removing unreachable block (ram,0x003ad6a8) */
/* WARNING: Removing unreachable block (ram,0x003ad6bc) */
/* WARNING: Removing unreachable block (ram,0x003ad6e0) */
/* WARNING: Removing unreachable block (ram,0x003ad6e8) */
/* WARNING: Removing unreachable block (ram,0x003ad6fc) */
/* WARNING: Removing unreachable block (ram,0x003ad720) */
/* WARNING: Removing unreachable block (ram,0x003ad728) */
/* WARNING: Removing unreachable block (ram,0x003ad72c) */
/* WARNING: Removing unreachable block (ram,0x003ad748) */
/* WARNING: Removing unreachable block (ram,0x003ad78c) */
/* WARNING: Removing unreachable block (ram,0x003ad794) */
/* WARNING: Removing unreachable block (ram,0x003ad7a4) */
/* WARNING: Removing unreachable block (ram,0x003ad7ac) */

void FUN_003acb10(int *param_1,float *param_2,float *param_3,int param_4,fn_003acb10_arg4 *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  char cVar5;
  int iVar6;
  float *pfVar7;
  float *pfVar8;
  float *pfVar9;
  float *pfVar10;
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
  float fStack_14;
  float fStack_4;
  
  iVar1 = param_5->field_0x8;
  pfVar8 = (float *)*param_1;
  pfVar10 = (float *)param_5->field_0x4;
  pfVar9 = (float *)param_5->field_0x34;
  pfVar7 = (float *)param_5->field_0x1c;
  iVar2 = param_5->field_0x38;
  iVar3 = param_5->field_0x20;
  iVar4 = param_1[1];
  if (iVar1 == iVar4) {
    FUN_00421a60(0);
    fVar18 = fRam70003f5c;
    fVar11 = fRam70003f4c;
    if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x100;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar5 = getCopCondition(0,0);
      } while (cVar5 == '\0');
    }
    if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x200;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar5 = getCopCondition(0,0);
      } while (cVar5 == '\0');
      do {
      } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
    }
    fVar12 = *param_2 * 0.5f;
    fVar13 = param_2[1] * 0.5f;
    fVar14 = param_2[2] * 0.5f;
    fVar15 = *param_3 * 0.5f;
    fVar16 = param_3[1] * 0.5f;
    fVar17 = param_3[2] * 0.5f;
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
    if (0xaf < param_4) {
      if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar5 = getCopCondition(0,0);
        } while (cVar5 == '\0');
      }
      if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar5 = getCopCondition(0,0);
        } while (cVar5 == '\0');
        do {
        } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
      }
      SYNC(0);
      SYNC(0x10);
      SYNC(0);
      SYNC(0x10);
      do {
        cVar5 = getCopCondition(0,0);
      } while (cVar5 == '\0');
      do {
      } while( 1 );
    }
    if (3 < param_4) {
      if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar5 = getCopCondition(0,0);
        } while (cVar5 == '\0');
      }
      if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar5 = getCopCondition(0,0);
        } while (cVar5 == '\0');
        do {
        } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
      }
      SYNC(0);
      SYNC(0x10);
      SYNC(0);
      SYNC(0x10);
      do {
        cVar5 = getCopCondition(0,0);
      } while (cVar5 == '\0');
      do {
      } while( 1 );
    }
    iVar6 = 0;
    fRam70003f40 = fVar12;
    fRam70003f44 = fVar13;
    fRam70003f48 = fVar14;
    fRam70003f50 = fVar15;
    fRam70003f54 = fVar16;
    fRam70003f58 = fVar17;
    if (0 < param_4) {
      do {
        fRam70003f60 = *pfVar9;
        fRam70003f64 = *pfVar7;
        fRam70003f68 = *pfVar9;
        fRam70003f6c = pfVar7[1];
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar23 = 1.5707964f - ABS(fRam70003f60) * 1.0f;
        fVar19 = 3.1415927f - fRam70003f68 * 1.0f;
        fVar21 = -3.1415927f - fRam70003f68 * 1.0f;
        fVar24 = fRam70003f68;
        if (fVar19 < fRam70003f68) {
          fVar24 = fVar19;
        }
        if (fVar24 < fVar21) {
          fVar24 = fVar21;
        }
        fVar19 = fVar23 * fVar23;
        fVar21 = fVar24 * fVar24;
        fVar20 = fVar19 * fVar19;
        fVar22 = fVar21 * fVar21;
        fVar19 = fVar23 * -0.16666657f * fVar19 + fVar23 * -0.00019807414f * fVar19 * fVar20 +
                 fVar23 * 0.0083330255f * fVar20 + fVar23 * 1.0f +
                 fVar23 * 2.601887e-06f * fVar20 * fVar20;
        fVar24 = fVar24 * -0.16666657f * fVar21 + fVar24 * -0.00019807414f * fVar21 * fVar22 +
                 fVar24 * 0.0083330255f * fVar22 + fVar24 * 1.0f +
                 fVar24 * 2.601887e-06f * fVar22 * fVar22;
        fVar28 = *pfVar10;
        fVar29 = pfVar10[1];
        fVar30 = pfVar10[2];
        fVar31 = pfVar10[3];
        fVar21 = (fVar12 * fVar19 - fVar15 * fVar24) * fRam70003f64;
        fVar23 = (fVar13 * fVar19 - fVar16 * fVar24) * fRam70003f64;
        fVar20 = (fVar14 * fVar19 - fVar17 * fVar24) * fRam70003f64;
        fVar22 = (fVar11 * fVar19 - fVar18 * fVar24) * fRam70003f64;
        fVar25 = (fVar15 * fVar19 + fVar12 * fVar24) * fRam70003f6c;
        fVar26 = (fVar16 * fVar19 + fVar13 * fVar24) * fRam70003f6c;
        fVar27 = (fVar17 * fVar19 + fVar14 * fVar24) * fRam70003f6c;
        fVar24 = (fVar18 * fVar19 + fVar11 * fVar24) * fRam70003f6c;
        *pfVar8 = (fVar28 - fVar21) - fVar25;
        pfVar8[1] = (fVar29 - fVar23) - fVar26;
        pfVar8[2] = (fVar30 - fVar20) - fVar27;
        pfVar8[3] = (fVar31 - fVar22) - fVar24;
        pfVar8[4] = (fVar28 + fVar21) - fVar25;
        pfVar8[5] = (fVar29 + fVar23) - fVar26;
        pfVar8[6] = (fVar30 + fVar20) - fVar27;
        pfVar8[7] = (fVar31 + fVar22) - fVar24;
        pfVar8[8] = fVar28 + fVar21 + fVar25;
        pfVar8[9] = fVar29 + fVar23 + fVar26;
        pfVar8[10] = fVar30 + fVar20 + fVar27;
        pfVar8[0xb] = fVar31 + fVar22 + fVar24;
        pfVar8[0xc] = (fVar28 - fVar21) + fVar25;
        pfVar8[0xd] = (fVar29 - fVar23) + fVar26;
        pfVar8[0xe] = (fVar30 - fVar20) + fVar27;
        pfVar8[0xf] = (fVar31 - fVar22) + fVar24;
        EI();
        iVar6 = iVar6 + 1;
        pfVar10 = (float *)((int)pfVar10 + iVar1);
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar9 = (float *)((int)pfVar9 + iVar2);
        pfVar8 = pfVar8 + iVar4;
      } while (iVar6 < param_4);
    }
  }
  else {
    fVar14 = *param_2;
    fVar15 = param_2[1];
    fVar12 = param_2[2];
    fVar13 = *param_3;
    fVar11 = param_3[1];
    fVar18 = param_3[2];
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
    iVar6 = 0;
    if (0 < param_4) {
      do {
        fVar16 = *pfVar9;
        fVar17 = *pfVar7;
        fVar24 = pfVar7[1];
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar23 = 1.5707964f - ABS(fVar16) * 1.0f;
        fVar19 = 3.1415927f - fVar16 * 1.0f;
        fVar21 = -3.1415927f - fVar16 * 1.0f;
        if (fVar19 < fVar16) {
          fVar16 = fVar19;
        }
        if (fVar16 < fVar21) {
          fVar16 = fVar21;
        }
        fVar19 = fVar23 * fVar23;
        fVar21 = fVar16 * fVar16;
        fVar20 = fVar19 * fVar19;
        fVar22 = fVar21 * fVar21;
        fVar19 = fVar23 * -0.16666657f * fVar19 + fVar23 * -0.00019807414f * fVar19 * fVar20 +
                 fVar23 * 0.0083330255f * fVar20 + fVar23 * 1.0f +
                 fVar23 * 2.601887e-06f * fVar20 * fVar20;
        fVar16 = fVar16 * -0.16666657f * fVar21 + fVar16 * -0.00019807414f * fVar21 * fVar22 +
                 fVar16 * 0.0083330255f * fVar22 + fVar16 * 1.0f +
                 fVar16 * 2.601887e-06f * fVar22 * fVar22;
        fVar27 = *pfVar10;
        fVar28 = pfVar10[1];
        fVar29 = pfVar10[2];
        fVar30 = pfVar10[3];
        fVar21 = (fVar14 * 0.5f * fVar19 - fVar13 * 0.5f * fVar16) * fVar17;
        fVar23 = (fVar15 * 0.5f * fVar19 - fVar11 * 0.5f * fVar16) * fVar17;
        fVar20 = (fVar12 * 0.5f * fVar19 - fVar18 * 0.5f * fVar16) * fVar17;
        fVar17 = (fStack_4 * fVar19 - fStack_14 * fVar16) * fVar17;
        fVar22 = (fVar13 * 0.5f * fVar19 + fVar14 * 0.5f * fVar16) * fVar24;
        fVar25 = (fVar11 * 0.5f * fVar19 + fVar15 * 0.5f * fVar16) * fVar24;
        fVar26 = (fVar18 * 0.5f * fVar19 + fVar12 * 0.5f * fVar16) * fVar24;
        fVar24 = (fStack_14 * fVar19 + fStack_4 * fVar16) * fVar24;
        *pfVar8 = (fVar27 - fVar21) - fVar22;
        pfVar8[1] = (fVar28 - fVar23) - fVar25;
        pfVar8[2] = (fVar29 - fVar20) - fVar26;
        pfVar8[3] = (fVar30 - fVar17) - fVar24;
        pfVar8[4] = (fVar27 + fVar21) - fVar22;
        pfVar8[5] = (fVar28 + fVar23) - fVar25;
        pfVar8[6] = (fVar29 + fVar20) - fVar26;
        pfVar8[7] = (fVar30 + fVar17) - fVar24;
        pfVar8[8] = fVar27 + fVar21 + fVar22;
        pfVar8[9] = fVar28 + fVar23 + fVar25;
        pfVar8[10] = fVar29 + fVar20 + fVar26;
        pfVar8[0xb] = fVar30 + fVar17 + fVar24;
        pfVar8[0xc] = (fVar27 - fVar21) + fVar22;
        pfVar8[0xd] = (fVar28 - fVar23) + fVar25;
        pfVar8[0xe] = (fVar29 - fVar20) + fVar26;
        pfVar8[0xf] = (fVar30 - fVar17) + fVar24;
        EI();
        iVar6 = iVar6 + 1;
        pfVar10 = (float *)((int)pfVar10 + iVar1);
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar9 = (float *)((int)pfVar9 + iVar2);
        pfVar8 = pfVar8 + iVar4;
      } while (iVar6 < param_4);
    }
  }
  return;
}
