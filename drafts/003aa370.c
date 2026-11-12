/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003aa370 rpPTankSkyInsPosNCCSNR src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003aa370_arg4 fn_003aa370_arg4;
struct fn_003aa370_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x64];
    float field_0x70; /* +0x70 */
    float field_0x74; /* +0x74 */
};


/* WARNING: Removing unreachable block (ram,0x003aa5d0) */
/* WARNING: Removing unreachable block (ram,0x003aa5e0) */
/* WARNING: Removing unreachable block (ram,0x003aa630) */
/* WARNING: Removing unreachable block (ram,0x003aa640) */
/* WARNING: Removing unreachable block (ram,0x003aa664) */
/* WARNING: Removing unreachable block (ram,0x003aa670) */
/* WARNING: Removing unreachable block (ram,0x003aa680) */
/* WARNING: Removing unreachable block (ram,0x003aa6a4) */
/* WARNING: Removing unreachable block (ram,0x003aa6b0) */
/* WARNING: Removing unreachable block (ram,0x003aa6d0) */
/* WARNING: Removing unreachable block (ram,0x003aa828) */
/* WARNING: Removing unreachable block (ram,0x003aa83c) */
/* WARNING: Removing unreachable block (ram,0x003aa844) */
/* WARNING: Removing unreachable block (ram,0x003aa898) */
/* WARNING: Removing unreachable block (ram,0x003aa8ac) */
/* WARNING: Removing unreachable block (ram,0x003aa8d0) */
/* WARNING: Removing unreachable block (ram,0x003aa8d8) */
/* WARNING: Removing unreachable block (ram,0x003aa8ec) */
/* WARNING: Removing unreachable block (ram,0x003aa910) */
/* WARNING: Removing unreachable block (ram,0x003aa918) */
/* WARNING: Removing unreachable block (ram,0x003aa91c) */
/* WARNING: Removing unreachable block (ram,0x003aa938) */

void FUN_003aa370(int *param_1,float *param_2,float *param_3,int param_4,fn_003aa370_arg4 *param_5)

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
  float in_vf4w;
  float in_vf5w;
  float in_vf6w;
  float in_vf7w;
  
  iVar1 = param_5->field_0x8;
  iVar2 = param_1[1];
  pfVar6 = (float *)param_5->field_0x4;
  pfVar5 = (float *)*param_1;
  fVar12 = param_5->field_0x70 * 0.5f;
  fVar11 = param_5->field_0x74 * 0.5f;
  fVar7 = *param_2 * fVar12;
  fVar8 = param_2[2] * fVar12;
  fVar10 = *param_3 * fVar11;
  fVar12 = param_2[1] * fVar12;
  fVar9 = param_3[1] * fVar11;
  fVar11 = param_3[2] * fVar11;
  if (iVar1 == iVar2) {
    FUN_00421a60(0);
    if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x100;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar3 = getCopCondition(0,0);
      } while (cVar3 == '\0');
    }
    if ((REG_DMAC_8_SPR_FROM_CHCR & 0x100) != 0) {
      REG_DMAC_PCR = 0x100;
      SYNC(0);
      SYNC(0x10);
      do {
        cVar3 = getCopCondition(0,0);
      } while (cVar3 == '\0');
    }
    if (0xcc < param_4) {
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
    iVar4 = 0;
    if (0 < param_4) {
      do {
        fVar13 = *pfVar6;
        fVar14 = pfVar6[1];
        fVar15 = pfVar6[2];
        *pfVar5 = (fVar13 - fVar7) - fVar10;
        pfVar5[1] = (fVar14 - fVar12) - fVar9;
        pfVar5[2] = (fVar15 - fVar8) - fVar11;
        pfVar5[3] = in_vf4w;
        pfVar5[4] = (fVar13 + fVar7) - fVar10;
        pfVar5[5] = (fVar14 + fVar12) - fVar9;
        pfVar5[6] = (fVar15 + fVar8) - fVar11;
        pfVar5[7] = in_vf5w;
        pfVar5[8] = fVar13 + fVar7 + fVar10;
        pfVar5[9] = fVar14 + fVar12 + fVar9;
        pfVar5[10] = fVar15 + fVar8 + fVar11;
        pfVar5[0xb] = in_vf6w;
        pfVar5[0xc] = (fVar13 - fVar7) + fVar10;
        pfVar5[0xd] = (fVar14 - fVar12) + fVar9;
        pfVar5[0xe] = (fVar15 - fVar8) + fVar11;
        pfVar5[0xf] = in_vf7w;
        iVar4 = iVar4 + 1;
        pfVar5 = pfVar5 + iVar2;
        pfVar6 = (float *)((int)pfVar6 + iVar1);
      } while (iVar4 < param_4);
    }
  }
  return;
}
