/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b1a10 rpPTankSkyInsColorPP src/promoted/code1_003b.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x003b30e4) */
/* WARNING: Removing unreachable block (ram,0x003b2f74) */
/* WARNING: Removing unreachable block (ram,0x003b1e08) */
/* WARNING: Removing unreachable block (ram,0x003b1e18) */
/* WARNING: Removing unreachable block (ram,0x003b222c) */
/* WARNING: Removing unreachable block (ram,0x003b2f80) */
/* WARNING: Removing unreachable block (ram,0x003b2fa4) */
/* WARNING: Removing unreachable block (ram,0x003b2fac) */
/* WARNING: Removing unreachable block (ram,0x003b2240) */
/* WARNING: Removing unreachable block (ram,0x003b2fb8) */
/* WARNING: Removing unreachable block (ram,0x003b2fdc) */
/* WARNING: Removing unreachable block (ram,0x003b2fe8) */
/* WARNING: Removing unreachable block (ram,0x003b3004) */
/* WARNING: Removing unreachable block (ram,0x003b2250) */
/* WARNING: Removing unreachable block (ram,0x003b235c) */
/* WARNING: Removing unreachable block (ram,0x003b2360) */
/* WARNING: Removing unreachable block (ram,0x003b2370) */
/* WARNING: Removing unreachable block (ram,0x003b237c) */
/* WARNING: Removing unreachable block (ram,0x003b2388) */
/* WARNING: Removing unreachable block (ram,0x003b239c) */
/* WARNING: Removing unreachable block (ram,0x003b23a0) */
/* WARNING: Removing unreachable block (ram,0x003b23a8) */
/* WARNING: Removing unreachable block (ram,0x003b27c0) */
/* WARNING: Removing unreachable block (ram,0x003b27cc) */
/* WARNING: Removing unreachable block (ram,0x003b2860) */
/* WARNING: Removing unreachable block (ram,0x003b30ec) */
/* WARNING: Removing unreachable block (ram,0x003b3110) */
/* WARNING: Removing unreachable block (ram,0x003b3118) */
/* WARNING: Removing unreachable block (ram,0x003b2874) */
/* WARNING: Removing unreachable block (ram,0x003b2878) */
/* WARNING: Removing unreachable block (ram,0x003b3120) */
/* WARNING: Removing unreachable block (ram,0x003b3144) */
/* WARNING: Removing unreachable block (ram,0x003b314c) */
/* WARNING: Removing unreachable block (ram,0x003b3150) */
/* WARNING: Removing unreachable block (ram,0x003b316c) */
/* WARNING: Removing unreachable block (ram,0x003b2888) */
/* WARNING: Removing unreachable block (ram,0x003b288c) */
/* WARNING: Removing unreachable block (ram,0x003b28c8) */
/* WARNING: Removing unreachable block (ram,0x003b28c0) */
/* WARNING: Removing unreachable block (ram,0x003b28f4) */

void FUN_003b1a10(unsigned int *param_1,int param_2,int param_3)

