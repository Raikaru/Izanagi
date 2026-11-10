/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003aa9f0 rpPTankSkyInsPosCCCSNR src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003aa9f0_arg4 fn_003aa9f0_arg4;
struct fn_003aa9f0_arg4 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x5c];
    float field_0x68; /* +0x68 */
    float field_0x6c; /* +0x6c */
    float field_0x70; /* +0x70 */
    float field_0x74; /* +0x74 */
};


/* WARNING: Removing unreachable block (ram,0x003aaca0) */
/* WARNING: Removing unreachable block (ram,0x003aacb0) */
/* WARNING: Removing unreachable block (ram,0x003aad14) */
/* WARNING: Removing unreachable block (ram,0x003aad24) */
/* WARNING: Removing unreachable block (ram,0x003aad44) */
/* WARNING: Removing unreachable block (ram,0x003aad50) */
/* WARNING: Removing unreachable block (ram,0x003aad60) */
/* WARNING: Removing unreachable block (ram,0x003aad84) */
/* WARNING: Removing unreachable block (ram,0x003aad90) */
/* WARNING: Removing unreachable block (ram,0x003aadb0) */
/* WARNING: Removing unreachable block (ram,0x003aaf08) */
/* WARNING: Removing unreachable block (ram,0x003aaf1c) */
/* WARNING: Removing unreachable block (ram,0x003aaf28) */
/* WARNING: Removing unreachable block (ram,0x003aaf90) */
/* WARNING: Removing unreachable block (ram,0x003aafa4) */
/* WARNING: Removing unreachable block (ram,0x003aafc8) */
/* WARNING: Removing unreachable block (ram,0x003aafd0) */
/* WARNING: Removing unreachable block (ram,0x003aafe4) */
/* WARNING: Removing unreachable block (ram,0x003ab008) */
/* WARNING: Removing unreachable block (ram,0x003ab010) */
/* WARNING: Removing unreachable block (ram,0x003ab014) */
/* WARNING: Removing unreachable block (ram,0x003ab030) */

void FUN_003aa9f0(int *param_1,float *param_2,float *param_3,int param_4,fn_003aa9f0_arg4 *param_5)

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
  float in_vf4w;
  float in_vf5w;
  float in_vf6w;
  float in_vf7w;
  
  iVar1 = param_5->field_0x8;
  iVar2 = param_1[1];
  pfVar6 = (float *)param_5->field_0x4;
  pfVar5 = (float *)*param_1;
  fVar7 = param_5->field_0x70 * 0.5f;
  fVar16 = param_5->field_0x74 * 0.5f;
  fVar9 = *param_2 * fVar7;
  fVar10 = param_2[1] * fVar7;
  fVar7 = param_2[2] * fVar7;
  fVar11 = *param_3 * fVar16;
  fVar8 = param_5->field_0x6c;
  fVar15 = param_5->field_0x68;
  fVar12 = param_3[1] * fVar16;
  fVar16 = param_3[2] * fVar16;
  fVar13 = fVar15 * *param_2 + fVar8 * *param_3;
  fVar14 = fVar15 * param_2[1] + fVar8 * param_3[1];
  fVar8 = fVar15 * param_2[2] + fVar8 * param_3[2];
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
        fVar15 = *pfVar6;
        fVar17 = pfVar6[1];
        fVar18 = pfVar6[2];
        *pfVar5 = ((fVar15 - fVar9) - fVar11) + fVar13;
        pfVar5[1] = ((fVar17 - fVar10) - fVar12) + fVar14;
        pfVar5[2] = ((fVar18 - fVar7) - fVar16) + fVar8;
        pfVar5[3] = in_vf4w;
        pfVar5[4] = ((fVar15 + fVar9) - fVar11) + fVar13;
        pfVar5[5] = ((fVar17 + fVar10) - fVar12) + fVar14;
        pfVar5[6] = ((fVar18 + fVar7) - fVar16) + fVar8;
        pfVar5[7] = in_vf5w;
        pfVar5[8] = fVar15 + fVar9 + fVar11 + fVar13;
        pfVar5[9] = fVar17 + fVar10 + fVar12 + fVar14;
        pfVar5[10] = fVar18 + fVar7 + fVar16 + fVar8;
        pfVar5[0xb] = in_vf6w;
        pfVar5[0xc] = (fVar15 - fVar9) + fVar11 + fVar13;
        pfVar5[0xd] = (fVar17 - fVar10) + fVar12 + fVar14;
        pfVar5[0xe] = (fVar18 - fVar7) + fVar16 + fVar8;
        pfVar5[0xf] = in_vf7w;
        iVar4 = iVar4 + 1;
        pfVar5 = pfVar5 + iVar2;
        pfVar6 = (float *)((int)pfVar6 + iVar1);
      } while (iVar4 < param_4);
    }
  }
  return;
}
