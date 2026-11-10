/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003aed60 rpPTankSkyInsPosCCPSNR src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003aed60_arg4 fn_003aed60_arg4;
struct fn_003aed60_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x10];
    int field_0x1c; /* +0x1c */
    int field_0x20; /* +0x20 */
    unsigned char pad_0x24[0x44];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
};


/* WARNING: Removing unreachable block (ram,0x003af460) */
/* WARNING: Removing unreachable block (ram,0x003af49c) */
/* WARNING: Removing unreachable block (ram,0x003af4bc) */
/* WARNING: Removing unreachable block (ram,0x003af4c4) */
/* WARNING: Removing unreachable block (ram,0x003af4c8) */
/* WARNING: Removing unreachable block (ram,0x003af068) */
/* WARNING: Removing unreachable block (ram,0x003af094) */
/* WARNING: Removing unreachable block (ram,0x003af0b4) */
/* WARNING: Removing unreachable block (ram,0x003af0c0) */
/* WARNING: Removing unreachable block (ram,0x003af0e0) */
/* WARNING: Removing unreachable block (ram,0x003af0f0) */
/* WARNING: Removing unreachable block (ram,0x003af10c) */
/* WARNING: Removing unreachable block (ram,0x003af230) */
/* WARNING: Removing unreachable block (ram,0x003af240) */
/* WARNING: Removing unreachable block (ram,0x003af264) */
/* WARNING: Removing unreachable block (ram,0x003af270) */
/* WARNING: Removing unreachable block (ram,0x003af280) */
/* WARNING: Removing unreachable block (ram,0x003af2a4) */
/* WARNING: Removing unreachable block (ram,0x003af2b0) */
/* WARNING: Removing unreachable block (ram,0x003af2d0) */
/* WARNING: Removing unreachable block (ram,0x003af4e8) */
/* WARNING: Removing unreachable block (ram,0x003af4f4) */
/* WARNING: Removing unreachable block (ram,0x003af4fc) */
/* WARNING: Removing unreachable block (ram,0x003af51c) */
/* WARNING: Removing unreachable block (ram,0x003af520) */
/* WARNING: Removing unreachable block (ram,0x003af53c) */
/* WARNING: Removing unreachable block (ram,0x003af660) */
/* WARNING: Removing unreachable block (ram,0x003af674) */
/* WARNING: Removing unreachable block (ram,0x003af698) */
/* WARNING: Removing unreachable block (ram,0x003af6a0) */
/* WARNING: Removing unreachable block (ram,0x003af6b4) */
/* WARNING: Removing unreachable block (ram,0x003af6d8) */
/* WARNING: Removing unreachable block (ram,0x003af6e0) */
/* WARNING: Removing unreachable block (ram,0x003af6e4) */
/* WARNING: Removing unreachable block (ram,0x003af700) */
/* WARNING: Removing unreachable block (ram,0x003af714) */
/* WARNING: Removing unreachable block (ram,0x003af71c) */