{
  unsigned char uVar1;
  unsigned char uVar2;
  unsigned char uVar3;
  int iVar4;
  char cVar5;
  unsigned char bVar6;
  unsigned int *puVar7;
  unsigned int *puVar8;
  int iVar9;
  unsigned int *puVar10;
  unsigned int *puVar11;
  unsigned int *puVar12;
  unsigned int *puVar13;
  unsigned int *puVar14;
  unsigned int *puVar15;
  unsigned int *puVar16;
  unsigned int *puVar17;
  unsigned int *puVar18;
  unsigned int *puVar19;
  unsigned int *puVar20;
  unsigned int *puVar21;
  unsigned int *puVar22;
  unsigned int *puVar23;
  unsigned int *puVar24;
  unsigned int *puVar25;
  unsigned int *puVar26;
  unsigned int *puVar27;
  unsigned int *puVar28;
  unsigned int *puVar29;
  unsigned int *puVar30;
  unsigned int *puVar31;
  unsigned int *puVar32;
  unsigned int *puVar33;
  unsigned int *puVar34;
  unsigned int *puVar35;
  unsigned int *puVar36;
  unsigned int *puVar37;
  unsigned int *puVar38;
  unsigned int *puVar39;
  unsigned int *puVar40;
  int iVar41;
  unsigned int *puVar42;
  unsigned int *puVar43;
  unsigned int *puVar44;
  
  iVar9 = param_1[1];
  iVar4 = *(int *)(param_3 + 0x28);
  param_1 = (unsigned int *)*param_1;
  puVar37 = *(unsigned int **)(param_3 + 0x24);
  if (iVar4 != iVar9) {
    iVar41 = 0;
    if (0 < param_2) {
      if (8 < param_2) {
        bVar6 = 0;
        if ((-1 < param_2) && (param_2 < 0x7fffffff)) {
          bVar6 = 1;
        }
        if (bVar6) {
          do {
            puVar12 = (unsigned int *)((int)param_1 + iVar9 + iVar9);
            puVar39 = (unsigned int *)((int)puVar37 + iVar4);
            puVar7 = (unsigned int *)((int)puVar12 + iVar9);
            puVar20 = (unsigned int *)((int)puVar39 + iVar4);
            puVar40 = (unsigned int *)((int)puVar7 + iVar9);
            puVar17 = (unsigned int *)((int)puVar20 + iVar4);
            puVar38 = (unsigned int *)((int)puVar40 + iVar9);
            puVar15 = (unsigned int *)((int)puVar17 + iVar4);
            puVar43 = (unsigned int *)((int)puVar38 + iVar9);
            puVar14 = (unsigned int *)((int)puVar15 + iVar4);
            puVar42 = (unsigned int *)((int)puVar43 + iVar9);
            *param_1 = *puVar37;
            puVar21 = (unsigned int *)((int)puVar42 + iVar9);
            puVar19 = (unsigned int *)((int)puVar21 + iVar9);
            puVar11 = (unsigned int *)((int)puVar14 + iVar4);
            puVar18 = (unsigned int *)((int)puVar19 + iVar9);
            iVar41 = iVar41 + 8;
            puVar22 = (unsigned int *)((int)puVar18 + iVar9);
            puVar10 = (unsigned int *)((int)puVar11 + iVar4);
            puVar23 = (unsigned int *)((int)puVar22 + iVar9);
            *(unsigned int *)((int)param_1 + iVar9) = *puVar37;
            *puVar12 = *puVar37;
            puVar24 = (unsigned int *)((int)puVar23 + iVar9);
            puVar16 = (unsigned int *)((int)puVar24 + iVar9);
            puVar25 = (unsigned int *)((int)puVar16 + iVar9);
            puVar26 = (unsigned int *)((int)puVar25 + iVar9);
            puVar27 = (unsigned int *)((int)puVar26 + iVar9);
            puVar28 = (unsigned int *)((int)puVar27 + iVar9);
            puVar29 = (unsigned int *)((int)puVar28 + iVar9);
            puVar30 = (unsigned int *)((int)puVar29 + iVar9);
            puVar13 = (unsigned int *)((int)puVar30 + iVar9);
            puVar31 = (unsigned int *)((int)puVar13 + iVar9);
            *puVar7 = *puVar37;
            puVar32 = (unsigned int *)((int)puVar31 + iVar9);
            puVar33 = (unsigned int *)((int)puVar32 + iVar9);
            puVar34 = (unsigned int *)((int)puVar33 + iVar9);
            puVar35 = (unsigned int *)((int)puVar34 + iVar9);
            puVar36 = (unsigned int *)((int)puVar35 + iVar9);
            puVar44 = (unsigned int *)((int)puVar36 + iVar9);
            puVar37 = (unsigned int *)((int)puVar10 + iVar4);
            puVar7 = (unsigned int *)((int)puVar44 + iVar9);
            puVar12 = (unsigned int *)((int)puVar7 + iVar9);
            puVar8 = (unsigned int *)((int)puVar12 + iVar9);
            param_1 = (unsigned int *)((int)puVar8 + iVar9);
            *puVar40 = *puVar39;
            *puVar38 = *puVar39;
            *puVar43 = *puVar39;
            *puVar42 = *puVar39;
            *puVar21 = *puVar20;
            *puVar19 = *puVar20;
            *puVar18 = *puVar20;
            *puVar22 = *puVar20;
            *puVar23 = *puVar17;
            *puVar24 = *puVar17;
            *puVar16 = *puVar17;
            *puVar25 = *puVar17;
            *puVar26 = *puVar15;
            *puVar27 = *puVar15;
            *puVar28 = *puVar15;
            *puVar29 = *puVar15;
            *puVar30 = *puVar14;
            *puVar13 = *puVar14;
            *puVar31 = *puVar14;
            *puVar32 = *puVar14;
            *puVar33 = *puVar11;
            *puVar34 = *puVar11;
            *puVar35 = *puVar11;
            *puVar36 = *puVar11;
            *puVar44 = *puVar10;
            *puVar7 = *puVar10;
            *puVar12 = *puVar10;
            *puVar8 = *puVar10;
          } while (iVar41 < param_2 - 8);
        }
      }
      for (; iVar41 < param_2; iVar41 = iVar41 + 1) {
        puVar8 = (unsigned int *)((int)param_1 + iVar9);
        puVar12 = (unsigned int *)((int)puVar8 + iVar9);
        puVar7 = (unsigned int *)((int)puVar12 + iVar9);
        *param_1 = *puVar37;
        param_1 = (unsigned int *)((int)puVar7 + iVar9);
        *puVar8 = *puVar37;
        *puVar12 = *puVar37;
        *puVar7 = *puVar37;
        puVar37 = (unsigned int *)((int)puVar37 + iVar4);
      }
    }
  }
  else {
    FUN_00421a60(0);
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
    if (0x330 < param_2) {
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
    if (3 < param_2) {
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
    if ((param_2 != 0) && (iVar9 = 0, 0 < param_2)) {
      if (8 < param_2) {
        bVar6 = 0;
        if ((-1 < param_2) && (param_2 < 0x7fffffff)) {
          bVar6 = 1;
        }
        if (bVar6) {
          do {
            iVar9 = iVar9 + 8;
            uVar1 = *(unsigned char *)((int)puVar37 + 1);
            uVar2 = *(unsigned char *)((int)puVar37 + 2);
            uVar3 = *(unsigned char *)((int)puVar37 + 3);
            *(unsigned char *)param_1 = *(unsigned char *)puVar37;
            *(unsigned char *)((int)param_1 + 1) = uVar1;
            *(unsigned char *)((int)param_1 + 2) = uVar2;
            *(unsigned char *)((int)param_1 + 3) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 1);
            uVar2 = *(unsigned char *)((int)puVar37 + 2);
            uVar3 = *(unsigned char *)((int)puVar37 + 3);
            *(unsigned char *)(param_1 + 1) = *(unsigned char *)puVar37;
            *(unsigned char *)((int)param_1 + 5) = uVar1;
            *(unsigned char *)((int)param_1 + 6) = uVar2;
            *(unsigned char *)((int)param_1 + 7) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 1);
            uVar2 = *(unsigned char *)((int)puVar37 + 2);
            uVar3 = *(unsigned char *)((int)puVar37 + 3);
            *(unsigned char *)(param_1 + 2) = *(unsigned char *)puVar37;
            *(unsigned char *)((int)param_1 + 9) = uVar1;
            *(unsigned char *)((int)param_1 + 10) = uVar2;
            *(unsigned char *)((int)param_1 + 0xb) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 1);
            uVar2 = *(unsigned char *)((int)puVar37 + 2);
            uVar3 = *(unsigned char *)((int)puVar37 + 3);
            *(unsigned char *)(param_1 + 3) = *(unsigned char *)puVar37;
            *(unsigned char *)((int)param_1 + 0xd) = uVar1;
            *(unsigned char *)((int)param_1 + 0xe) = uVar2;
            *(unsigned char *)((int)param_1 + 0xf) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 5);
            uVar2 = *(unsigned char *)((int)puVar37 + 6);
            uVar3 = *(unsigned char *)((int)puVar37 + 7);
            *(unsigned char *)(param_1 + 4) = *(unsigned char *)(puVar37 + 1);
            *(unsigned char *)((int)param_1 + 0x11) = uVar1;
            *(unsigned char *)((int)param_1 + 0x12) = uVar2;
            *(unsigned char *)((int)param_1 + 0x13) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 5);
            uVar2 = *(unsigned char *)((int)puVar37 + 6);
            uVar3 = *(unsigned char *)((int)puVar37 + 7);
            *(unsigned char *)(param_1 + 5) = *(unsigned char *)(puVar37 + 1);
            *(unsigned char *)((int)param_1 + 0x15) = uVar1;
            *(unsigned char *)((int)param_1 + 0x16) = uVar2;
            *(unsigned char *)((int)param_1 + 0x17) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 5);
            uVar2 = *(unsigned char *)((int)puVar37 + 6);
            uVar3 = *(unsigned char *)((int)puVar37 + 7);
            *(unsigned char *)(param_1 + 6) = *(unsigned char *)(puVar37 + 1);
            *(unsigned char *)((int)param_1 + 0x19) = uVar1;
            *(unsigned char *)((int)param_1 + 0x1a) = uVar2;
            *(unsigned char *)((int)param_1 + 0x1b) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 5);
            uVar2 = *(unsigned char *)((int)puVar37 + 6);
            uVar3 = *(unsigned char *)((int)puVar37 + 7);
            *(unsigned char *)(param_1 + 7) = *(unsigned char *)(puVar37 + 1);
            *(unsigned char *)((int)param_1 + 0x1d) = uVar1;
            *(unsigned char *)((int)param_1 + 0x1e) = uVar2;
            *(unsigned char *)((int)param_1 + 0x1f) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 9);
            uVar2 = *(unsigned char *)((int)puVar37 + 10);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xb);
            *(unsigned char *)(param_1 + 8) = *(unsigned char *)(puVar37 + 2);
            *(unsigned char *)((int)param_1 + 0x21) = uVar1;
            *(unsigned char *)((int)param_1 + 0x22) = uVar2;
            *(unsigned char *)((int)param_1 + 0x23) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 9);
            uVar2 = *(unsigned char *)((int)puVar37 + 10);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xb);
            *(unsigned char *)(param_1 + 9) = *(unsigned char *)(puVar37 + 2);
            *(unsigned char *)((int)param_1 + 0x25) = uVar1;
            *(unsigned char *)((int)param_1 + 0x26) = uVar2;
            *(unsigned char *)((int)param_1 + 0x27) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 9);
            uVar2 = *(unsigned char *)((int)puVar37 + 10);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xb);
            *(unsigned char *)(param_1 + 10) = *(unsigned char *)(puVar37 + 2);
            *(unsigned char *)((int)param_1 + 0x29) = uVar1;
            *(unsigned char *)((int)param_1 + 0x2a) = uVar2;
            *(unsigned char *)((int)param_1 + 0x2b) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 9);
            uVar2 = *(unsigned char *)((int)puVar37 + 10);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xb);
            *(unsigned char *)(param_1 + 0xb) = *(unsigned char *)(puVar37 + 2);
            *(unsigned char *)((int)param_1 + 0x2d) = uVar1;
            *(unsigned char *)((int)param_1 + 0x2e) = uVar2;
            *(unsigned char *)((int)param_1 + 0x2f) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0xd);
            uVar2 = *(unsigned char *)((int)puVar37 + 0xe);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xf);
            *(unsigned char *)(param_1 + 0xc) = *(unsigned char *)(puVar37 + 3);
            *(unsigned char *)((int)param_1 + 0x31) = uVar1;
            *(unsigned char *)((int)param_1 + 0x32) = uVar2;
            *(unsigned char *)((int)param_1 + 0x33) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0xd);
            uVar2 = *(unsigned char *)((int)puVar37 + 0xe);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xf);
            *(unsigned char *)(param_1 + 0xd) = *(unsigned char *)(puVar37 + 3);
            *(unsigned char *)((int)param_1 + 0x35) = uVar1;
            *(unsigned char *)((int)param_1 + 0x36) = uVar2;
            *(unsigned char *)((int)param_1 + 0x37) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0xd);
            uVar2 = *(unsigned char *)((int)puVar37 + 0xe);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xf);
            *(unsigned char *)(param_1 + 0xe) = *(unsigned char *)(puVar37 + 3);
            *(unsigned char *)((int)param_1 + 0x39) = uVar1;
            *(unsigned char *)((int)param_1 + 0x3a) = uVar2;
            *(unsigned char *)((int)param_1 + 0x3b) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0xd);
            uVar2 = *(unsigned char *)((int)puVar37 + 0xe);
            uVar3 = *(unsigned char *)((int)puVar37 + 0xf);
            *(unsigned char *)(param_1 + 0xf) = *(unsigned char *)(puVar37 + 3);
            *(unsigned char *)((int)param_1 + 0x3d) = uVar1;
            *(unsigned char *)((int)param_1 + 0x3e) = uVar2;
            *(unsigned char *)((int)param_1 + 0x3f) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x11);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x12);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x13);
            *(unsigned char *)(param_1 + 0x10) = *(unsigned char *)(puVar37 + 4);
            *(unsigned char *)((int)param_1 + 0x41) = uVar1;
            *(unsigned char *)((int)param_1 + 0x42) = uVar2;
            *(unsigned char *)((int)param_1 + 0x43) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x11);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x12);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x13);
            *(unsigned char *)(param_1 + 0x11) = *(unsigned char *)(puVar37 + 4);
            *(unsigned char *)((int)param_1 + 0x45) = uVar1;
            *(unsigned char *)((int)param_1 + 0x46) = uVar2;
            *(unsigned char *)((int)param_1 + 0x47) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x11);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x12);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x13);
            *(unsigned char *)(param_1 + 0x12) = *(unsigned char *)(puVar37 + 4);
            *(unsigned char *)((int)param_1 + 0x49) = uVar1;
            *(unsigned char *)((int)param_1 + 0x4a) = uVar2;
            *(unsigned char *)((int)param_1 + 0x4b) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x11);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x12);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x13);
            *(unsigned char *)(param_1 + 0x13) = *(unsigned char *)(puVar37 + 4);
            *(unsigned char *)((int)param_1 + 0x4d) = uVar1;
            *(unsigned char *)((int)param_1 + 0x4e) = uVar2;
            *(unsigned char *)((int)param_1 + 0x4f) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x15);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x16);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x17);
            *(unsigned char *)(param_1 + 0x14) = *(unsigned char *)(puVar37 + 5);
            *(unsigned char *)((int)param_1 + 0x51) = uVar1;
            *(unsigned char *)((int)param_1 + 0x52) = uVar2;
            *(unsigned char *)((int)param_1 + 0x53) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x15);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x16);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x17);
            *(unsigned char *)(param_1 + 0x15) = *(unsigned char *)(puVar37 + 5);
            *(unsigned char *)((int)param_1 + 0x55) = uVar1;
            *(unsigned char *)((int)param_1 + 0x56) = uVar2;
            *(unsigned char *)((int)param_1 + 0x57) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x15);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x16);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x17);
            *(unsigned char *)(param_1 + 0x16) = *(unsigned char *)(puVar37 + 5);
            *(unsigned char *)((int)param_1 + 0x59) = uVar1;
            *(unsigned char *)((int)param_1 + 0x5a) = uVar2;
            *(unsigned char *)((int)param_1 + 0x5b) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x15);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x16);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x17);
            *(unsigned char *)(param_1 + 0x17) = *(unsigned char *)(puVar37 + 5);
            *(unsigned char *)((int)param_1 + 0x5d) = uVar1;
            *(unsigned char *)((int)param_1 + 0x5e) = uVar2;
            *(unsigned char *)((int)param_1 + 0x5f) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x19);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1a);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1b);
            *(unsigned char *)(param_1 + 0x18) = *(unsigned char *)(puVar37 + 6);
            *(unsigned char *)((int)param_1 + 0x61) = uVar1;
            *(unsigned char *)((int)param_1 + 0x62) = uVar2;
            *(unsigned char *)((int)param_1 + 99) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x19);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1a);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1b);
            *(unsigned char *)(param_1 + 0x19) = *(unsigned char *)(puVar37 + 6);
            *(unsigned char *)((int)param_1 + 0x65) = uVar1;
            *(unsigned char *)((int)param_1 + 0x66) = uVar2;
            *(unsigned char *)((int)param_1 + 0x67) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x19);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1a);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1b);
            *(unsigned char *)(param_1 + 0x1a) = *(unsigned char *)(puVar37 + 6);
            *(unsigned char *)((int)param_1 + 0x69) = uVar1;
            *(unsigned char *)((int)param_1 + 0x6a) = uVar2;
            *(unsigned char *)((int)param_1 + 0x6b) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x19);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1a);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1b);
            *(unsigned char *)(param_1 + 0x1b) = *(unsigned char *)(puVar37 + 6);
            *(unsigned char *)((int)param_1 + 0x6d) = uVar1;
            *(unsigned char *)((int)param_1 + 0x6e) = uVar2;
            *(unsigned char *)((int)param_1 + 0x6f) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x1d);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1e);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1f);
            *(unsigned char *)(param_1 + 0x1c) = *(unsigned char *)(puVar37 + 7);
            *(unsigned char *)((int)param_1 + 0x71) = uVar1;
            *(unsigned char *)((int)param_1 + 0x72) = uVar2;
            *(unsigned char *)((int)param_1 + 0x73) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x1d);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1e);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1f);
            *(unsigned char *)(param_1 + 0x1d) = *(unsigned char *)(puVar37 + 7);
            *(unsigned char *)((int)param_1 + 0x75) = uVar1;
            *(unsigned char *)((int)param_1 + 0x76) = uVar2;
            *(unsigned char *)((int)param_1 + 0x77) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x1d);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1e);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1f);
            *(unsigned char *)(param_1 + 0x1e) = *(unsigned char *)(puVar37 + 7);
            *(unsigned char *)((int)param_1 + 0x79) = uVar1;
            *(unsigned char *)((int)param_1 + 0x7a) = uVar2;
            *(unsigned char *)((int)param_1 + 0x7b) = uVar3;
            uVar1 = *(unsigned char *)((int)puVar37 + 0x1d);
            uVar2 = *(unsigned char *)((int)puVar37 + 0x1e);
            uVar3 = *(unsigned char *)((int)puVar37 + 0x1f);
            *(unsigned char *)(param_1 + 0x1f) = *(unsigned char *)(puVar37 + 7);
            puVar37 = puVar37 + 8;
            *(unsigned char *)((int)param_1 + 0x7d) = uVar1;
            *(unsigned char *)((int)param_1 + 0x7e) = uVar2;
            *(unsigned char *)((int)param_1 + 0x7f) = uVar3;
            param_1 = param_1 + 0x20;
          } while (iVar9 < param_2 - 8);
        }
      }
      for (; iVar9 < param_2; iVar9 = iVar9 + 1) {
        uVar1 = *(unsigned char *)((int)puVar37 + 1);
        uVar2 = *(unsigned char *)((int)puVar37 + 2);
        uVar3 = *(unsigned char *)((int)puVar37 + 3);
        *(unsigned char *)param_1 = *(unsigned char *)puVar37;
        *(unsigned char *)((int)param_1 + 1) = uVar1;
        *(unsigned char *)((int)param_1 + 2) = uVar2;
        *(unsigned char *)((int)param_1 + 3) = uVar3;
        uVar1 = *(unsigned char *)((int)puVar37 + 1);
        uVar2 = *(unsigned char *)((int)puVar37 + 2);
        uVar3 = *(unsigned char *)((int)puVar37 + 3);
        *(unsigned char *)(param_1 + 1) = *(unsigned char *)puVar37;
        *(unsigned char *)((int)param_1 + 5) = uVar1;
        *(unsigned char *)((int)param_1 + 6) = uVar2;
        *(unsigned char *)((int)param_1 + 7) = uVar3;
        uVar1 = *(unsigned char *)((int)puVar37 + 1);
        uVar2 = *(unsigned char *)((int)puVar37 + 2);
        uVar3 = *(unsigned char *)((int)puVar37 + 3);
        *(unsigned char *)(param_1 + 2) = *(unsigned char *)puVar37;
        *(unsigned char *)((int)param_1 + 9) = uVar1;
        *(unsigned char *)((int)param_1 + 10) = uVar2;
        *(unsigned char *)((int)param_1 + 0xb) = uVar3;
        uVar1 = *(unsigned char *)((int)puVar37 + 1);
        uVar2 = *(unsigned char *)((int)puVar37 + 2);
        uVar3 = *(unsigned char *)((int)puVar37 + 3);
        *(unsigned char *)(param_1 + 3) = *(unsigned char *)puVar37;
        puVar37 = puVar37 + 1;
        *(unsigned char *)((int)param_1 + 0xd) = uVar1;
        *(unsigned char *)((int)param_1 + 0xe) = uVar2;
        *(unsigned char *)((int)param_1 + 0xf) = uVar3;
        param_1 = param_1 + 4;
      }
    }
  }
  return;
}
