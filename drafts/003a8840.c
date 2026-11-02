/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a8840 func_003a8840 src/promoted/code1_003a.c - romwright draft, unreviewed */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

unsigned long long FUN_003a8840(unsigned long long param_1,int *param_2)

{
  int iVar1;
  unsigned char bVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int *piVar7;
  unsigned char auVar8 [16];
  unsigned char auVar9 [16];
  unsigned char auVar10 [16];
  unsigned char auVar11 [16];
  unsigned char auVar12 [16];
  unsigned char auVar13 [16];
  unsigned char auVar14 [16];
  unsigned int uVar15;
  unsigned int uVar16;
  int iVar17;
  unsigned int uVar18;
  long lVar19;
  unsigned long long uVar20;
  unsigned long long extraout_v0_udw;
  unsigned char auVar21 [16];
  unsigned char auVar22 [16];
  unsigned char auVar23 [16];
  unsigned char auVar24 [16];
  unsigned long long extraout_v0_udw_00;
  unsigned char auVar25 [16];
  unsigned int *puVar26;
  unsigned long long in_v1_udw;
  unsigned char auVar27 [16];
  unsigned char auVar28 [16];
  unsigned char auVar29 [16];
  int iVar30;
  unsigned long long uVar31;
  unsigned long long in_a0_udw;
  unsigned char auVar33 [16];
  unsigned char auVar34 [16];
  unsigned char auVar35 [16];
  unsigned char auVar36 [16];
  unsigned char auVar37 [16];
  unsigned int in_a1_udw;
  unsigned int in_a1_udw_00;
  unsigned int *puVar38;
  unsigned char auVar39 [16];
  unsigned char auVar40 [16];
  unsigned long long in_a3_udw;
  unsigned long long in_t0_udw;
  unsigned int uVar41;
  unsigned int uVar42;
  unsigned long long in_t3_udw;
  unsigned int uVar43;
  float fVar44;
  float fVar45;
  float fVar46;
  unsigned int auStack_40 [16];
  unsigned char auVar32 [16];
  
  iVar17 = DAT_008872e0;
  iVar3 = *param_2;
  iVar4 = *(int *)(DAT_008872e0 + 0x60);
  iVar5 = *(int *)(iVar3 + DAT_00764700);
  iVar6 = *(int *)(iVar5 + 0x9c);
  iVar30 = *(int *)(*(int *)(iVar6 + 0xfc) + 0x10);
  iVar1 = (*(int *)(iVar5 + 4) - 1) / iVar30;
  uVar43 = *(int *)(iVar5 + 4) - iVar1 * iVar30;
  if (*(int *)(iVar6 + 0xdc) != 0) {
    puVar26 = *(unsigned int **)(*(int *)(iVar6 + 0xe4) * 0x20 + iVar6 + 0x18);
    *puVar26 = 0x10000000;
    puVar26[2] = 0;
    puVar26 = (unsigned int *)
              (*(int *)(*(int *)(iVar6 + 0xe4) * 0x20 + iVar6 + 0x1c) +
              (*(int *)(*(int *)(iVar6 + 0xfc) + 0x18) + 1) * iVar1 * 0x10);
    *puVar26 = 0x20000000;
    puVar26[2] = uVar43 | 0x4000000;
    *(unsigned int **)(*(int *)(iVar6 + 0xe4) * 0x20 + iVar6 + 0x18) = puVar26;
  }
  FUN_00421a60(0);
  puVar38 = (unsigned int *)(iVar17 + 0x20);
  puVar26 = auStack_40;
  iVar30 = 8;
  do {
    iVar30 = iVar30 - 1;
    uVar18 = puVar38[1];
    *puVar26 = *puVar38;
    puVar38 = puVar38 + 2;
    puVar26[1] = uVar18;
    puVar26 = puVar26 + 2;
  } while (0 < iVar30);
  FUN_0040a800(auStack_40,0,0);
  piVar7 = (int *)**(int **)(*(int *)(iVar3 + 0x18) + 0x20);
  iVar3 = *piVar7;
  FUN_003a2ad0(iVar5 + 0x50,iVar3);
  FUN_003a29f0(iVar5 + 0x50,iVar3);
  lVar19 = FUN_003f3eb0(0,0x12);
  if (lVar19 != 0) {
    auVar27._8_8_ = in_v1_udw;
    auVar27._0_8_ = 0x6c0503eb01000101;
    auVar21._8_8_ = extraout_v0_udw;
    auVar21._0_8_ = 0x1000000f;
    auVar22 = _pcpyld(auVar27,auVar21);
    *DAT_00764974 = auVar22._0_4_;
    DAT_00764974[1] = auVar22._4_4_;
    DAT_00764974[2] = auVar22._8_4_;
    DAT_00764974[3] = auVar22._12_4_;
    bVar2 = *(unsigned char *)(iVar6 + 0xf5);
    uVar41 = (unsigned int)*(unsigned char *)(iVar6 + 0xf6);
    uVar42 = (unsigned int)*(unsigned char *)(iVar6 + 0xf7);
    auVar39._8_8_ = 0;
    auVar39._0_8_ = in_t3_udw;
    auVar22._8_4_ = in_a1_udw;
    auVar22._0_8_ = 0x6c0203fa01000101;
    auVar22._12_4_ = in_a1_udw_00;
    auVar22 = _pcpyld(auVar22,auVar39 << 0x40);
    DAT_00764974[4] = (unsigned int)*(unsigned char *)(iVar6 + 0xf4);
    DAT_00764974[5] = (unsigned int)bVar2;
    DAT_00764974[6] = uVar41;
    DAT_00764974[7] = uVar42;
    fVar46 = *(float *)(iVar5 + 0xbc);
    fVar44 = *(float *)(iVar5 + 0xb8) * 2.0f;
    fVar45 = fVar46 * 2.0f;
    DAT_00764974[8] = *(float *)(iVar5 + 0xb8);
    DAT_00764974[9] = fVar46;
    DAT_00764974[10] = fVar44;
    DAT_00764974[0xb] = fVar45;
    iVar30 = *(int *)(iVar4 + 0x10);
    fVar46 = *(float *)(iVar17 + 0x74);
    DAT_00764974[0xc] = (float)*(int *)(iVar4 + 0xc) * -0.25f * *(float *)(iVar17 + 0x70);
    DAT_00764974[0xd] = (float)iVar30 * -0.25f * fVar46;
    DAT_00764974[0xe] = iVar1 + 1;
    DAT_00764974[0xf] = uVar43;
    uVar16 = DAT_0088cb9c;
    uVar15 = DAT_0088cb98;
    uVar18 = DAT_0088cb94;
    DAT_00764974[0x10] = DAT_0088cb90;
    DAT_00764974[0x11] = uVar18;
    DAT_00764974[0x12] = uVar15;
    DAT_00764974[0x13] = uVar16;
    auVar23 = _DAT_0088cba0;
    DAT_00764974[0x14] = DAT_0088cba0;
    DAT_00764974[0x15] = auVar23._4_4_;
    DAT_00764974[0x16] = auVar23._8_4_;
    DAT_00764974[0x17] = auVar23._12_4_;
    DAT_00764974[0x18] = auVar22._0_4_;
    DAT_00764974[0x19] = auVar22._4_4_;
    DAT_00764974[0x1a] = auVar22._8_4_;
    DAT_00764974[0x1b] = auVar22._12_4_;
    uVar31 = **(unsigned long long **)(iVar6 + 0xfc);
    if (iVar3 != 0) {
      auVar23._0_8_ = 0x8000000000000;
      uVar31 = uVar31 | 0x8000000000000;
    }
    auVar24._8_8_ = auVar23._8_8_;
    if ((DAT_00763cd8 & 0x20) != 0) {
      uVar31 = uVar31 | 0x10000000000000;
    }
    if (*(int *)(iVar5 + 0xb4) != 0) {
      uVar31 = uVar31 | 0x20000000000000;
    }
    auVar32._8_8_ = in_a0_udw;
    auVar32._0_8_ = uVar31;
    auVar24._0_8_ = (*(unsigned long long **)(iVar6 + 0xfc))[1];
    auVar22 = _pcpyld(auVar24,auVar32);
    DAT_00764974[0x1c] = auVar22._0_4_;
    DAT_00764974[0x1d] = auVar22._4_4_;
    DAT_00764974[0x1e] = auVar22._8_4_;
    DAT_00764974[0x1f] = auVar22._12_4_;
    if (iVar3 == 0) {
      uVar18 = 0x3b808081;
    }
    else {
      uVar18 = 0x3b011b50;
    }
    DAT_00764974 = DAT_00764974 + 0x20;
    FUN_003cc560(uVar18,piVar7 + 1);
    auVar13._8_8_ = 0;
    auVar13._0_8_ = in_a0_udw;
    auVar28._8_8_ = in_v1_udw;
    auVar28._0_8_ = 0x5000000600000000;
    auVar39 = _pcpyld(auVar28,auVar13 << 0x40);
    auVar29._8_8_ = in_v1_udw;
    auVar29._0_8_ = 0x1000000000008005;
    *DAT_00764974 = auVar39._0_4_;
    DAT_00764974[1] = auVar39._4_4_;
    DAT_00764974[2] = auVar39._8_4_;
    DAT_00764974[3] = auVar39._12_4_;
    auVar8._8_4_ = fVar44;
    auVar8._0_8_ = 0xe;
    auVar8._12_4_ = fVar45;
    auVar22 = _pcpyld(auVar8,auVar29);
    DAT_00764974[4] = auVar22._0_4_;
    DAT_00764974[5] = auVar22._4_4_;
    DAT_00764974[6] = auVar22._8_4_;
    DAT_00764974[7] = auVar22._12_4_;
    auVar33._0_8_ = *(unsigned long long *)(iVar6 + 0xb8);
    auVar33._8_8_ = in_a0_udw;
    auVar40._8_8_ = auVar39._8_8_;
    auVar40._0_8_ = 0x15;
    auVar12._8_4_ = uVar41;
    auVar12._0_8_ = 0x43;
    auVar12._12_4_ = uVar42;
    auVar22 = _pcpyld(auVar12,auVar33);
    DAT_00764974[8] = auVar22._0_4_;
    DAT_00764974[9] = auVar22._4_4_;
    DAT_00764974[10] = auVar22._8_4_;
    DAT_00764974[0xb] = auVar22._12_4_;
    auVar34._8_8_ = auVar22._8_8_;
    auVar34._0_8_ = *(unsigned long long *)(iVar6 + 0xc0);
    auVar11._8_8_ = in_t0_udw;
    auVar11._0_8_ = 9;
    auVar22 = _pcpyld(auVar11,auVar34);
    DAT_00764974[0xc] = auVar22._0_4_;
    DAT_00764974[0xd] = auVar22._4_4_;
    DAT_00764974[0xe] = auVar22._8_4_;
    DAT_00764974[0xf] = auVar22._12_4_;
    auVar35._8_8_ = auVar22._8_8_;
    auVar35._0_8_ = *(unsigned long long *)(iVar6 + 200);
    auVar10._8_8_ = in_a3_udw;
    auVar10._0_8_ = 0x48;
    auVar22 = _pcpyld(auVar10,auVar35);
    DAT_00764974[0x10] = auVar22._0_4_;
    DAT_00764974[0x11] = auVar22._4_4_;
    DAT_00764974[0x12] = auVar22._8_4_;
    DAT_00764974[0x13] = auVar22._12_4_;
    auVar36._8_8_ = auVar22._8_8_;
    auVar36._0_8_ = *(unsigned long long *)(iVar6 + 0xd0);
    auVar22 = _pcpyld(auVar40,auVar36);
    DAT_00764974[0x14] = auVar22._0_4_;
    DAT_00764974[0x15] = auVar22._4_4_;
    DAT_00764974[0x16] = auVar22._8_4_;
    DAT_00764974[0x17] = auVar22._12_4_;
    auVar37._8_8_ = auVar22._8_8_;
    auVar37._0_8_ = DAT_007649b0;
    auVar9._8_4_ = fVar44;
    auVar9._0_8_ = 0x4f;
    auVar9._12_4_ = fVar45;
    auVar22 = _pcpyld(auVar9,auVar37);
    DAT_00764974[0x18] = auVar22._0_4_;
    DAT_00764974[0x19] = auVar22._4_4_;
    DAT_00764974[0x1a] = auVar22._8_4_;
    DAT_00764974[0x1b] = auVar22._12_4_;
    uVar43 = *(unsigned int *)(iVar6 + 0xf8);
    puVar26 = DAT_00764974 + 0x1c;
    if (DAT_00764a78 != uVar43) {
      auVar25._0_8_ = ((unsigned long long)uVar43 << 0x20) | 0x50000000;
      auVar25._8_8_ = extraout_v0_udw_00;
      auVar14._4_4_ = fVar45;
      auVar14._0_4_ = fVar44;
      auVar14._8_8_ = 0;
      auVar22 = _pcpyld(auVar14 << 0x40,auVar25);
      DAT_00764a78 = uVar43;
      *puVar26 = auVar22._0_4_;
      DAT_00764974[0x1d] = auVar22._4_4_;
      DAT_00764974[0x1e] = auVar22._8_4_;
      DAT_00764974[0x1f] = auVar22._12_4_;
      puVar26 = DAT_00764974 + 0x20;
    }
    DAT_00764974 = puVar26 + 4;
    puVar38 = (unsigned int *)(iVar6 + *(int *)(iVar6 + 0xe4) * 0x20);
    uVar18 = puVar38[1];
    uVar15 = puVar38[2];
    uVar16 = puVar38[3];
    *puVar26 = *puVar38;
    puVar26[1] = uVar18;
    puVar26[2] = uVar15;
    puVar26[3] = uVar16;
    FUN_0042ba20();
    puVar38[4] = puVar38[4] + 1;
    FUN_0042ba70();
    iVar1 = puVar38[5];
    puVar38[5] = iVar1 + 1;
    if (iVar1 == 0) {
      FUN_003f36a0(puVar38 + 4);
    }
    uVar20 = 1;
  }
  else {
    uVar20 = 0;
  }
  return uVar20;
}