void FUN_003aed60(int *param_1,unsigned long long param_2,unsigned long long param_3,int param_4,
                 fn_003aed60_arg4 *param_5)

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
  float extraout_vf7w;
  float extraout_vf7w_00;
  float extraout_vf8w;
  float extraout_vf8w_00;
  float extraout_vf9w;
  float extraout_vf9w_00;
  float extraout_vf10w;
  float extraout_vf10w_00;
  float fVar28;
  float fVar29;
  float fVar30;
  float fStack_40;
  float fStack_3c;
  float fStack_38;
  float fStack_30;
  float fStack_2c;
  float fStack_28;
  
  iVar1 = param_1[1];
  iVar2 = param_5->field_0x8;
  pfVar8 = (float *)param_5->field_0x4;
  pfVar7 = (float *)param_5->field_0x1c;
  iVar3 = param_5->field_0x20;
  pfVar6 = (float *)*param_1;
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
    pfVar9 = (float *)param_2;
    fRam70003f50 = *pfVar9;
    fRam70003f54 = pfVar9[1];
    fRam70003f58 = pfVar9[2];
    pfVar9 = (float *)param_3;
    fRam70003f60 = *pfVar9;
    fRam70003f64 = pfVar9[1];
    fRam70003f68 = pfVar9[2];
    fRam70003f90 = param_5->field_0x68;
    fRam70003f94 = param_5->field_0x6c;
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    fVar28 = fRam70003f50 * fRam70003f90;
    fVar29 = fRam70003f54 * fRam70003f90;
    fVar30 = fRam70003f58 * fRam70003f90;
    fVar11 = fRam70003f60 * fRam70003f94;
    fVar14 = fRam70003f64 * fRam70003f94;
    fVar12 = fRam70003f68 * fRam70003f94;
    fVar15 = fRam70003f50 * 0.5f;
    fVar16 = fRam70003f54 * 0.5f;
    fVar17 = fRam70003f58 * 0.5f;
    fVar18 = fRam70003f60 * 0.5f;
    fVar19 = fRam70003f64 * 0.5f;
    fVar20 = fRam70003f68 * 0.5f;
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
        fVar13 = pfVar7[1];
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar21 = fVar15 * fVar10;
        fVar22 = fVar16 * fVar10;
        fVar10 = fVar17 * fVar10;
        fVar23 = fVar18 * fVar13;
        fVar24 = fVar19 * fVar13;
        fVar13 = fVar20 * fVar13;
        fVar25 = *pfVar8 + fVar28 + fVar11 + 0.0f;
        fVar26 = pfVar8[1] + fVar29 + fVar14 + 0.0f;
        fVar27 = pfVar8[2] + fVar30 + fVar12 + 0.0f;
        *pfVar6 = (fVar25 - fVar21) - fVar23;
        pfVar6[1] = (fVar26 - fVar22) - fVar24;
        pfVar6[2] = (fVar27 - fVar10) - fVar13;
        pfVar6[3] = extraout_vf7w;
        pfVar6[4] = (fVar25 + fVar21) - fVar23;
        pfVar6[5] = (fVar26 + fVar22) - fVar24;
        pfVar6[6] = (fVar27 + fVar10) - fVar13;
        pfVar6[7] = extraout_vf8w;
        pfVar6[8] = fVar25 + fVar21 + fVar23;
        pfVar6[9] = fVar26 + fVar22 + fVar24;
        pfVar6[10] = fVar27 + fVar10 + fVar13;
        pfVar6[0xb] = extraout_vf9w;
        pfVar6[0xc] = (fVar25 - fVar21) + fVar23;
        pfVar6[0xd] = (fVar26 - fVar22) + fVar24;
        pfVar6[0xe] = (fVar27 - fVar10) + fVar13;
        pfVar6[0xf] = extraout_vf10w;
        EI();
        iVar5 = iVar5 + 1;
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar6 = pfVar6 + iVar1;
        pfVar8 = (float *)((int)pfVar8 + iVar2);
      } while (iVar5 < param_4);
    }
  }
  else {
    FUN_0043f810(&fStack_30,param_2,0xc);
    FUN_0043f810(&fStack_40,param_3,0xc);
    fVar11 = param_5->field_0x68;
    fVar14 = param_5->field_0x6c;
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    iVar5 = 0;
    if (0 < param_4) {
      do {
        fVar12 = *pfVar7;
        fVar15 = pfVar7[1];
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar16 = fStack_30 * 0.5f * fVar12;
        fVar17 = fStack_2c * 0.5f * fVar12;
        fVar12 = fStack_28 * 0.5f * fVar12;
        fVar18 = fStack_40 * 0.5f * fVar15;
        fVar19 = fStack_3c * 0.5f * fVar15;
        fVar15 = fStack_38 * 0.5f * fVar15;
        fVar20 = *pfVar8 + fStack_30 * fVar11 + fStack_40 * fVar14 + 0.0f;
        fVar28 = pfVar8[1] + fStack_2c * fVar11 + fStack_3c * fVar14 + 0.0f;
        fVar29 = pfVar8[2] + fStack_28 * fVar11 + fStack_38 * fVar14 + 0.0f;
        *pfVar6 = (fVar20 - fVar16) - fVar18;
        pfVar6[1] = (fVar28 - fVar17) - fVar19;
        pfVar6[2] = (fVar29 - fVar12) - fVar15;
        pfVar6[3] = extraout_vf7w_00;
        pfVar6[4] = (fVar20 + fVar16) - fVar18;
        pfVar6[5] = (fVar28 + fVar17) - fVar19;
        pfVar6[6] = (fVar29 + fVar12) - fVar15;
        pfVar6[7] = extraout_vf8w_00;
        pfVar6[8] = fVar20 + fVar16 + fVar18;
        pfVar6[9] = fVar28 + fVar17 + fVar19;
        pfVar6[10] = fVar29 + fVar12 + fVar15;
        pfVar6[0xb] = extraout_vf9w_00;
        pfVar6[0xc] = (fVar20 - fVar16) + fVar18;
        pfVar6[0xd] = (fVar28 - fVar17) + fVar19;
        pfVar6[0xe] = (fVar29 - fVar12) + fVar15;
        pfVar6[0xf] = extraout_vf10w_00;
        EI();
        iVar5 = iVar5 + 1;
        pfVar7 = (float *)((int)pfVar7 + iVar3);
        pfVar6 = pfVar6 + iVar1;
        pfVar8 = (float *)((int)pfVar8 + iVar2);
      } while (iVar5 < param_4);
    }
  }
  return;
}
