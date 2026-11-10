/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003ab100 rpPTankSkyInsPosNCCSPR src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003ab100_arg4 fn_003ab100_arg4;
struct fn_003ab100_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x28];
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
    unsigned char pad_0x3c[0x34];
    float field_0x70; /* +0x70 */
    float field_0x74; /* +0x74 */
};


/* WARNING: Removing unreachable block (ram,0x003ab988) */
/* WARNING: Removing unreachable block (ram,0x003ab9c4) */
/* WARNING: Removing unreachable block (ram,0x003ab9e4) */
/* WARNING: Removing unreachable block (ram,0x003ab9ec) */
/* WARNING: Removing unreachable block (ram,0x003ab9f0) */
/* WARNING: Removing unreachable block (ram,0x003ab580) */
/* WARNING: Removing unreachable block (ram,0x003ab5ac) */
/* WARNING: Removing unreachable block (ram,0x003ab5cc) */
/* WARNING: Removing unreachable block (ram,0x003ab5d8) */
/* WARNING: Removing unreachable block (ram,0x003ab5f8) */
/* WARNING: Removing unreachable block (ram,0x003ab608) */
/* WARNING: Removing unreachable block (ram,0x003ab628) */
/* WARNING: Removing unreachable block (ram,0x003ab644) */
/* WARNING: Removing unreachable block (ram,0x003ab674) */
/* WARNING: Removing unreachable block (ram,0x003ab678) */
/* WARNING: Removing unreachable block (ram,0x003ab75c) */
/* WARNING: Removing unreachable block (ram,0x003ab76c) */
/* WARNING: Removing unreachable block (ram,0x003ab78c) */
/* WARNING: Removing unreachable block (ram,0x003ab798) */
/* WARNING: Removing unreachable block (ram,0x003ab7a8) */
/* WARNING: Removing unreachable block (ram,0x003ab7cc) */
/* WARNING: Removing unreachable block (ram,0x003ab7d8) */
/* WARNING: Removing unreachable block (ram,0x003ab7f8) */
/* WARNING: Removing unreachable block (ram,0x003aba10) */
/* WARNING: Removing unreachable block (ram,0x003aba1c) */
/* WARNING: Removing unreachable block (ram,0x003aba24) */
/* WARNING: Removing unreachable block (ram,0x003aba44) */
/* WARNING: Removing unreachable block (ram,0x003aba48) */
/* WARNING: Removing unreachable block (ram,0x003aba68) */
/* WARNING: Removing unreachable block (ram,0x003aba84) */
/* WARNING: Removing unreachable block (ram,0x003abab4) */
/* WARNING: Removing unreachable block (ram,0x003abab8) */
/* WARNING: Removing unreachable block (ram,0x003abba0) */
/* WARNING: Removing unreachable block (ram,0x003abbb4) */
/* WARNING: Removing unreachable block (ram,0x003abbd8) */
/* WARNING: Removing unreachable block (ram,0x003abbe0) */
/* WARNING: Removing unreachable block (ram,0x003abbf4) */
/* WARNING: Removing unreachable block (ram,0x003abc18) */
/* WARNING: Removing unreachable block (ram,0x003abc20) */
/* WARNING: Removing unreachable block (ram,0x003abc24) */
/* WARNING: Removing unreachable block (ram,0x003abc40) */
/* WARNING: Removing unreachable block (ram,0x003abc54) */
/* WARNING: Removing unreachable block (ram,0x003abc5c) */

void FUN_003ab100(int *param_1,float *param_2,float *param_3,int param_4,fn_003ab100_arg4 *param_5)

