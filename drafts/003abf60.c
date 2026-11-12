/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003abf60 rpPTankSkyInsPosNCPSNR src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003abf60_arg4 fn_003abf60_arg4;
struct fn_003abf60_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x10];
    int field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
};


/* WARNING: Removing unreachable block (ram,0x003ac620) */
/* WARNING: Removing unreachable block (ram,0x003ac65c) */
/* WARNING: Removing unreachable block (ram,0x003ac67c) */
/* WARNING: Removing unreachable block (ram,0x003ac684) */
/* WARNING: Removing unreachable block (ram,0x003ac688) */
/* WARNING: Removing unreachable block (ram,0x003ac238) */
/* WARNING: Removing unreachable block (ram,0x003ac264) */
/* WARNING: Removing unreachable block (ram,0x003ac284) */
/* WARNING: Removing unreachable block (ram,0x003ac290) */
/* WARNING: Removing unreachable block (ram,0x003ac2b0) */
/* WARNING: Removing unreachable block (ram,0x003ac2c0) */
/* WARNING: Removing unreachable block (ram,0x003ac2dc) */
/* WARNING: Removing unreachable block (ram,0x003ac3f0) */
/* WARNING: Removing unreachable block (ram,0x003ac400) */
/* WARNING: Removing unreachable block (ram,0x003ac424) */
/* WARNING: Removing unreachable block (ram,0x003ac430) */
/* WARNING: Removing unreachable block (ram,0x003ac440) */
/* WARNING: Removing unreachable block (ram,0x003ac464) */
/* WARNING: Removing unreachable block (ram,0x003ac470) */
/* WARNING: Removing unreachable block (ram,0x003ac490) */
/* WARNING: Removing unreachable block (ram,0x003ac6a8) */
/* WARNING: Removing unreachable block (ram,0x003ac6b4) */
/* WARNING: Removing unreachable block (ram,0x003ac6bc) */
/* WARNING: Removing unreachable block (ram,0x003ac6dc) */
/* WARNING: Removing unreachable block (ram,0x003ac6e0) */
/* WARNING: Removing unreachable block (ram,0x003ac6fc) */
/* WARNING: Removing unreachable block (ram,0x003ac810) */
/* WARNING: Removing unreachable block (ram,0x003ac824) */
/* WARNING: Removing unreachable block (ram,0x003ac848) */
/* WARNING: Removing unreachable block (ram,0x003ac850) */
/* WARNING: Removing unreachable block (ram,0x003ac864) */
/* WARNING: Removing unreachable block (ram,0x003ac888) */
/* WARNING: Removing unreachable block (ram,0x003ac890) */
/* WARNING: Removing unreachable block (ram,0x003ac894) */
/* WARNING: Removing unreachable block (ram,0x003ac8b0) */
/* WARNING: Removing unreachable block (ram,0x003ac8c4) */
/* WARNING: Removing unreachable block (ram,0x003ac8cc) */

