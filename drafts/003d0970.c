/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d0970 func_003d0970 src/promoted/code1_003d.c - romwright draft, unreviewed */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

unsigned long long FUN_003d0970(int param_1)

{
  int *piVar1;
  unsigned int uVar2;
  unsigned char bVar3;
  unsigned char auVar4 [16];
  unsigned char auVar5 [16];
  unsigned char auVar6 [16];
  unsigned char auVar7 [16];
  unsigned char auVar8 [16];
  unsigned char auVar9 [16];
  unsigned char auVar10 [16];
  unsigned char auVar11 [16];
  unsigned int *puVar12;
  unsigned int in_zero_lo;
  unsigned int in_zero_hi;
  unsigned int in_zero_udw;
  unsigned int in_zero_udw_00;
  unsigned int uVar13;
  unsigned long long uVar14;
  unsigned long long extraout_v0_udw;
  unsigned char auVar15 [16];
  unsigned char auVar16 [16];
  unsigned char auVar17 [16];
  unsigned char auVar18 [16];
  unsigned char auVar19 [16];
  unsigned long long extraout_v0_udw_00;
  unsigned char auVar20 [16];
  unsigned long long extraout_v0_udw_01;
  unsigned char auVar21 [16];
  unsigned char auVar22 [16];
  unsigned char auVar23 [16];
  unsigned char auVar24 [16];
  unsigned char auVar25 [16];
  unsigned char auVar26 [16];
  unsigned long long uVar31;
  unsigned char auVar27 [16];
  unsigned char auVar28 [16];
  unsigned char auVar29 [16];
  unsigned char auVar30 [16];
  unsigned char bVar32;
  unsigned int *puVar33;
  unsigned long long in_v1_udw;
  unsigned char auVar34 [16];
  unsigned long long uVar40;
  unsigned char auVar35 [16];
  unsigned char auVar36 [16];
  unsigned char auVar37 [16];
  unsigned char auVar38 [16];
  unsigned char auVar39 [16];
  unsigned long long uVar41;
  unsigned long long in_a0_udw;
  unsigned char auVar42 [16];
  unsigned char auVar43 [16];
  unsigned char auVar44 [16];
  unsigned char auVar45 [16];
  unsigned char in_a1_qw [16];
  unsigned long long uVar48;
  unsigned char auVar46 [16];
  unsigned char auVar47 [16];
  unsigned char auVar49 [16];
  unsigned char auVar50 [16];
  int iVar51;
  
  if (*(unsigned int **)(param_1 + 0x40) != (unsigned int *)0x0) {
    FUN_0040f480(**(unsigned int **)(param_1 + 0x40));
  }
  uVar48 = in_a1_qw._8_8_;
  piVar1 = *(int **)(param_1 + 0x40);
  iVar51 = 0;
  if (piVar1 != (int *)0x0) {
    iVar51 = *piVar1;
  }
  DAT_00764a38 = 0;
  if (iVar51 != 0) {
    bVar32 = *(unsigned char *)(iVar51 + 0x23) & 0xf;
    DAT_00764a38 = (unsigned int)(bVar32 == 1 || bVar32 == 5);
  }
  if (iVar51 != DAT_00764a3c) {
    if (iVar51 != 0) {
      auVar17._8_8_ = 0;
      auVar17._0_8_ = uVar48;
      in_a1_qw = auVar17 << 0x40;
      DAT_00763cd8 = DAT_00763cd8 | 0x10;
      FUN_0040f570(iVar51,0);
    }
    else {
      DAT_00763cd8 = DAT_00763cd8 & 0xffffffffffffffef;
    }
    uVar48 = in_a1_qw._8_8_;
    DAT_00764a3c = iVar51;
    if (piVar1 != (int *)0x0) {
      FUN_003cc500((char)piVar1[0x14]);
      uVar48 = in_a1_qw._8_8_;
      DAT_007649d8 = (unsigned long long)((piVar1[0x14] & 0xf00U) == 0x300);
      if ((piVar1[0x14] & 0xf000U) == 0x3000) {
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
  FUN_003f3eb0(0,0x12);
  puVar12 = DAT_00764974;
  auVar49._8_8_ = 0;
  auVar49._0_8_ = in_v1_udw;
  auVar15._8_8_ = extraout_v0_udw;
  auVar15._0_8_ = 0x1000000f;
  auVar49 = _pcpyld(auVar49 << 0x40,auVar15);
  auVar8._8_8_ = 0;
  auVar8._0_8_ = uVar48;
  auVar16._8_8_ = extraout_v0_udw;
  auVar16._0_8_ = 0x6c0103fa01000404;
  *DAT_00764974 = auVar49._0_4_;
  puVar12[1] = auVar49._4_4_;
  puVar12[2] = auVar49._8_4_;
  puVar12[3] = auVar49._12_4_;
  auVar17 = _pcpyld(auVar16,auVar8 << 0x40);
  puVar12[4] = auVar17._0_4_;
  puVar12[5] = auVar17._4_4_;
  puVar12[6] = auVar17._8_4_;
  puVar12[7] = auVar17._12_4_;
  puVar12 = DAT_00764974;
  auVar18._8_8_ = auVar17._8_8_;
  DAT_00764974 = DAT_00764974 + 8;
  bVar3 = *(char *)(param_1 + 0x47) != -1;
  DAT_00764a34 = (unsigned int)bVar3;
  if (bVar3 || DAT_00764a38 != 0) {
    DAT_00763cd8 = DAT_00763cd8 | 0x40;
  }
  else {
    DAT_00763cd8 = DAT_00763cd8 & 0xffffffffffffffbf;
  }
  uVar14 = DAT_00763cd8;
  auVar50._8_8_ = auVar49._8_8_;
  auVar50._0_8_ = 0xfc007fffffffffff;
  auVar18._4_4_ = DAT_0088cb28._4_4_;
  auVar18._0_4_ = (unsigned int)DAT_0088cb28;
  auVar4._4_4_ = in_zero_hi;
  auVar4._0_4_ = in_zero_lo;
  auVar4._8_4_ = in_zero_udw;
  auVar4._12_4_ = in_zero_udw_00;
  auVar17 = _pcpyld(auVar4,auVar50);
  auVar42._0_8_ =
       (CONCAT44(DAT_0088cb20._4_4_,(unsigned int)DAT_0088cb20) & auVar17._0_8_) |
       (((long)(int)((unsigned int)DAT_00763cd8 | (unsigned int)*(unsigned char *)(param_1 + 0x29)) & 0x7ffU) << 0x2f);
  auVar42._8_8_ = in_a0_udw;
  auVar17 = _pcpyld(auVar18,auVar42);
  DAT_0088cb20._0_4_ = auVar17._0_4_;
  DAT_0088cb20._4_4_ = auVar17._4_4_;
  DAT_0088cb28._0_4_ = auVar17._8_4_;
  DAT_0088cb28._4_4_ = auVar17._12_4_;
  *DAT_00764974 = (unsigned int)DAT_0088cb20;
  puVar12[9] = auVar17._4_4_;
  puVar12[10] = auVar17._8_4_;
  puVar12[0xb] = auVar17._12_4_;
  auVar5._8_8_ = 0;
  auVar5._0_8_ = auVar17._8_8_;
  auVar19._8_8_ = auVar18._8_8_;
  auVar19._0_8_ = 0x6c0103fb01000404;
  auVar17 = _pcpyld(auVar19,auVar5 << 0x40);
  puVar12[0xc] = auVar17._0_4_;
  puVar12[0xd] = auVar17._4_4_;
  puVar12[0xe] = auVar17._8_4_;
  puVar12[0xf] = auVar17._12_4_;
  DAT_00764974 = DAT_00764974 + 8;
  if ((uVar14 & 0x10) == 0) {
    uVar13 = 0x3b808081;
  }
  else {
    uVar13 = 0x3b011b50;
  }
  FUN_003cc560(uVar13,param_1 + 0x44);
  puVar12 = DAT_00764974;
  auVar9._8_8_ = 0;
  auVar9._0_8_ = in_a0_udw;
  auVar20._8_8_ = extraout_v0_udw_00;
  auVar20._0_8_ = 0x6c0103fc01000404;
  auVar17 = _pcpyld(auVar20,auVar9 << 0x40);
  *DAT_00764974 = auVar17._0_4_;
  puVar12[1] = auVar17._4_4_;
  puVar12[2] = auVar17._8_4_;
  puVar12[3] = auVar17._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  uVar31 = auVar17._8_8_;
  FUN_003cc680(*(unsigned int *)(param_1 + 0x1c),*(unsigned int *)(param_1 + 0x3c));
  puVar12 = DAT_00764974;
  bVar32 = *(unsigned char *)(param_1 + 0x28);
  auVar10._8_8_ = 0;
  auVar10._0_8_ = uVar48;
  auVar34._8_8_ = uVar31;
  auVar34._0_8_ = 0x6c0303fd01000404;
  uVar14 = (unsigned long long)bVar32 & 7;
  auVar17 = _pcpyld(auVar34,auVar10 << 0x40);
  *DAT_00764974 = auVar17._0_4_;
  puVar12[1] = auVar17._4_4_;
  puVar12[2] = auVar17._8_4_;
  puVar12[3] = auVar17._12_4_;
  auVar49 = _DAT_0088cbb0;
  auVar17 = _DAT_0088cb90;
  puVar12 = DAT_00764974;
  if (((DAT_00764a84 != 0) && ((bVar32 & 4) == 0)) || ((DAT_00763d00 != 0 && ((bVar32 & 4) != 0))))
  {
    uVar14 = uVar14 | 8;
    DAT_00764974[4] = DAT_0088cbb0;
    puVar12[5] = auVar49._4_4_;
    puVar12[6] = auVar49._8_4_;
    puVar12[7] = auVar49._12_4_;
    auVar17 = _DAT_0088cbc0;
    puVar12[8] = DAT_0088cbc0;
    puVar12[9] = auVar17._4_4_;
    puVar12[10] = auVar17._8_4_;
    puVar12[0xb] = auVar17._12_4_;
    uVar40 = auVar17._8_8_;
    auVar17 = auVar49;
    puVar12 = DAT_00764974;
  }
  else {
    DAT_00764974[4] = DAT_0088cb90;
    puVar12[5] = auVar17._4_4_;
    puVar12[6] = auVar17._8_4_;
    puVar12[7] = auVar17._12_4_;
    auVar49 = _DAT_0088cba0;
    puVar12[8] = DAT_0088cba0;
    puVar12[9] = auVar49._4_4_;
    puVar12[10] = auVar49._8_4_;
    puVar12[0xb] = auVar49._12_4_;
    uVar40 = auVar49._8_8_;
    puVar12 = DAT_00764974;
  }
  DAT_00764974 = puVar12 + 0xc;
  DAT_00764a7c = 0;
  auVar43._8_8_ = auVar17._8_8_;
  uVar41 = 0;
  if (DAT_00764a2c == 3) {
    uVar41 = 0x20;
    DAT_00764a7c = 0x20;
  }
  auVar21._8_8_ = extraout_v0_udw_01;
  auVar21._0_8_ = uVar14;
  auVar43._0_8_ = ((unsigned long long)DAT_00764a80 << 0x20) | uVar41;
  auVar17 = _pcpyld(auVar43,auVar21);
  *DAT_00764974 = auVar17._0_4_;
  puVar12[0xd] = auVar17._4_4_;
  puVar12[0xe] = auVar17._8_4_;
  puVar12[0xf] = auVar17._12_4_;
  puVar12 = DAT_00764974;
  puVar33 = DAT_00764974 + 4;
  auVar44._8_8_ = auVar17._8_8_;
  DAT_00764974 = puVar33;
  if (*(int *)(param_1 + 0x40) != 0) {
    auVar6._8_8_ = 0;
    auVar6._0_8_ = auVar44._8_8_;
    auVar22._8_8_ = extraout_v0_udw_01;
    auVar22._0_8_ = 0x5000000400000000;
    auVar17 = _pcpyld(auVar22,auVar6 << 0x40);
    auVar44._0_8_ = 0xe;
    *puVar33 = auVar17._0_4_;
    puVar12[5] = auVar17._4_4_;
    puVar12[6] = auVar17._8_4_;
    puVar12[7] = auVar17._12_4_;
    auVar23._8_8_ = auVar17._8_8_;
    auVar23._0_8_ = 0x1000000000008003;
    auVar17 = _pcpyld(auVar44,auVar23);
    puVar12[8] = auVar17._0_4_;
    puVar12[9] = auVar17._4_4_;
    puVar12[10] = auVar17._8_4_;
    puVar12[0xb] = auVar17._12_4_;
    auVar35._8_8_ = uVar40;
    auVar35._0_8_ = DAT_007649c0;
    auVar24._8_8_ = auVar17._8_8_;
    auVar24._0_8_ = 0x47;
    auVar17 = _pcpyld(auVar24,auVar35);
    puVar12[0xc] = auVar17._0_4_;
    puVar12[0xd] = auVar17._4_4_;
    puVar12[0xe] = auVar17._8_4_;
    puVar12[0xf] = auVar17._12_4_;
    auVar36._8_8_ = uVar40;
    auVar36._0_8_ = DAT_007649e0;
    auVar25._8_8_ = auVar17._8_8_;
    auVar25._0_8_ = 0x14;
    auVar17 = _pcpyld(auVar25,auVar36);
    puVar12[0x10] = auVar17._0_4_;
    puVar12[0x11] = auVar17._4_4_;
    puVar12[0x12] = auVar17._8_4_;
    puVar12[0x13] = auVar17._12_4_;
    auVar37._8_8_ = uVar40;
    auVar37._0_8_ = DAT_007649d8;
    auVar26._8_8_ = auVar17._8_8_;
    auVar26._0_8_ = 8;
    auVar17 = _pcpyld(auVar26,auVar37);
    puVar12[0x14] = auVar17._0_4_;
    puVar12[0x15] = auVar17._4_4_;
    puVar12[0x16] = auVar17._8_4_;
    puVar12[0x17] = auVar17._12_4_;
    uVar31 = auVar17._8_8_;
  }
  else {
    *puVar33 = in_zero_lo;
    puVar12[5] = in_zero_hi;
    puVar12[6] = in_zero_udw;
    puVar12[7] = in_zero_udw_00;
    auVar29._8_8_ = extraout_v0_udw_01;
    auVar29._0_8_ = 0x5000000200000000;
    auVar11._8_8_ = 0;
    auVar11._0_8_ = uVar40;
    puVar12[8] = in_zero_lo;
    puVar12[9] = in_zero_hi;
    puVar12[10] = in_zero_udw;
    puVar12[0xb] = in_zero_udw_00;
    auVar17 = _pcpyld(auVar29,auVar11 << 0x40);
    puVar12[0xc] = auVar17._0_4_;
    puVar12[0xd] = auVar17._4_4_;
    puVar12[0xe] = auVar17._8_4_;
    puVar12[0xf] = auVar17._12_4_;
    auVar45._8_8_ = auVar44._8_8_;
    auVar45._0_8_ = 0xe;
    auVar30._8_8_ = auVar17._8_8_;
    auVar38._8_8_ = uVar40;
    auVar38._0_8_ = 0x1000000000008001;
    auVar17 = _pcpyld(auVar45,auVar38);
    puVar12[0x10] = auVar17._0_4_;
    puVar12[0x11] = auVar17._4_4_;
    puVar12[0x12] = auVar17._8_4_;
    puVar12[0x13] = auVar17._12_4_;
    auVar30._0_8_ = 0x47;
    auVar39._8_8_ = auVar17._8_8_;
    auVar39._0_8_ = DAT_007649c0;
    auVar17 = _pcpyld(auVar30,auVar39);
    puVar12[0x14] = auVar17._0_4_;
    puVar12[0x15] = auVar17._4_4_;
    puVar12[0x16] = auVar17._8_4_;
    puVar12[0x17] = auVar17._12_4_;
    uVar31 = auVar17._8_8_;
  }
  DAT_00764974 = DAT_00764974 + 0x14;
  auVar46._0_8_ = (long)(int)puVar33;
  auVar46._8_8_ = uVar48;
  *(unsigned char *)(param_1 + 0x2b) =
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x20) >> 5) |
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x18) >> 2);
  puVar12 = DAT_00764974;
  uVar2 = *(unsigned int *)(*(int *)(*(int *)(param_1 + 4) + 0x18) + (unsigned int)*(unsigned char *)(param_1 + 0x2b) * 4);
  if (DAT_00764a78 != uVar2) {
    auVar7._8_8_ = 0;
    auVar7._0_8_ = uVar48;
    auVar46 = auVar7 << 0x40;
    auVar27._0_8_ = ((unsigned long long)uVar2 << 0x20) | 0x50000000;
    auVar27._8_8_ = uVar31;
    auVar17 = _pcpyld(auVar46,auVar27);
    *DAT_00764974 = auVar17._0_4_;
    puVar12[1] = auVar17._4_4_;
    puVar12[2] = auVar17._8_4_;
    puVar12[3] = auVar17._12_4_;
    DAT_00764974 = DAT_00764974 + 4;
    DAT_00764a78 = uVar2;
  }
  puVar12 = DAT_00764974;
  auVar47._8_8_ = auVar46._8_8_;
  iVar51 = **(int **)(param_1 + 0x30);
  auVar47._0_8_ = (((long)*(int *)(*(int *)(param_1 + 4) + 0x14) | 0x2000000U) << 0x20) | 0x3000000;
  auVar28._0_8_ = ((unsigned long long)*(unsigned int *)(iVar51 + 0x20) << 0x20) | 0x50000000;
  auVar28._8_8_ = uVar31;
  auVar17 = _pcpyld(auVar47,auVar28);
  *DAT_00764974 = auVar17._0_4_;
  puVar12[1] = auVar17._4_4_;
  puVar12[2] = auVar17._8_4_;
  puVar12[3] = auVar17._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  do {
    DI();
    SYNC(0x10);
  } while ((Status & 0x10000) != 0);
  *(int *)(iVar51 + 0x18) = *(int *)(iVar51 + 0x18) + 1;
  EI();
  *(int *)(iVar51 + 0x1c) = *(int *)(iVar51 + 0x1c) + 1;
  if (*(int *)(iVar51 + 0x1c) == 1) {
    FUN_003f36a0();
  }
  return 1;
}