{
  int iVar1;
  int iVar2;
  int iVar3;
  char cVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  int iVar8;
  float *pfVar9;
  float *pfVar10;
  float *pfVar11;
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
  float fStack_4;
  
  iVar1 = param_5->field_0x38;
  iVar2 = param_5->field_0x8;
  iVar3 = param_1[1];
  pfVar11 = (float *)param_5->field_0x4;
  pfVar10 = (float *)param_5->field_0x34;
  pfVar9 = (float *)*param_1;
  fVar13 = param_5->field_0x70 * 0.5f;
  fVar12 = param_5->field_0x74 * 0.5f;
  if (iVar2 == iVar3) {
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
    fRam70003f00 = *param_2;
    fRam70003f04 = param_2[1];
    fRam70003f08 = param_2[2];
    fRam70003f10 = *param_3;
    fRam70003f14 = param_3[1];
    fRam70003f18 = param_3[2];
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    fVar20 = fRam70003f00 * fVar13;
    fVar21 = fRam70003f04 * fVar13;
    fVar22 = fRam70003f08 * fVar13;
    fVar26 = fRam70003f10 * fVar13;
    fVar27 = fRam70003f14 * fVar13;
    fVar28 = fRam70003f18 * fVar13;
    fVar23 = fRam70003f00 * fVar12;
    fVar24 = fRam70003f04 * fVar12;
    fVar25 = fRam70003f08 * fVar12;
    fVar29 = fRam70003f10 * fVar12;
    fVar30 = fRam70003f14 * fVar12;
    fVar31 = fRam70003f18 * fVar12;
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
    iVar8 = 0;
    fRam70003f30 = fVar13;
    fRam70003f34 = fVar13;
    fRam70003f38 = fVar13;
    fRam70003f3c = fVar13;
    fRam70003f40 = fVar12;
    fRam70003f44 = fVar12;
    fRam70003f48 = fVar12;
    fRam70003f4c = fVar12;
    if (0 < param_4) {
      do {
        fVar12 = *pfVar10;
        fVar33 = *pfVar11;
        fVar34 = pfVar11[1];
        fVar35 = pfVar11[2];
        fVar36 = pfVar11[3];
        fVar19 = 1.5707964f - ABS(fVar12) * 1.0f;
        fVar13 = 3.1415927f - fVar12 * 1.0f;
        fVar14 = 3.1415927f - fStack_4 * 1.0f;
        fVar15 = -3.1415927f - fVar12 * 1.0f;
        fVar17 = -3.1415925f - fStack_4 * 1.0f;
        if (fVar13 < fVar12) {
          fVar12 = fVar13;
        }
        fVar13 = fStack_4;
        if (fVar14 < fStack_4) {
          fVar13 = fVar14;
        }
        if (fVar12 < fVar15) {
          fVar12 = fVar15;
        }
        if (fVar13 < fVar17) {
          fVar13 = fVar17;
        }
        fVar14 = fVar19 * fVar19;
        fVar15 = fVar12 * fVar12;
        fVar17 = fVar14 * fVar14;
        fVar16 = fVar15 * fVar15;
        fVar18 = fVar13 * fVar13 * fVar13 * fVar13;
        fVar14 = fVar19 * -0.16666657f * fVar14 + fVar19 * -0.00019807414f * fVar14 * fVar17 +
                 fVar19 * 0.0083330255f * fVar17 + fVar19 * 1.0f +
                 fVar19 * 2.601887e-06f * fVar17 * fVar17;
        fVar12 = fVar12 * -0.16666657f * fVar15 + fVar12 * -0.00019807414f * fVar15 * fVar16 +
                 fVar12 * 0.0083330255f * fVar16 + fVar12 * 1.0f +
                 fVar12 * 2.601887e-06f * fVar16 * fVar16;
        fVar15 = fVar20 * fVar14 - fVar26 * fVar12;
        fVar17 = fVar21 * fVar14 - fVar27 * fVar12;
        fVar19 = fVar22 * fVar14 - fVar28 * fVar12;
        fVar18 = fVar18 * fVar18 - fVar18;
        fVar16 = fVar23 * fVar12 + fVar29 * fVar14;
        fVar32 = fVar24 * fVar12 + fVar30 * fVar14;
        fVar12 = fVar25 * fVar12 + fVar31 * fVar14;
        fVar13 = fVar13 * 2.601887e-06f + fVar13 * 0.0083330255f;
        *pfVar9 = (fVar33 - fVar15) - fVar16;
        pfVar9[1] = (fVar34 - fVar17) - fVar32;
        pfVar9[2] = (fVar35 - fVar19) - fVar12;
        pfVar9[3] = (fVar36 - fVar18) - fVar13;
        pfVar9[4] = (fVar33 + fVar15) - fVar16;
        pfVar9[5] = (fVar34 + fVar17) - fVar32;
        pfVar9[6] = (fVar35 + fVar19) - fVar12;
        pfVar9[7] = (fVar36 + fVar18) - fVar13;
        pfVar9[8] = fVar33 + fVar15 + fVar16;
        pfVar9[9] = fVar34 + fVar17 + fVar32;
        pfVar9[10] = fVar35 + fVar19 + fVar12;
        pfVar9[0xb] = fVar36 + fVar18 + fVar13;
        pfVar9[0xc] = (fVar33 - fVar15) + fVar16;
        pfVar9[0xd] = (fVar34 - fVar17) + fVar32;
        pfVar9[0xe] = (fVar35 - fVar19) + fVar12;
        pfVar9[0xf] = (fVar36 - fVar18) + fVar13;
        EI();
        iVar8 = iVar8 + 1;
        pfVar11 = (float *)((int)pfVar11 + iVar2);
        pfVar10 = (float *)((int)pfVar10 + iVar1);
        pfVar9 = pfVar9 + iVar3;
      } while (iVar8 < param_4);
    }
  }
  else {
    iVar8 = 0;
    if (0 < param_4) {
      do {
        fVar20 = (float)FUN_0044b7b0(*pfVar10);
        fVar21 = (float)FUN_0044b610(*pfVar10);
        pfVar7 = (float *)((int)pfVar9 + iVar3);
        pfVar6 = (float *)((int)pfVar7 + iVar3);
        fVar22 = -fVar20;
        pfVar5 = (float *)((int)pfVar6 + iVar3);
        iVar8 = iVar8 + 1;
        fVar25 = (fVar21 * *param_2 + fVar22 * *param_3) * fVar13;
        pfVar10 = (float *)((int)pfVar10 + iVar1);
        fVar27 = (fVar21 * param_2[1] + fVar22 * param_3[1]) * fVar13;
        fVar24 = (fVar20 * param_2[1] + fVar21 * param_3[1]) * fVar12;
        fVar23 = (fVar20 * *param_2 + fVar21 * *param_3) * fVar12;
        fVar26 = (fVar20 * param_2[2] + fVar21 * param_3[2]) * fVar12;
        fVar20 = (fVar21 * param_2[2] + fVar22 * param_3[2]) * fVar13;
        *pfVar9 = *pfVar11 + (-fVar25 - fVar23);
        pfVar9[1] = pfVar11[1] + (-fVar27 - fVar24);
        pfVar9[2] = pfVar11[2] + (-fVar20 - fVar26);
        pfVar9 = (float *)((int)pfVar5 + iVar3);
        *pfVar7 = *pfVar11 + (fVar25 - fVar23);
        pfVar7[1] = pfVar11[1] + (fVar27 - fVar24);
        pfVar7[2] = pfVar11[2] + (fVar20 - fVar26);
        *pfVar6 = *pfVar11 + fVar25 + fVar23;
        pfVar6[1] = pfVar11[1] + fVar27 + fVar24;
        pfVar6[2] = pfVar11[2] + fVar20 + fVar26;
        *pfVar5 = *pfVar11 + -fVar25 + fVar23;
        pfVar5[1] = pfVar11[1] + -fVar27 + fVar24;
        pfVar6 = pfVar11 + 2;
        pfVar11 = (float *)((int)pfVar11 + iVar2);
        pfVar5[2] = *pfVar6 + -fVar20 + fVar26;
      } while (iVar8 < param_4);
    }
  }
  return;
}
