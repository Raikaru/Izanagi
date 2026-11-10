/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003af990 rpPTankSkyInsPosCCPSPR src/promoted/code1_003a.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x003b0370) */
/* WARNING: Removing unreachable block (ram,0x003b03ac) */
/* WARNING: Removing unreachable block (ram,0x003b03cc) */
/* WARNING: Removing unreachable block (ram,0x003b03d4) */
/* WARNING: Removing unreachable block (ram,0x003b03d8) */
/* WARNING: Removing unreachable block (ram,0x003b02d0) */
/* WARNING: Removing unreachable block (ram,0x003b02f0) */
/* WARNING: Removing unreachable block (ram,0x003b02f8) */
/* WARNING: Removing unreachable block (ram,0x003b0320) */
/* WARNING: Removing unreachable block (ram,0x003b0344) */
/* WARNING: Removing unreachable block (ram,0x003b034c) */
/* WARNING: Removing unreachable block (ram,0x003b0350) */
/* WARNING: Removing unreachable block (ram,0x003afe10) */
/* WARNING: Removing unreachable block (ram,0x003afe3c) */
/* WARNING: Removing unreachable block (ram,0x003afe5c) */
/* WARNING: Removing unreachable block (ram,0x003afe68) */
/* WARNING: Removing unreachable block (ram,0x003afe88) */
/* WARNING: Removing unreachable block (ram,0x003afeb8) */
/* WARNING: Removing unreachable block (ram,0x003afedc) */
/* WARNING: Removing unreachable block (ram,0x003afee8) */
/* WARNING: Removing unreachable block (ram,0x003aff08) */
/* WARNING: Removing unreachable block (ram,0x003aff20) */
/* WARNING: Removing unreachable block (ram,0x003aff40) */
/* WARNING: Removing unreachable block (ram,0x003aff60) */
/* WARNING: Removing unreachable block (ram,0x003aff88) */
/* WARNING: Removing unreachable block (ram,0x003aff8c) */
/* WARNING: Removing unreachable block (ram,0x003b0098) */
/* WARNING: Removing unreachable block (ram,0x003b00a8) */
/* WARNING: Removing unreachable block (ram,0x003b00cc) */
/* WARNING: Removing unreachable block (ram,0x003b00d8) */
/* WARNING: Removing unreachable block (ram,0x003b00e8) */
/* WARNING: Removing unreachable block (ram,0x003b010c) */
/* WARNING: Removing unreachable block (ram,0x003b0118) */
/* WARNING: Removing unreachable block (ram,0x003b0138) */
/* WARNING: Removing unreachable block (ram,0x003b03f8) */
/* WARNING: Removing unreachable block (ram,0x003b0414) */
/* WARNING: Removing unreachable block (ram,0x003b0418) */
/* WARNING: Removing unreachable block (ram,0x003b0438) */
/* WARNING: Removing unreachable block (ram,0x003b0454) */
/* WARNING: Removing unreachable block (ram,0x003b047c) */
/* WARNING: Removing unreachable block (ram,0x003b0480) */
/* WARNING: Removing unreachable block (ram,0x003b0590) */
/* WARNING: Removing unreachable block (ram,0x003b05a4) */
/* WARNING: Removing unreachable block (ram,0x003b05c8) */
/* WARNING: Removing unreachable block (ram,0x003b05d0) */
/* WARNING: Removing unreachable block (ram,0x003b05e4) */
/* WARNING: Removing unreachable block (ram,0x003b0608) */
/* WARNING: Removing unreachable block (ram,0x003b0610) */
/* WARNING: Removing unreachable block (ram,0x003b0614) */
/* WARNING: Removing unreachable block (ram,0x003b0630) */
/* WARNING: Removing unreachable block (ram,0x003b0674) */
/* WARNING: Removing unreachable block (ram,0x003b067c) */
/* WARNING: Removing unreachable block (ram,0x003b068c) */
/* WARNING: Removing unreachable block (ram,0x003b0694) */
/* WARNING: Heritage AFTER dead removal. Example location: r0x70003f60 : 0x003afcbc */
/* WARNING: Restarted to delay deadcode elimination for space: ram */

