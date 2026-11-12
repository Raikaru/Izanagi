/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b12a0 rpPTankSkyInsPosCCPM src/promoted/code1_003b.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x003b1520) */
/* WARNING: Removing unreachable block (ram,0x003b1530) */
/* WARNING: Removing unreachable block (ram,0x003b154c) */
/* WARNING: Removing unreachable block (ram,0x003b15a4) */
/* WARNING: Removing unreachable block (ram,0x003b15b4) */
/* WARNING: Removing unreachable block (ram,0x003b15d4) */
/* WARNING: Removing unreachable block (ram,0x003b15e0) */
/* WARNING: Removing unreachable block (ram,0x003b15f0) */
/* WARNING: Removing unreachable block (ram,0x003b1614) */
/* WARNING: Removing unreachable block (ram,0x003b1620) */
/* WARNING: Removing unreachable block (ram,0x003b1640) */
/* WARNING: Removing unreachable block (ram,0x003b17a0) */
/* WARNING: Removing unreachable block (ram,0x003b17b4) */
/* WARNING: Removing unreachable block (ram,0x003b17b8) */
/* WARNING: Removing unreachable block (ram,0x003b17d4) */
/* WARNING: Removing unreachable block (ram,0x003b1830) */
/* WARNING: Removing unreachable block (ram,0x003b1844) */
/* WARNING: Removing unreachable block (ram,0x003b1868) */
/* WARNING: Removing unreachable block (ram,0x003b1870) */
/* WARNING: Removing unreachable block (ram,0x003b1884) */
/* WARNING: Removing unreachable block (ram,0x003b18a8) */
/* WARNING: Removing unreachable block (ram,0x003b18b0) */
/* WARNING: Removing unreachable block (ram,0x003b18b4) */
/* WARNING: Removing unreachable block (ram,0x003b18d0) */

void FUN_003b12a0(int *param_1,unsigned long long param_2,unsigned long long param_3,int param_4,int param_5)

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
  float fVar19;
  float fVar20;
  
  iVar1 = param_1[1];
  iVar2 = *(int *)(param_5 + 0x10);
  pfVar6 = (float *)*param_1;
  pfVar5 = *(float **)(param_5 + 0xc);
  if (iVar2 == iVar1 << 2) {
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
    uRam70003fb0 = *(unsigned int *)(param_5 + 0x68);
    uRam70003fb4 = *(unsigned int *)(param_5 + 0x6c);
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
    fVar11 = *(float *)(param_5 + 0x68) + 0.5f;
    fVar12 = *(float *)(param_5 + 0x6c) + 0.5f;
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
        fVar13 = pfVar5[0xc] + fVar7 * fVar12 + *pfVar5 * fVar11;
        fVar14 = pfVar5[0xd] + fVar8 * fVar12 + pfVar5[1] * fVar11;
        fVar15 = pfVar5[0xe] + fVar9 * fVar12 + pfVar5[2] * fVar11;
        fVar16 = pfVar5[0xf] + fVar10 * fVar12 + pfVar5[3] * fVar11;
        fVar17 = fVar13 - *pfVar5;
        fVar18 = fVar14 - pfVar5[1];
        fVar19 = fVar15 - pfVar5[2];
        fVar20 = fVar16 - pfVar5[3];
        pfVar6[0xc] = fVar13;
        pfVar6[0xd] = fVar14;
        pfVar6[0xe] = fVar15;
        pfVar6[0xf] = fVar16;
        pfVar6[8] = fVar17;
        pfVar6[9] = fVar18;
        pfVar6[10] = fVar19;
        pfVar6[0xb] = fVar20;
        *pfVar6 = fVar13 - fVar7;
        pfVar6[1] = fVar14 - fVar8;
        pfVar6[2] = fVar15 - fVar9;
        pfVar6[3] = fVar16 - fVar10;
        pfVar6[4] = fVar17 - fVar7;
        pfVar6[5] = fVar18 - fVar8;
        pfVar6[6] = fVar19 - fVar9;
        pfVar6[7] = fVar20 - fVar10;
        pfVar5 = (float *)((int)pfVar5 + iVar2);
        pfVar6 = pfVar6 + iVar1;
        EI();
        iVar4 = iVar4 + 1;
      } while (iVar4 < param_4);
    }
  }
  return;
}
