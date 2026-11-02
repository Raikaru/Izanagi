/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b3880 rpPTankSkyInsUV2 src/promoted/code1_003b.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x003b3e50) */
/* WARNING: Removing unreachable block (ram,0x003b3ce4) */
/* WARNING: Removing unreachable block (ram,0x003b39e0) */
/* WARNING: Removing unreachable block (ram,0x003b39f0) */
/* WARNING: Removing unreachable block (ram,0x003b3a4c) */
/* WARNING: Removing unreachable block (ram,0x003b3cf0) */
/* WARNING: Removing unreachable block (ram,0x003b3d14) */
/* WARNING: Removing unreachable block (ram,0x003b3d1c) */
/* WARNING: Removing unreachable block (ram,0x003b3a60) */
/* WARNING: Removing unreachable block (ram,0x003b3d28) */
/* WARNING: Removing unreachable block (ram,0x003b3d4c) */
/* WARNING: Removing unreachable block (ram,0x003b3d58) */
/* WARNING: Removing unreachable block (ram,0x003b3d74) */
/* WARNING: Removing unreachable block (ram,0x003b3a70) */
/* WARNING: Removing unreachable block (ram,0x003b3b08) */
/* WARNING: Removing unreachable block (ram,0x003b3b0c) */
/* WARNING: Removing unreachable block (ram,0x003b3e58) */
/* WARNING: Removing unreachable block (ram,0x003b3b1c) */
/* WARNING: Removing unreachable block (ram,0x003b3b78) */
/* WARNING: Removing unreachable block (ram,0x003b3e5c) */
/* WARNING: Removing unreachable block (ram,0x003b3f04) */
/* WARNING: Removing unreachable block (ram,0x003b3f28) */
/* WARNING: Removing unreachable block (ram,0x003b3f30) */
/* WARNING: Removing unreachable block (ram,0x003b3e6c) */
/* WARNING: Removing unreachable block (ram,0x003b3e70) */
/* WARNING: Removing unreachable block (ram,0x003b3e80) */
/* WARNING: Removing unreachable block (ram,0x003b3ea4) */
/* WARNING: Removing unreachable block (ram,0x003b3eac) */
/* WARNING: Removing unreachable block (ram,0x003b3eb0) */
/* WARNING: Removing unreachable block (ram,0x003b3ed0) */

void FUN_003b3880(int *param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  char cVar3;
  unsigned int *puVar4;
  unsigned int *puVar5;
  unsigned int *puVar6;
  int iVar7;
  unsigned int *puVar8;
  unsigned int *puVar9;
  
  iVar1 = param_1[1];
  iVar2 = *(int *)(param_3 + 0x40);
  puVar9 = (unsigned int *)*param_1;
  if (iVar2 != iVar1 << 1) {
    iVar7 = 0;
    puVar8 = *(unsigned int **)(param_3 + 0x3c);
    if (0 < param_2) {
      do {
        puVar6 = (unsigned int *)((int)puVar9 + iVar1);
        puVar5 = (unsigned int *)((int)puVar6 + iVar1);
        iVar7 = iVar7 + 1;
        puVar4 = (unsigned int *)((int)puVar5 + iVar1);
        *puVar9 = *puVar8;
        puVar9[1] = puVar8[3];
        puVar9 = (unsigned int *)((int)puVar4 + iVar1);
        *puVar6 = puVar8[2];
        puVar6[1] = puVar8[3];
        *puVar5 = puVar8[2];
        puVar5[1] = puVar8[1];
        *puVar4 = *puVar8;
        puVar4[1] = puVar8[1];
        puVar8 = (unsigned int *)((int)puVar8 + iVar2);
      } while (iVar7 < param_2);
    }
  }
  else {
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
    if (0x155 < param_2) {
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
    if (param_2 != 0) {
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
  return;
}