void FUN_003af990(int *param_1,float *param_2,float *param_3,int param_4,int param_5)

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
  float fVar32;
  float fVar33;
  float fVar34;
  float fVar35;
  float fVar36;
  float fVar37;
  float fStack_14;
  float fStack_4;
  
  iVar1 = *(int *)(param_5 + 8);
  pfVar8 = (float *)*param_1;
  pfVar10 = *(float **)(param_5 + 4);
  pfVar9 = *(float **)(param_5 + 0x34);
  pfVar7 = *(float **)(param_5 + 0x1c);
  iVar2 = *(int *)(param_5 + 0x38);
  iVar3 = *(int *)(param_5 + 0x20);
  iVar4 = param_1[1];
  if (iVar1 == iVar4) {
    FUN_00421a60(0);
    fVar20 = fRam70003f64;
    fVar19 = fRam70003f60;
    fVar12 = fRam70003f5c;
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
    fRam70003f60 = *(float *)(param_5 + 0x68);
    fRam70003f64 = *(float *)(param_5 + 0x6c);
    uRam70003f68 = *(unsigned int *)(param_5 + 0x68);
    uRam70003f6c = *(unsigned int *)(param_5 + 0x6c);
    fVar13 = *param_2 * 0.5f;
    fVar14 = param_2[1] * 0.5f;
    fVar15 = param_2[2] * 0.5f;
    fVar16 = *param_3 * 0.5f;
    fVar17 = param_3[1] * 0.5f;
    fVar18 = param_3[2] * 0.5f;
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
    fRam70003f40 = fVar13;
    fRam70003f44 = fVar14;
    fRam70003f48 = fVar15;
    fRam70003f50 = fVar16;
    fRam70003f54 = fVar17;
    fRam70003f58 = fVar18;
    if (0 < param_4) {
      do {
        fRam70003fa0 = *pfVar9;
        fRam70003fa8 = *pfVar9;
        fRam70003f60 = *pfVar7;
        fRam70003f64 = pfVar7[1];
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar26 = 1.5707964f - ABS(fRam70003fa0) * 1.0f;
        fVar22 = 3.1415927f - fRam70003fa8 * 1.0f;
        fVar24 = -3.1415927f - fRam70003fa8 * 1.0f;
        fVar21 = fRam70003fa8;
        if (fVar22 < fRam70003fa8) {
          fVar21 = fVar22;
        }
        if (fVar21 < fVar24) {
          fVar21 = fVar24;
        }
        fVar22 = fVar26 * fVar26;
        fVar24 = fVar21 * fVar21;
        fVar23 = fVar22 * fVar22;
        fVar25 = fVar24 * fVar24;
        fVar23 = fVar26 * -0.16666657f * fVar22 + fVar26 * -0.00019807414f * fVar22 * fVar23 +
                 fVar26 * 0.0083330255f * fVar23 + fVar26 * 1.0f +
                 fVar26 * 2.601887e-06f * fVar23 * fVar23;
        fVar25 = fVar21 * -0.16666657f * fVar24 + fVar21 * -0.00019807414f * fVar24 * fVar25 +
                 fVar21 * 0.0083330255f * fVar25 + fVar21 * 1.0f +
                 fVar21 * 2.601887e-06f * fVar25 * fVar25;
        fVar34 = *pfVar10;
        fVar35 = pfVar10[1];
        fVar36 = pfVar10[2];
        fVar37 = pfVar10[3];
        fVar27 = fVar13 * fVar23 - fVar16 * fVar25;
        fVar28 = fVar14 * fVar23 - fVar17 * fVar25;
        fVar29 = fVar15 * fVar23 - fVar18 * fVar25;
        fVar30 = fVar11 * fVar23 - fVar12 * fVar25;
        fVar21 = fVar19 + fRam70003f60;
        fVar22 = fVar20 + fRam70003f64;
        fVar24 = fVar19 - fRam70003f60;
        fVar26 = fVar20 - fRam70003f64;
        fVar31 = fVar16 * fVar23 + fVar13 * fVar25;
        fVar32 = fVar17 * fVar23 + fVar14 * fVar25;
        fVar33 = fVar18 * fVar23 + fVar15 * fVar25;
        fVar23 = fVar12 * fVar23 + fVar11 * fVar25;
        *pfVar8 = fVar27 * fVar24 + fVar31 * fVar26 + fVar34;
        pfVar8[1] = fVar28 * fVar24 + fVar32 * fVar26 + fVar35;
        pfVar8[2] = fVar29 * fVar24 + fVar33 * fVar26 + fVar36;
        pfVar8[3] = fVar30 * fVar24 + fVar23 * fVar26 + fVar37;
        pfVar8[4] = fVar27 * fVar21 + fVar31 * fVar26 + fVar34;
        pfVar8[5] = fVar28 * fVar21 + fVar32 * fVar26 + fVar35;
        pfVar8[6] = fVar29 * fVar21 + fVar33 * fVar26 + fVar36;
        pfVar8[7] = fVar30 * fVar21 + fVar23 * fVar26 + fVar37;
        pfVar8[8] = fVar27 * fVar21 + fVar31 * fVar22 + fVar34;
        pfVar8[9] = fVar28 * fVar21 + fVar32 * fVar22 + fVar35;
        pfVar8[10] = fVar29 * fVar21 + fVar33 * fVar22 + fVar36;
        pfVar8[0xb] = fVar30 * fVar21 + fVar23 * fVar22 + fVar37;
        pfVar8[0xc] = fVar27 * fVar24 + fVar31 * fVar22 + fVar34;
        pfVar8[0xd] = fVar28 * fVar24 + fVar32 * fVar22 + fVar35;
        pfVar8[0xe] = fVar29 * fVar24 + fVar33 * fVar22 + fVar36;
        pfVar8[0xf] = fVar30 * fVar24 + fVar23 * fVar22 + fVar37;
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
    fVar13 = *param_2;
    fVar11 = param_2[1];
    fVar19 = param_2[2];
    fVar15 = *(float *)(param_5 + 0x68) + *(float *)(param_5 + 0x68);
    fVar16 = *(float *)(param_5 + 0x6c) + *(float *)(param_5 + 0x6c);
    fVar14 = *param_3;
    fVar12 = param_3[1];
    fVar20 = param_3[2];
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
        fVar17 = *pfVar9;
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar22 = 1.5707964f - ABS(fVar17) * 1.0f;
        fVar18 = 3.1415927f - fVar17 * 1.0f;
        fVar21 = -3.1415927f - fVar17 * 1.0f;
        if (fVar18 < fVar17) {
          fVar17 = fVar18;
        }
        if (fVar17 < fVar21) {
          fVar17 = fVar21;
        }
        fVar18 = fVar22 * fVar22;
        fVar21 = fVar17 * fVar17;
        fVar24 = fVar18 * fVar18;
        fVar26 = fVar21 * fVar21;
        fVar24 = fVar22 * -0.16666657f * fVar18 + fVar22 * -0.00019807414f * fVar18 * fVar24 +
                 fVar22 * 0.0083330255f * fVar24 + fVar22 * 1.0f +
                 fVar22 * 2.601887e-06f * fVar24 * fVar24;
        fVar26 = fVar17 * -0.16666657f * fVar21 + fVar17 * -0.00019807414f * fVar21 * fVar26 +
                 fVar17 * 0.0083330255f * fVar26 + fVar17 * 1.0f +
                 fVar17 * 2.601887e-06f * fVar26 * fVar26;
        fVar32 = *pfVar10;
        fVar33 = pfVar10[1];
        fVar34 = pfVar10[2];
        fVar35 = pfVar10[3];
        fVar23 = fVar13 * 0.5f * fVar24 - fVar14 * 0.5f * fVar26;
        fVar25 = fVar11 * 0.5f * fVar24 - fVar12 * 0.5f * fVar26;
        fVar27 = fVar19 * 0.5f * fVar24 - fVar20 * 0.5f * fVar26;
        fVar28 = fStack_4 * fVar24 - fStack_14 * fVar26;
        fVar17 = fVar15 + *pfVar7;
        fVar18 = fVar16 + pfVar7[1];
        fVar21 = fVar15 - *pfVar7;
        fVar22 = fVar16 - pfVar7[1];
        fVar29 = fVar14 * 0.5f * fVar24 + fVar13 * 0.5f * fVar26;
        fVar30 = fVar12 * 0.5f * fVar24 + fVar11 * 0.5f * fVar26;
        fVar31 = fVar20 * 0.5f * fVar24 + fVar19 * 0.5f * fVar26;
        fVar24 = fStack_14 * fVar24 + fStack_4 * fVar26;
        *pfVar8 = fVar23 * fVar21 + fVar29 * fVar22 + fVar32;
        pfVar8[1] = fVar25 * fVar21 + fVar30 * fVar22 + fVar33;
        pfVar8[2] = fVar27 * fVar21 + fVar31 * fVar22 + fVar34;
        pfVar8[3] = fVar28 * fVar21 + fVar24 * fVar22 + fVar35;
        pfVar8[4] = fVar23 * fVar17 + fVar29 * fVar22 + fVar32;
        pfVar8[5] = fVar25 * fVar17 + fVar30 * fVar22 + fVar33;
        pfVar8[6] = fVar27 * fVar17 + fVar31 * fVar22 + fVar34;
        pfVar8[7] = fVar28 * fVar17 + fVar24 * fVar22 + fVar35;
        pfVar8[8] = fVar23 * fVar17 + fVar29 * fVar18 + fVar32;
        pfVar8[9] = fVar25 * fVar17 + fVar30 * fVar18 + fVar33;
        pfVar8[10] = fVar27 * fVar17 + fVar31 * fVar18 + fVar34;
        pfVar8[0xb] = fVar28 * fVar17 + fVar24 * fVar18 + fVar35;
        pfVar8[0xc] = fVar23 * fVar21 + fVar29 * fVar18 + fVar32;
        pfVar8[0xd] = fVar25 * fVar21 + fVar30 * fVar18 + fVar33;
        pfVar8[0xe] = fVar27 * fVar21 + fVar31 * fVar18 + fVar34;
        pfVar8[0xf] = fVar28 * fVar21 + fVar24 * fVar18 + fVar35;
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
