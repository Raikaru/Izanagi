/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b0b80 rpPTankSkyInsPosNCPM src/promoted/code1_003b.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x003b0dd8) */
/* WARNING: Removing unreachable block (ram,0x003b0de8) */
/* WARNING: Removing unreachable block (ram,0x003b0e04) */
/* WARNING: Removing unreachable block (ram,0x003b0e5c) */
/* WARNING: Removing unreachable block (ram,0x003b0e6c) */
/* WARNING: Removing unreachable block (ram,0x003b0e8c) */
/* WARNING: Removing unreachable block (ram,0x003b0e98) */
/* WARNING: Removing unreachable block (ram,0x003b0ea8) */
/* WARNING: Removing unreachable block (ram,0x003b0ecc) */
/* WARNING: Removing unreachable block (ram,0x003b0ed8) */
/* WARNING: Removing unreachable block (ram,0x003b0ef8) */
/* WARNING: Removing unreachable block (ram,0x003b1058) */
/* WARNING: Removing unreachable block (ram,0x003b106c) */
/* WARNING: Removing unreachable block (ram,0x003b1070) */
/* WARNING: Removing unreachable block (ram,0x003b108c) */
/* WARNING: Removing unreachable block (ram,0x003b10e8) */
/* WARNING: Removing unreachable block (ram,0x003b10fc) */
/* WARNING: Removing unreachable block (ram,0x003b1120) */
/* WARNING: Removing unreachable block (ram,0x003b1128) */
/* WARNING: Removing unreachable block (ram,0x003b113c) */
/* WARNING: Removing unreachable block (ram,0x003b1160) */
/* WARNING: Removing unreachable block (ram,0x003b1168) */
/* WARNING: Removing unreachable block (ram,0x003b116c) */
/* WARNING: Removing unreachable block (ram,0x003b1188) */

void FUN_003b0b80(int *param_1,unsigned long long param_2,unsigned long long param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  float *pfVar5;
  float *pfVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  
  pfVar6 = (float *)*param_1;
  iVar1 = *(int *)(param_5 + 0x10);
  iVar2 = param_1[1];
  pfVar5 = *(float **)(param_5 + 0xc);
  if (iVar1 == iVar2 << 2) {
    FUN_00421a60(0);
    if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x100;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar3 = getCopCondition(0,0);
      } while (cVar3 == '\0');
    }
    if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x200;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar3 = getCopCondition(0,0);
      } while (cVar3 == '\0');
      do {
      } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
    }
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    if (0x7e < param_4) {
      if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar3 = getCopCondition(0,0);
        } while (cVar3 == '\0');
      }
      if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar3 = getCopCondition(0,0);
        } while (cVar3 == '\0');
        do {
        } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
      }
      SYNC(0);
      SYNC(0x10);
      SYNC(0);
      SYNC(0x10);
      do {
        cVar3 = getCopCondition(0,0);
      } while (cVar3 == '\0');
      do {
      } while( 1 );
    }
    if (param_4 != 0) {
      if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar3 = getCopCondition(0,0);
        } while (cVar3 == '\0');
      }
      if ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0) {
        SYNC(0);
        SYNC(0x10);
        do {
          cVar3 = getCopCondition(0,0);
        } while (cVar3 == '\0');
        do {
        } while ((REG_DMAC_9_SPR_TO_CHCR & 0x100) != 0);
      }
      SYNC(0);
      SYNC(0x10);
      SYNC(0);
      SYNC(0x10);
      do {
        cVar3 = getCopCondition(0,0);
      } while (cVar3 == '\0');
      do {
      } while( 1 );
    }
  }
  else {
    do {
      DI();
      SYNC(0x10);
    } while ((Status & 0x10000) != 0);
    EI();
    iVar4 = 0;
    if (0 < param_4) {
      do {
        do {
          DI();
          SYNC(0x10);
        } while ((Status & 0x10000) != 0);
        fVar7 = pfVar5[4];
        fVar8 = pfVar5[5];
        fVar9 = pfVar5[6];
        fVar10 = pfVar5[7];
        fVar11 = pfVar5[0xc] + fVar7 * 0.5f + *pfVar5 * 0.5f;
        fVar12 = pfVar5[0xd] + fVar8 * 0.5f + pfVar5[1] * 0.5f;
        fVar13 = pfVar5[0xe] + fVar9 * 0.5f + pfVar5[2] * 0.5f;
        fVar14 = pfVar5[0xf] + fVar10 * 0.5f + pfVar5[3] * 0.5f;
        fVar15 = fVar11 - *pfVar5;
        fVar16 = fVar12 - pfVar5[1];
        fVar17 = fVar13 - pfVar5[2];
        fVar18 = fVar14 - pfVar5[3];
        pfVar6[0xc] = fVar11;
        pfVar6[0xd] = fVar12;
        pfVar6[0xe] = fVar13;
        pfVar6[0xf] = fVar14;
        pfVar6[8] = fVar15;
        pfVar6[9] = fVar16;
        pfVar6[10] = fVar17;
        pfVar6[0xb] = fVar18;
        *pfVar6 = fVar11 - fVar7;
        pfVar6[1] = fVar12 - fVar8;
        pfVar6[2] = fVar13 - fVar9;
        pfVar6[3] = fVar14 - fVar10;
        pfVar6[4] = fVar15 - fVar7;
        pfVar6[5] = fVar16 - fVar8;
        pfVar6[6] = fVar17 - fVar9;
        pfVar6[7] = fVar18 - fVar10;
        pfVar5 = (float *)((int)pfVar5 + iVar1);
        pfVar6 = pfVar6 + iVar2;
        EI();
        iVar4 = iVar4 + 1;
      } while (iVar4 < param_4);
    }
  }
  return;
}