void FUN_003abf60(int *param_1,unsigned long long param_2,unsigned long long param_3,int param_4,
                 fn_003abf60_arg4 *param_5)

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
  float extraout_vf7w;
  float extraout_vf7w_00;
  float extraout_vf8w;
  float extraout_vf8w_00;
  float extraout_vf9w;
  float extraout_vf9w_00;
  float extraout_vf10w;
  float extraout_vf10w_00;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  float fStack_20;
  float fStack_1c;
  float fStack_18;
  
  iVar1 = param_5->field_0x8;
  iVar2 = param_1[1];
  pfVar8 = (float *)param_5->field_0x4;
  pfVar7 = (float *)param_5->field_0x1c;
  iVar3 = param_5->field_0x20;
  pfVar6 = (float *)*param_1;
  if (iVar1 == iVar2) {
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
    pfVar9 = (float *)param_2;
    fRam70003f50 = *pfVar9;
    fRam70003f54 = pfVar9[1];
    fRam70003f58 = pfVar9[2];
    pfVar9 = (float *)param_3;
    fRam70003f60 = *pfVar9;
    fRam70003f64 = pfVar9[1];
    fRam70003f68 = pfVar9[2];
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    fVar12 = fRam70003f50 * 0.5f;
    fVar13 = fRam70003f54 * 0.5f;
    fVar14 = fRam70003f58 * 0.5f;
    fVar15 = fRam70003f60 * 0.5f;
    fVar16 = fRam70003f64 * 0.5f;
    fVar17 = fRam70003f68 * 0.5f;
    EI();
    if (0xb7 < param_4) {
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
    if (0 < param_4) {
      do {
        fVar10 = *pfVar7;
        fVar11 = pfVar7[1];
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar18 = fVar12 * fVar10;
        fVar19 = fVar13 * fVar10;
        fVar10 = fVar14 * fVar10;
        fVar20 = fVar15 * fVar11;
        fVar21 = fVar16 * fVar11;
        fVar11 = fVar17 * fVar11;
        fVar22 = *pfVar8;
        fVar23 = pfVar8[1];
        fVar24 = pfVar8[2];
        *pfVar6 = (fVar22 - fVar18) - fVar20;
        pfVar6[1] = (fVar23 - fVar19) - fVar21;
        pfVar6[2] = (fVar24 - fVar10) - fVar11;
        pfVar6[3] = extraout_vf7w;
        pfVar6[4] = (fVar22 + fVar18) - fVar20;
        pfVar6[5] = (fVar23 + fVar19) - fVar21;
        pfVar6[6] = (fVar24 + fVar10) - fVar11;
        pfVar6[7] = extraout_vf8w;
        pfVar6[8] = fVar22 + fVar18 + fVar20;
        pfVar6[9] = fVar23 + fVar19 + fVar21;
        pfVar6[10] = fVar24 + fVar10 + fVar11;
        pfVar6[0xb] = extraout_vf9w;
        pfVar6[0xc] = (fVar22 - fVar18) + fVar20;
        pfVar6[0xd] = (fVar23 - fVar19) + fVar21;
        pfVar6[0xe] = (fVar24 - fVar10) + fVar11;
        pfVar6[0xf] = extraout_vf10w;
        EI();
        iVar5 = iVar5 + 1;
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar6 = pfVar6 + iVar2;
        pfVar8 = (float *)((int)pfVar8 + iVar1);
      } while (iVar5 < param_4);
    }
  }
  else {
    FUN_0043f810(&fStack_20,param_2,0xc);
    FUN_0043f810(&fStack_30,param_3,0xc);
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    iVar5 = 0;
    if (0 < param_4) {
      do {
        fVar12 = *pfVar7;
        fVar13 = pfVar7[1];
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar14 = fStack_20 * 0.5f * fVar12;
        fVar15 = fStack_1c * 0.5f * fVar12;
        fVar12 = fStack_18 * 0.5f * fVar12;
        fVar16 = fStack_30 * 0.5f * fVar13;
        fVar17 = fStack_2c * 0.5f * fVar13;
        fVar13 = fStack_28 * 0.5f * fVar13;
        fVar10 = *pfVar8;
        fVar11 = pfVar8[1];
        fVar18 = pfVar8[2];
        *pfVar6 = (fVar10 - fVar14) - fVar16;
        pfVar6[1] = (fVar11 - fVar15) - fVar17;
        pfVar6[2] = (fVar18 - fVar12) - fVar13;
        pfVar6[3] = extraout_vf7w_00;
        pfVar6[4] = (fVar10 + fVar14) - fVar16;
        pfVar6[5] = (fVar11 + fVar15) - fVar17;
        pfVar6[6] = (fVar18 + fVar12) - fVar13;
        pfVar6[7] = extraout_vf8w_00;
        pfVar6[8] = fVar10 + fVar14 + fVar16;
        pfVar6[9] = fVar11 + fVar15 + fVar17;
        pfVar6[10] = fVar18 + fVar12 + fVar13;
        pfVar6[0xb] = extraout_vf9w_00;
        pfVar6[0xc] = (fVar10 - fVar14) + fVar16;
        pfVar6[0xd] = (fVar11 - fVar15) + fVar17;
        pfVar6[0xe] = (fVar18 - fVar12) + fVar13;
        pfVar6[0xf] = extraout_vf10w_00;
        EI();
        iVar5 = iVar5 + 1;
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar6 = pfVar6 + iVar2;
        pfVar8 = (float *)((int)pfVar8 + iVar1);
      } while (iVar5 < param_4);
    }
  }
  return;
}
