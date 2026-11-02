/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039cb80 func_0039cb80 src/promoted/code1_0039.c - romwright draft, unreviewed */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

unsigned long long FUN_0039cb80(int param_1)

{
  unsigned int *puVar1;
  unsigned int *puVar2;
  int *piVar3;
  unsigned int uVar4;
  unsigned char bVar5;
  unsigned char auVar6 [16];
  unsigned char auVar7 [16];
  unsigned char auVar8 [16];
  unsigned char auVar9 [16];
  unsigned char auVar10 [16];
  unsigned char auVar11 [16];
  unsigned char auVar12 [16];
  unsigned char auVar13 [16];
  unsigned char auVar14 [16];
  unsigned char auVar15 [16];
  unsigned char auVar16 [16];
  unsigned char auVar17 [16];
  unsigned char auVar18 [16];
  float fVar19;
  unsigned int in_zero_lo;
  unsigned int in_zero_hi;
  unsigned int in_zero_udw;
  unsigned int in_zero_udw_00;
  unsigned int uVar20;
  unsigned long long extraout_v0_udw;
  unsigned char auVar21 [16];
  unsigned char auVar22 [16];
  unsigned char auVar23 [16];
  unsigned char auVar24 [16];
  unsigned long long extraout_v0_udw_00;
  unsigned char auVar25 [16];
  unsigned char auVar26 [16];
  unsigned char auVar27 [16];
  unsigned char auVar28 [16];
  unsigned char auVar29 [16];
  unsigned char bVar30;
  unsigned int *puVar31;
  unsigned long long uVar32;
  unsigned long long in_v1_udw;
  unsigned char auVar33 [16];
  unsigned char auVar34 [16];
  unsigned char auVar35 [16];
  unsigned char auVar36 [16];
  unsigned long long uVar46;
  unsigned char auVar37 [16];
  unsigned char auVar38 [16];
  unsigned char auVar39 [16];
  unsigned char auVar40 [16];
  unsigned char auVar41 [16];
  unsigned char auVar42 [16];
  unsigned long long uVar47;
  unsigned char auVar43 [16];
  unsigned char auVar44 [16];
  unsigned char auVar45 [16];
  unsigned long long in_a0_udw;
  unsigned char auVar48 [16];
  unsigned char auVar49 [16];
  unsigned char auVar50 [16];
  unsigned char auVar51 [16];
  unsigned char auVar52 [16];
  unsigned char auVar53 [16];
  unsigned char auVar54 [16];
  unsigned long long in_a1_udw;
  unsigned char auVar55 [16];
  unsigned char auVar56 [16];
  unsigned char auVar57 [16];
  unsigned char auVar58 [16];
  unsigned char auVar59 [16];
  unsigned long long in_a2_udw;
  unsigned char auVar60 [16];
  unsigned char auVar61 [16];
  unsigned char auVar62 [16];
  unsigned long long in_a3_udw;
  unsigned char auVar63 [16];
  unsigned char auVar64 [16];
  unsigned char auVar65 [16];
  unsigned char auVar66 [16];
  unsigned long long in_t0_udw;
  unsigned char auVar67 [16];
  unsigned char auVar68 [16];
  unsigned char auVar69 [16];
  unsigned char auVar70 [16];
  unsigned long long in_t3_udw;
  unsigned char auVar71 [16];
  unsigned char auVar72 [16];
  unsigned int in_s1_udw;
  unsigned int in_s1_udw_00;
  unsigned int *puVar73;
  int iVar74;
  float fVar75;
  unsigned char auStack_50 [8];
  unsigned int uStack_48;
  unsigned int uStack_44;
  unsigned long long uStack_40;
  unsigned int uStack_38;
  unsigned int uStack_34;
  unsigned long long uStack_30;
  unsigned int uStack_28;
  unsigned int uStack_24;
  float fStack_4;
  
  auVar68._0_8_ = (long)(int)auStack_50;
  auVar68._8_8_ = in_a1_udw;
  puVar1 = *(unsigned int **)(*(int *)(param_1 + 0x2c) + 8);
  puVar2 = *(unsigned int **)((int)puVar1 + DAT_007646d0);
  puVar73 = (unsigned int *)puVar2[1];
  fStack_4 = (float)puVar2[3] * (float)puVar2[4];
  FUN_0039beb0(*puVar2,auVar68._0_8_);
  fVar75 = *(float *)(DAT_008872e0 + 0x84);
  if (fVar75 != DAT_00763b0c) {
    DAT_00763b08 = 1.0f / (fVar75 - *(float *)(DAT_008872e0 + 0x80));
    DAT_00763b0c = fVar75;
  }
  if (puVar73 == (unsigned int *)0x0) {
    puVar73 = (unsigned int *)*puVar1;
  }
  if (puVar73 != (unsigned int *)0x0) {
    FUN_0040f480(*puVar73);
  }
  uVar32 = auVar68._8_8_;
  *(unsigned int **)(param_1 + 0x40) = puVar73;
  piVar3 = *(int **)(param_1 + 0x40);
  iVar74 = 0;
  if (piVar3 != (int *)0x0) {
    iVar74 = *piVar3;
  }
  DAT_00764a38 = 0;
  if (iVar74 != 0) {
    bVar30 = *(unsigned char *)(iVar74 + 0x23) & 0xf;
    DAT_00764a38 = (unsigned int)(bVar30 == 1 || bVar30 == 5);
  }
  if (iVar74 != DAT_00764a3c) {
    if (iVar74 != 0) {
      auVar69._8_8_ = 0;
      auVar69._0_8_ = uVar32;
      auVar68 = auVar69 << 0x40;
      DAT_00763cd8 = DAT_00763cd8 | 0x10;
      FUN_0040f570(iVar74,0);
    }
    else {
      DAT_00763cd8 = DAT_00763cd8 & 0xffffffffffffffef;
    }
    uVar32 = auVar68._8_8_;
    DAT_00764a3c = iVar74;
    if (piVar3 != (int *)0x0) {
      FUN_003cc500((char)piVar3[0x14]);
      uVar32 = auVar68._8_8_;
      DAT_007649d8 = (unsigned long long)((piVar3[0x14] & 0xf00U) == 0x300);
      if ((piVar3[0x14] & 0xf000U) == 0x3000) {
        DAT_007649d8 = DAT_007649d8 | 4;
      }
    }
  }
  if (DAT_00764a38 != 0) {
    DAT_007649c0 = DAT_007649c0 | 1;
  }
  else {
    DAT_007649c0 = DAT_007649c0 & 0xfffffffffffffffe;
  }
  FUN_003f3eb0(0,0x21);
  puVar1 = DAT_00764974;
  fVar19 = DAT_00763b0c;
  fVar75 = DAT_00763b08;
  auVar11._8_8_ = 0;
  auVar11._0_8_ = in_a0_udw;
  auVar33._8_8_ = in_v1_udw;
  auVar33._0_8_ = 0x1000001a;
  auVar68 = _pcpyld(auVar11 << 0x40,auVar33);
  auVar48._8_8_ = in_a0_udw;
  auVar48._0_8_ = 0x6c0a03bc;
  auVar34._8_8_ = in_v1_udw;
  auVar34._0_8_ = 0x1000101;
  auVar69 = _ppacw(auVar48,auVar34);
  *DAT_00764974 = auVar68._0_4_;
  puVar1[1] = auVar68._4_4_;
  puVar1[2] = auVar68._8_4_;
  puVar1[3] = auVar68._12_4_;
  puVar2 = DAT_00764974;
  auVar67._8_8_ = 0;
  auVar67._0_8_ = auVar68._8_8_;
  auVar12._8_8_ = 0;
  auVar12._0_8_ = in_t0_udw;
  auVar35._4_4_ = 0;
  auVar35._0_4_ = fStack_4;
  auVar35._8_8_ = in_v1_udw;
  auVar68 = _ppacw(auVar67 << 0x40,auVar12 << 0x40);
  auVar67 = _ppacw(auVar69,auVar68);
  DAT_00764974[4] = auVar67._0_4_;
  DAT_00764974[5] = auVar67._4_4_;
  DAT_00764974[6] = auVar67._8_4_;
  DAT_00764974[7] = auVar67._12_4_;
  auVar55._4_4_ = fVar19;
  auVar55._0_4_ = fVar75;
  auVar55._8_8_ = uVar32;
  auVar71._8_8_ = in_t3_udw;
  auVar71._0_8_ = 0x42;
  auVar68 = _pcpyld(auVar55,auVar35);
  DAT_00764974[8] = auVar68._0_4_;
  DAT_00764974[9] = auVar68._4_4_;
  DAT_00764974[10] = auVar68._8_4_;
  DAT_00764974[0xb] = auVar68._12_4_;
  auVar63._8_8_ = in_a3_udw;
  auVar63._0_8_ = 0xe;
  auVar72._8_4_ = in_s1_udw;
  auVar72._0_8_ = 0x1000000000008002;
  auVar72._12_4_ = in_s1_udw_00;
  auVar68 = _pcpyld(auVar63,auVar72);
  DAT_00764974[0xc] = auVar68._0_4_;
  DAT_00764974[0xd] = auVar68._4_4_;
  DAT_00764974[0xe] = auVar68._8_4_;
  DAT_00764974[0xf] = auVar68._12_4_;
  auVar70._8_8_ = auVar69._8_8_;
  auVar70._0_8_ = 0x47;
  auVar60._8_8_ = in_a2_udw;
  auVar60._0_8_ = 0x8000000002;
  auVar69 = _pcpyld(auVar71,auVar60);
  DAT_00764974[0x10] = auVar69._0_4_;
  DAT_00764974[0x11] = auVar69._4_4_;
  DAT_00764974[0x12] = auVar69._8_4_;
  DAT_00764974[0x13] = auVar69._12_4_;
  auVar61._8_8_ = auVar69._8_8_;
  auVar64._8_8_ = in_a3_udw;
  auVar64._0_8_ = 0x8000000048;
  auVar49._8_8_ = in_a0_udw;
  auVar49._0_8_ = 0x6c0103fa01000404;
  auVar61._0_8_ = DAT_007649c0 & 0x70000;
  auVar69 = _pcpyld(auVar70,auVar61);
  DAT_00764974[0x14] = auVar69._0_4_;
  DAT_00764974[0x15] = auVar69._4_4_;
  DAT_00764974[0x16] = auVar69._8_4_;
  DAT_00764974[0x17] = auVar69._12_4_;
  auVar72 = _pcpyld(auVar71,auVar64);
  DAT_00764974[0x18] = auVar68._0_4_;
  DAT_00764974[0x19] = auVar68._4_4_;
  DAT_00764974[0x1a] = auVar68._8_4_;
  DAT_00764974[0x1b] = auVar68._12_4_;
  auVar18._8_8_ = 0;
  auVar18._0_8_ = in_a3_udw;
  DAT_00764974[0x1c] = auVar72._0_4_;
  DAT_00764974[0x1d] = auVar72._4_4_;
  DAT_00764974[0x1e] = auVar72._8_4_;
  DAT_00764974[0x1f] = auVar72._12_4_;
  auVar68 = _pcpyld(auVar49,auVar18 << 0x40);
  DAT_00764974[0x20] = auVar69._0_4_;
  DAT_00764974[0x21] = auVar69._4_4_;
  DAT_00764974[0x22] = auVar69._8_4_;
  DAT_00764974[0x23] = auVar69._12_4_;
  DAT_00764974[0x24] = auStack_50._0_4_;
  DAT_00764974[0x25] = auStack_50._4_4_;
  DAT_00764974[0x26] = uStack_48;
  DAT_00764974[0x27] = uStack_44;
  DAT_00764974[0x28] = (int)uStack_40;
  DAT_00764974[0x29] = (int)((unsigned long long)uStack_40 >> 0x20);
  DAT_00764974[0x2a] = uStack_38;
  DAT_00764974[0x2b] = uStack_34;
  auVar36._12_4_ = uStack_24;
  auVar36._8_4_ = uStack_28;
  DAT_00764974[0x2c] = (int)uStack_30;
  DAT_00764974[0x2d] = (int)((unsigned long long)uStack_30 >> 0x20);
  DAT_00764974[0x2e] = uStack_28;
  DAT_00764974[0x2f] = uStack_24;
  puVar1 = DAT_00764974 + 0x30;
  DAT_00764974 = DAT_00764974 + 0x30;
  *puVar1 = auVar68._0_4_;
  puVar2[0x31] = auVar68._4_4_;
  puVar2[0x32] = auVar68._8_4_;
  puVar2[0x33] = auVar68._12_4_;
  puVar1 = DAT_00764974;
  DAT_00764974 = DAT_00764974 + 4;
  bVar5 = *(char *)(param_1 + 0x47) != -1;
  DAT_00764a34 = (unsigned int)bVar5;
  if (bVar5 || DAT_00764a38 != 0) {
    DAT_00763cd8 = DAT_00763cd8 | 0x40;
  }
  else {
    DAT_00763cd8 = DAT_00763cd8 & 0xffffffffffffffbf;
  }
  auVar50._8_8_ = auVar68._8_8_;
  auVar36._0_8_ = 0xfc007fffffffffff;
  auVar6._4_4_ = in_zero_hi;
  auVar6._0_4_ = in_zero_lo;
  auVar6._8_4_ = in_zero_udw;
  auVar6._12_4_ = in_zero_udw_00;
  auVar68 = _pcpyld(auVar6,auVar36);
  uVar46 = auVar68._8_8_;
  auVar62._8_8_ = auVar69._8_8_;
  auVar62._4_4_ = DAT_0088cb28._4_4_;
  auVar62._0_4_ = (unsigned int)DAT_0088cb28;
  auVar50._0_8_ = 0x6c0103fb01000404;
  uVar32 = DAT_00763cd8 & 0x10;
  auVar65._0_8_ =
       (CONCAT44(DAT_0088cb20._4_4_,(unsigned int)DAT_0088cb20) & auVar68._0_8_) |
       (((long)(int)((unsigned int)DAT_00763cd8 | (unsigned int)*(unsigned char *)(param_1 + 0x29)) & 0x7ffU) << 0x2f);
  auVar65._8_8_ = in_a3_udw;
  auVar13._8_8_ = 0;
  auVar13._0_8_ = auVar67._8_8_;
  auVar69 = _pcpyld(auVar62,auVar65);
  DAT_0088cb20._0_4_ = auVar69._0_4_;
  auVar68 = _pcpyld(auVar50,auVar13 << 0x40);
  DAT_0088cb20._4_4_ = auVar69._4_4_;
  DAT_0088cb28._0_4_ = auVar69._8_4_;
  DAT_0088cb28._4_4_ = auVar69._12_4_;
  *DAT_00764974 = (unsigned int)DAT_0088cb20;
  puVar1[5] = auVar69._4_4_;
  puVar1[6] = auVar69._8_4_;
  puVar1[7] = auVar69._12_4_;
  puVar1[8] = auVar68._0_4_;
  puVar1[9] = auVar68._4_4_;
  puVar1[10] = auVar68._8_4_;
  puVar1[0xb] = auVar68._12_4_;
  DAT_00764974 = DAT_00764974 + 8;
  uVar47 = auVar68._8_8_;
  if (uVar32 == 0) {
    uVar20 = 0x3b808081;
  }
  else {
    uVar20 = 0x3b011b50;
  }
  FUN_003cc560(uVar20,param_1 + 0x44);
  puVar1 = DAT_00764974;
  auVar14._8_8_ = 0;
  auVar14._0_8_ = uVar47;
  auVar37._8_8_ = uVar46;
  auVar37._0_8_ = 0x6c0103fc01000404;
  auVar68 = _pcpyld(auVar37,auVar14 << 0x40);
  *DAT_00764974 = auVar68._0_4_;
  puVar1[1] = auVar68._4_4_;
  puVar1[2] = auVar68._8_4_;
  puVar1[3] = auVar68._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  FUN_003cc680(*(unsigned int *)(param_1 + 0x1c),*(unsigned int *)(param_1 + 0x3c));
  puVar1 = DAT_00764974;
  bVar30 = *(unsigned char *)(param_1 + 0x28);
  auVar21._8_8_ = extraout_v0_udw;
  auVar21._0_8_ = 0x6c0303fd01000404;
  auVar15._8_8_ = 0;
  auVar15._0_8_ = auVar68._8_8_;
  auVar72 = _pcpyld(auVar21,auVar15 << 0x40);
  *DAT_00764974 = auVar72._0_4_;
  puVar1[1] = auVar72._4_4_;
  puVar1[2] = auVar72._8_4_;
  puVar1[3] = auVar72._12_4_;
  auVar69 = _DAT_0088cbb0;
  auVar68 = _DAT_0088cb90;
  puVar1 = DAT_00764974;
  uVar32 = (unsigned long long)bVar30 & 7;
  auVar56._8_8_ = auVar72._8_8_;
  if (((DAT_00764a84 != 0) && ((bVar30 & 4) == 0)) || ((DAT_00763d00 != 0 && ((bVar30 & 4) != 0))))
  {
    uVar32 = uVar32 | 8;
    DAT_00764974[4] = DAT_0088cbb0;
    puVar1[5] = auVar69._4_4_;
    puVar1[6] = auVar69._8_4_;
    puVar1[7] = auVar69._12_4_;
    auVar68 = _DAT_0088cbc0;
    puVar1[8] = DAT_0088cbc0;
    puVar1[9] = auVar68._4_4_;
    puVar1[10] = auVar68._8_4_;
    puVar1[0xb] = auVar68._12_4_;
    uVar46 = auVar68._8_8_;
    auVar68 = auVar69;
    puVar1 = DAT_00764974;
  }
  else {
    DAT_00764974[4] = DAT_0088cb90;
    puVar1[5] = auVar68._4_4_;
    puVar1[6] = auVar68._8_4_;
    puVar1[7] = auVar68._12_4_;
    auVar69 = _DAT_0088cba0;
    puVar1[8] = DAT_0088cba0;
    puVar1[9] = auVar69._4_4_;
    puVar1[10] = auVar69._8_4_;
    puVar1[0xb] = auVar69._12_4_;
    uVar46 = auVar69._8_8_;
    puVar1 = DAT_00764974;
  }
  DAT_00764974 = puVar1 + 0xc;
  auVar51._8_8_ = auVar68._8_8_;
  DAT_00764a7c = 0;
  uVar47 = 0;
  if (DAT_00764a2c == 3) {
    uVar47 = 0x20;
    DAT_00764a7c = 0x20;
  }
  auVar38._8_8_ = uVar46;
  auVar38._0_8_ = uVar32;
  auVar51._0_8_ = ((unsigned long long)DAT_00764a80 << 0x20) | uVar47;
  auVar68 = _pcpyld(auVar51,auVar38);
  *DAT_00764974 = auVar68._0_4_;
  puVar1[0xd] = auVar68._4_4_;
  puVar1[0xe] = auVar68._8_4_;
  puVar1[0xf] = auVar68._12_4_;
  puVar1 = DAT_00764974;
  DAT_00764974 = DAT_00764974 + 4;
  auVar39._8_8_ = auVar68._8_8_;
  if (*(int *)(param_1 + 0x40) != 0) {
    auVar8._8_8_ = 0;
    auVar8._0_8_ = auVar39._8_8_;
    auVar22._8_8_ = extraout_v0_udw;
    auVar22._0_8_ = 0x5000000400000000;
    auVar68 = _pcpyld(auVar22,auVar8 << 0x40);
    *DAT_00764974 = auVar68._0_4_;
    puVar1[5] = auVar68._4_4_;
    puVar1[6] = auVar68._8_4_;
    puVar1[7] = auVar68._12_4_;
    auVar56._0_8_ = 0xe;
    auVar23._8_8_ = auVar68._8_8_;
    auVar52._8_8_ = auVar51._8_8_;
    auVar52._0_8_ = 0x47;
    auVar23._0_8_ = 0x1000000000008003;
    auVar68 = _pcpyld(auVar56,auVar23);
    puVar1[8] = auVar68._0_4_;
    puVar1[9] = auVar68._4_4_;
    puVar1[10] = auVar68._8_4_;
    puVar1[0xb] = auVar68._12_4_;
    auVar39._0_8_ = 0x14;
    auVar57._8_8_ = auVar56._8_8_;
    auVar57._0_8_ = DAT_007649c0;
    auVar24._8_8_ = auVar68._8_8_;
    auVar24._0_8_ = 8;
    auVar68 = _pcpyld(auVar52,auVar57);
    puVar1[0xc] = auVar68._0_4_;
    puVar1[0xd] = auVar68._4_4_;
    puVar1[0xe] = auVar68._8_4_;
    puVar1[0xf] = auVar68._12_4_;
    auVar53._8_8_ = auVar68._8_8_;
    auVar53._0_8_ = DAT_007649e0;
    auVar68 = _pcpyld(auVar39,auVar53);
    puVar1[0x10] = auVar68._0_4_;
    puVar1[0x11] = auVar68._4_4_;
    puVar1[0x12] = auVar68._8_4_;
    puVar1[0x13] = auVar68._12_4_;
    auVar40._8_8_ = auVar68._8_8_;
    auVar40._0_8_ = DAT_007649d8;
    auVar68 = _pcpyld(auVar24,auVar40);
    puVar1[0x14] = auVar68._0_4_;
    puVar1[0x15] = auVar68._4_4_;
    puVar1[0x16] = auVar68._8_4_;
    puVar1[0x17] = auVar68._12_4_;
  }
  else {
    *DAT_00764974 = in_zero_lo;
    puVar1[5] = in_zero_hi;
    puVar1[6] = in_zero_udw;
    puVar1[7] = in_zero_udw_00;
    auVar28._8_8_ = extraout_v0_udw;
    auVar28._0_8_ = 0x5000000200000000;
    auVar10._8_8_ = 0;
    auVar10._0_8_ = auVar39._8_8_;
    puVar1[8] = in_zero_lo;
    puVar1[9] = in_zero_hi;
    puVar1[10] = in_zero_udw;
    puVar1[0xb] = in_zero_udw_00;
    auVar68 = _pcpyld(auVar28,auVar10 << 0x40);
    puVar1[0xc] = auVar68._0_4_;
    puVar1[0xd] = auVar68._4_4_;
    puVar1[0xe] = auVar68._8_4_;
    puVar1[0xf] = auVar68._12_4_;
    auVar53._8_8_ = auVar51._8_8_;
    auVar53._0_8_ = 0xe;
    auVar29._8_8_ = auVar68._8_8_;
    auVar45._8_8_ = auVar39._8_8_;
    auVar45._0_8_ = 0x1000000000008001;
    auVar68 = _pcpyld(auVar53,auVar45);
    puVar1[0x10] = auVar68._0_4_;
    puVar1[0x11] = auVar68._4_4_;
    puVar1[0x12] = auVar68._8_4_;
    puVar1[0x13] = auVar68._12_4_;
    auVar29._0_8_ = 0x47;
    auVar40._8_8_ = auVar68._8_8_;
    auVar40._0_8_ = DAT_007649c0;
    auVar68 = _pcpyld(auVar29,auVar40);
    puVar1[0x14] = auVar68._0_4_;
    puVar1[0x15] = auVar68._4_4_;
    puVar1[0x16] = auVar68._8_4_;
    puVar1[0x17] = auVar68._12_4_;
  }
  DAT_00764974 = DAT_00764974 + 0x14;
  auVar42._8_8_ = auVar40._8_8_;
  *(unsigned char *)(param_1 + 0x2b) =
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x20) >> 5) |
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x18) >> 2);
  puVar1 = DAT_00764974;
  uVar32 = auVar53._8_8_;
  puVar31 = (unsigned int *)(*(int *)(*(int *)(param_1 + 4) + 0x18) + (unsigned int)*(unsigned char *)(param_1 + 0x2b) * 4);
  auVar42._0_8_ = (long)(int)puVar31;
  uVar4 = *puVar31;
  if (DAT_00764a78 != uVar4) {
    auVar9._8_8_ = 0;
    auVar9._0_8_ = auVar56._8_8_;
    auVar41._0_8_ = ((unsigned long long)uVar4 << 0x20) | 0x50000000;
    auVar41._8_8_ = auVar42._8_8_;
    auVar42 = _pcpyld(auVar9 << 0x40,auVar41);
    *DAT_00764974 = auVar42._0_4_;
    puVar1[1] = auVar42._4_4_;
    puVar1[2] = auVar42._8_4_;
    puVar1[3] = auVar42._12_4_;
    DAT_00764974 = DAT_00764974 + 4;
    DAT_00764a78 = uVar4;
  }
  puVar1 = DAT_00764974;
  auVar66._8_8_ = auVar65._8_8_;
  uVar47 = auVar42._8_8_;
  iVar74 = **(int **)(param_1 + 0x30);
  auVar66._0_8_ = (((long)*(int *)(*(int *)(param_1 + 4) + 0x14) | 0x2000000U) << 0x20) | 0x3000000;
  auVar58._0_8_ = ((unsigned long long)*(unsigned int *)(iVar74 + 0x20) << 0x20) | 0x50000000;
  auVar58._8_8_ = auVar56._8_8_;
  auVar68 = _pcpyld(auVar66,auVar58);
  *DAT_00764974 = auVar68._0_4_;
  puVar1[1] = auVar68._4_4_;
  puVar1[2] = auVar68._8_4_;
  puVar1[3] = auVar68._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  uVar46 = auVar68._8_8_;
  do {
    DI();
    SYNC(0x10);
  } while ((Status & 0x10000) != 0);
  *(int *)(iVar74 + 0x18) = *(int *)(iVar74 + 0x18) + 1;
  EI();
  *(int *)(iVar74 + 0x1c) = *(int *)(iVar74 + 0x1c) + 1;
  if (*(int *)(iVar74 + 0x1c) == 1) {
    FUN_003f36a0();
  }
  FUN_003f3eb0(0,4);
  puVar1 = DAT_00764974;
  auVar16._8_8_ = 0;
  auVar16._0_8_ = uVar47;
  auVar25._8_8_ = extraout_v0_udw_00;
  auVar25._0_8_ = 0x50000003;
  auVar17._8_8_ = 0;
  auVar17._0_8_ = uVar32;
  auVar72 = _ppacw(auVar25,auVar16 << 0x40);
  auVar26._8_8_ = extraout_v0_udw_00;
  auVar26._0_8_ = 0xe;
  auVar7._8_4_ = in_s1_udw;
  auVar7._0_8_ = 0x1000000000008002;
  auVar7._12_4_ = in_s1_udw_00;
  auVar69 = _pcpyld(auVar26,auVar7);
  auVar27._8_8_ = extraout_v0_udw_00;
  auVar27._0_8_ = 0x10000003;
  auVar59._8_8_ = uVar46;
  auVar59._0_8_ = 0x42;
  auVar68 = _ppacw(auVar17 << 0x40,auVar27);
  auVar68 = _ppacw(auVar72,auVar68);
  *DAT_00764974 = auVar68._0_4_;
  puVar1[1] = auVar68._4_4_;
  puVar1[2] = auVar68._8_4_;
  puVar1[3] = auVar68._12_4_;
  auVar54._8_8_ = uVar32;
  auVar54._0_8_ = 0x47;
  puVar1[4] = auVar69._0_4_;
  puVar1[5] = auVar69._4_4_;
  puVar1[6] = auVar69._8_4_;
  puVar1[7] = auVar69._12_4_;
  auVar43._8_8_ = uVar47;
  auVar43._0_8_ = DAT_007649e8;
  auVar68 = _pcpyld(auVar59,auVar43);
  puVar1[8] = auVar68._0_4_;
  puVar1[9] = auVar68._4_4_;
  puVar1[10] = auVar68._8_4_;
  puVar1[0xb] = auVar68._12_4_;
  auVar44._8_8_ = auVar68._8_8_;
  auVar44._0_8_ = DAT_007649c0;
  auVar68 = _pcpyld(auVar54,auVar44);
  puVar1[0xc] = auVar68._0_4_;
  puVar1[0xd] = auVar68._4_4_;
  puVar1[0xe] = auVar68._8_4_;
  puVar1[0xf] = auVar68._12_4_;
  DAT_00764974 = DAT_00764974 + 0x10;
  return 1;
}
