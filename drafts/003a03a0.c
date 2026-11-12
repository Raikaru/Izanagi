/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a03a0 func_003a03a0 src/promoted/code1_003a.c - romwright draft, unreviewed */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

unsigned long long FUN_003a03a0(int param_1)

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
  unsigned long long extraout_v0_udw;
  unsigned char auVar14 [16];
  unsigned char auVar15 [16];
  unsigned char auVar16 [16];
  unsigned char auVar17 [16];
  unsigned char auVar18 [16];
  unsigned long long extraout_v0_udw_00;
  unsigned char auVar19 [16];
  unsigned char auVar20 [16];
  unsigned char auVar21 [16];
  unsigned char auVar22 [16];
  unsigned long long extraout_v0_udw_01;
  unsigned char auVar23 [16];
  unsigned long long extraout_v0_udw_02;
  unsigned char auVar24 [16];
  unsigned char auVar25 [16];
  unsigned char auVar26 [16];
  unsigned char auVar27 [16];
  unsigned char auVar28 [16];
  unsigned char auVar29 [16];
  unsigned long long uVar34;
  unsigned char auVar30 [16];
  unsigned char auVar31 [16];
  unsigned char auVar32 [16];
  unsigned char auVar33 [16];
  unsigned char bVar35;
  unsigned int *puVar36;
  unsigned long long in_v1_udw;
  unsigned char auVar37 [16];
  unsigned char auVar38 [16];
  unsigned long long uVar44;
  unsigned char auVar39 [16];
  unsigned char auVar40 [16];
  unsigned char auVar41 [16];
  unsigned char auVar42 [16];
  unsigned char auVar43 [16];
  unsigned long long uVar45;
  unsigned long long in_a0_udw;
  unsigned char auVar46 [16];
  unsigned char auVar47 [16];
  unsigned char auVar48 [16];
  unsigned char auVar49 [16];
  unsigned char auVar50 [16];
  unsigned char in_a1_qw [16];
  unsigned long long uVar53;
  unsigned char auVar51 [16];
  unsigned char auVar52 [16];
  unsigned char auVar54 [16];
  unsigned char auVar55 [16];
  int iVar56;
  unsigned long long *puVar57;
  int uStack_40;
  unsigned int uStack_3c;
  unsigned int uStack_38;
  unsigned int uStack_34;
  unsigned int uStack_30;
  unsigned int uStack_2c;
  unsigned int uStack_28;
  unsigned int uStack_20;
  unsigned int uStack_1c;
  unsigned int uStack_18;
  unsigned int uStack_10;
  unsigned int uStack_c;
  unsigned int uStack_8;
  
  puVar57 = (unsigned long long *)**(int **)(*(int *)(*(int *)(param_1 + 0x2c) + 8) + DAT_007646d0);
  if (puVar57 == (unsigned long long *)0x0) {
    uStack_18 = 0x3f800000;
    uStack_2c = 0x3f800000;
    puVar57 = (unsigned long long *)&uStack_40;
    uStack_40 = 0x3f800000;
    uStack_30 = 0;
    uStack_38 = 0;
    uStack_3c = 0;
    uStack_1c = 0;
    uStack_20 = 0;
    uStack_34 = uStack_34 | 0x20003;
    uStack_28 = 0;
    uStack_8 = 0;
    uStack_c = 0;
    uStack_10 = 0;
  }
  if (*(unsigned int **)(param_1 + 0x40) != (unsigned int *)0x0) {
    FUN_0040f480(**(unsigned int **)(param_1 + 0x40));
  }
  uVar53 = in_a1_qw._8_8_;
  piVar1 = *(int **)(param_1 + 0x40);
  iVar56 = 0;
  if (piVar1 != (int *)0x0) {
    iVar56 = *piVar1;
  }
  DAT_00764a38 = 0;
  if (iVar56 != 0) {
    bVar35 = *(unsigned char *)(iVar56 + 0x23) & 0xf;
    DAT_00764a38 = (unsigned int)(bVar35 == 1 || bVar35 == 5);
  }
  if (iVar56 != DAT_00764a3c) {
    if (iVar56 != 0) {
      auVar15._8_8_ = 0;
      auVar15._0_8_ = uVar53;
      in_a1_qw = auVar15 << 0x40;
      DAT_00763cd8 = DAT_00763cd8 | 0x10;
      FUN_0040f570(iVar56,0);
    }
    else {
      DAT_00763cd8 = DAT_00763cd8 & 0xffffffffffffffef;
    }
    uVar53 = in_a1_qw._8_8_;
    DAT_00764a3c = iVar56;
    if (piVar1 != (int *)0x0) {
      FUN_003cc500((char)piVar1[0x14]);
      uVar53 = in_a1_qw._8_8_;
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
  FUN_003f3eb0(0,4);
  auVar37._8_8_ = in_v1_udw;
  auVar37._0_8_ = 0x6c0303bc01000404;
  auVar14._8_8_ = extraout_v0_udw;
  auVar14._0_8_ = 0x10000003;
  auVar15 = _pcpyld(auVar37,auVar14);
  auVar46._8_8_ = 0;
  auVar46._0_8_ = in_a0_udw;
  auVar46 = auVar46 << 0x40;
  *DAT_00764974 = auVar15._0_4_;
  DAT_00764974[1] = auVar15._4_4_;
  DAT_00764974[2] = auVar15._8_4_;
  DAT_00764974[3] = auVar15._12_4_;
  auVar16._8_8_ = auVar15._8_8_;
  auVar16._0_8_ = *puVar57;
  auVar15 = _pcpyld(auVar46,auVar16);
  DAT_00764974[4] = auVar15._0_4_;
  DAT_00764974[5] = auVar15._4_4_;
  DAT_00764974[6] = auVar15._8_4_;
  DAT_00764974[7] = auVar15._12_4_;
  auVar17._8_8_ = auVar15._8_8_;
  auVar17._0_8_ = puVar57[2];
  auVar15 = _pcpyld(auVar46,auVar17);
  DAT_00764974[8] = auVar15._0_4_;
  DAT_00764974[9] = auVar15._4_4_;
  DAT_00764974[10] = auVar15._8_4_;
  DAT_00764974[0xb] = auVar15._12_4_;
  auVar18._8_8_ = auVar15._8_8_;
  auVar18._0_8_ = puVar57[6];
  auVar15 = _pcpyld(auVar46,auVar18);
  DAT_00764974[0xc] = auVar15._0_4_;
  DAT_00764974[0xd] = auVar15._4_4_;
  DAT_00764974[0xe] = auVar15._8_4_;
  DAT_00764974[0xf] = auVar15._12_4_;
  DAT_00764974 = DAT_00764974 + 0x10;
  FUN_003f3eb0(0,0x12);
  puVar12 = DAT_00764974;
  auVar54._8_8_ = 0;
  auVar54._0_8_ = in_v1_udw;
  auVar19._8_8_ = extraout_v0_udw_00;
  auVar19._0_8_ = 0x1000000f;
  auVar54 = _pcpyld(auVar54 << 0x40,auVar19);
  auVar47._8_8_ = auVar46._8_8_;
  auVar8._8_8_ = 0;
  auVar8._0_8_ = uVar53;
  auVar20._8_8_ = extraout_v0_udw_00;
  auVar20._0_8_ = 0x6c0103fa01000404;
  *DAT_00764974 = auVar54._0_4_;
  puVar12[1] = auVar54._4_4_;
  puVar12[2] = auVar54._8_4_;
  puVar12[3] = auVar54._12_4_;
  auVar15 = _pcpyld(auVar20,auVar8 << 0x40);
  puVar12[4] = auVar15._0_4_;
  puVar12[5] = auVar15._4_4_;
  puVar12[6] = auVar15._8_4_;
  puVar12[7] = auVar15._12_4_;
  puVar12 = DAT_00764974;
  auVar21._8_8_ = auVar15._8_8_;
  DAT_00764974 = DAT_00764974 + 8;
  bVar3 = *(char *)(param_1 + 0x47) != -1;
  DAT_00764a34 = (unsigned int)bVar3;
  if (bVar3 || DAT_00764a38 != 0) {
    DAT_00763cd8 = DAT_00763cd8 | 0x40;
  }
  else {
    DAT_00763cd8 = DAT_00763cd8 & 0xffffffffffffffbf;
  }
  uVar44 = DAT_00763cd8;
  auVar55._8_8_ = auVar54._8_8_;
  auVar55._0_8_ = 0xfc007fffffffffff;
  auVar21._4_4_ = DAT_0088cb28._4_4_;
  auVar21._0_4_ = (unsigned int)DAT_0088cb28;
  auVar4._4_4_ = in_zero_hi;
  auVar4._0_4_ = in_zero_lo;
  auVar4._8_4_ = in_zero_udw;
  auVar4._12_4_ = in_zero_udw_00;
  auVar15 = _pcpyld(auVar4,auVar55);
  auVar47._0_8_ =
       (CONCAT44(DAT_0088cb20._4_4_,(unsigned int)DAT_0088cb20) & auVar15._0_8_) |
       (((long)(int)((unsigned int)DAT_00763cd8 | (unsigned int)*(unsigned char *)(param_1 + 0x29)) & 0x7ffU) << 0x2f);
  auVar15 = _pcpyld(auVar21,auVar47);
  DAT_0088cb20._0_4_ = auVar15._0_4_;
  DAT_0088cb20._4_4_ = auVar15._4_4_;
  DAT_0088cb28._0_4_ = auVar15._8_4_;
  DAT_0088cb28._4_4_ = auVar15._12_4_;
  *DAT_00764974 = (unsigned int)DAT_0088cb20;
  puVar12[9] = auVar15._4_4_;
  puVar12[10] = auVar15._8_4_;
  puVar12[0xb] = auVar15._12_4_;
  auVar5._8_8_ = 0;
  auVar5._0_8_ = auVar15._8_8_;
  auVar22._8_8_ = auVar21._8_8_;
  auVar22._0_8_ = 0x6c0103fb01000404;
  auVar15 = _pcpyld(auVar22,auVar5 << 0x40);
  puVar12[0xc] = auVar15._0_4_;
  puVar12[0xd] = auVar15._4_4_;
  puVar12[0xe] = auVar15._8_4_;
  puVar12[0xf] = auVar15._12_4_;
  DAT_00764974 = DAT_00764974 + 8;
  if ((uVar44 & 0x10) == 0) {
    uVar13 = 0x3b808081;
  }
  else {
    uVar13 = 0x3b011b50;
  }
  FUN_003cc560(uVar13,param_1 + 0x44);
  puVar12 = DAT_00764974;
  auVar9._8_8_ = 0;
  auVar9._0_8_ = auVar47._8_8_;
  auVar23._8_8_ = extraout_v0_udw_01;
  auVar23._0_8_ = 0x6c0103fc01000404;
  auVar15 = _pcpyld(auVar23,auVar9 << 0x40);
  *DAT_00764974 = auVar15._0_4_;
  puVar12[1] = auVar15._4_4_;
  puVar12[2] = auVar15._8_4_;
  puVar12[3] = auVar15._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  uVar34 = auVar15._8_8_;
  FUN_003cc680(*(unsigned int *)(param_1 + 0x1c),*(unsigned int *)(param_1 + 0x3c));
  puVar12 = DAT_00764974;
  bVar35 = *(unsigned char *)(param_1 + 0x28);
  auVar10._8_8_ = 0;
  auVar10._0_8_ = uVar53;
  auVar38._8_8_ = uVar34;
  auVar38._0_8_ = 0x6c0303fd01000404;
  auVar47._8_8_ = (unsigned long long)bVar35 & 7;
  auVar15 = _pcpyld(auVar38,auVar10 << 0x40);
  *DAT_00764974 = auVar15._0_4_;
  puVar12[1] = auVar15._4_4_;
  puVar12[2] = auVar15._8_4_;
  puVar12[3] = auVar15._12_4_;
  auVar46 = _DAT_0088cbb0;
  auVar15 = _DAT_0088cb90;
  puVar12 = DAT_00764974;
  if (((DAT_00764a84 != 0) && ((bVar35 & 4) == 0)) || ((DAT_00763d00 != 0 && ((bVar35 & 4) != 0))))
  {
    auVar47._8_8_ = auVar47._8_8_ | 8;
    DAT_00764974[4] = DAT_0088cbb0;
    puVar12[5] = auVar46._4_4_;
    puVar12[6] = auVar46._8_4_;
    puVar12[7] = auVar46._12_4_;
    auVar15 = _DAT_0088cbc0;
    puVar12[8] = DAT_0088cbc0;
    puVar12[9] = auVar15._4_4_;
    puVar12[10] = auVar15._8_4_;
    puVar12[0xb] = auVar15._12_4_;
    uVar44 = auVar15._8_8_;
    auVar15 = auVar46;
    puVar12 = DAT_00764974;
  }
  else {
    DAT_00764974[4] = DAT_0088cb90;
    puVar12[5] = auVar15._4_4_;
    puVar12[6] = auVar15._8_4_;
    puVar12[7] = auVar15._12_4_;
    auVar46 = _DAT_0088cba0;
    puVar12[8] = DAT_0088cba0;
    puVar12[9] = auVar46._4_4_;
    puVar12[10] = auVar46._8_4_;
    puVar12[0xb] = auVar46._12_4_;
    uVar44 = auVar46._8_8_;
    puVar12 = DAT_00764974;
  }
  DAT_00764974 = puVar12 + 0xc;
  DAT_00764a7c = 0;
  auVar48._8_8_ = auVar15._8_8_;
  uVar45 = 0;
  if (DAT_00764a2c == 3) {
    uVar45 = 0x20;
    DAT_00764a7c = 0x20;
  }
  auVar24._8_8_ = extraout_v0_udw_02;
  auVar24._0_8_ = auVar47._8_8_;
  auVar48._0_8_ = ((unsigned long long)DAT_00764a80 << 0x20) | uVar45;
  auVar15 = _pcpyld(auVar48,auVar24);
  *DAT_00764974 = auVar15._0_4_;
  puVar12[0xd] = auVar15._4_4_;
  puVar12[0xe] = auVar15._8_4_;
  puVar12[0xf] = auVar15._12_4_;
  puVar12 = DAT_00764974;
  puVar36 = DAT_00764974 + 4;
  auVar49._8_8_ = auVar15._8_8_;
  DAT_00764974 = puVar36;
  if (*(int *)(param_1 + 0x40) != 0) {
    auVar6._8_8_ = 0;
    auVar6._0_8_ = auVar49._8_8_;
    auVar25._8_8_ = extraout_v0_udw_02;
    auVar25._0_8_ = 0x5000000400000000;
    auVar15 = _pcpyld(auVar25,auVar6 << 0x40);
    auVar49._0_8_ = 0xe;
    *puVar36 = auVar15._0_4_;
    puVar12[5] = auVar15._4_4_;
    puVar12[6] = auVar15._8_4_;
    puVar12[7] = auVar15._12_4_;
    auVar26._8_8_ = auVar15._8_8_;
    auVar26._0_8_ = 0x1000000000008003;
    auVar15 = _pcpyld(auVar49,auVar26);
    puVar12[8] = auVar15._0_4_;
    puVar12[9] = auVar15._4_4_;
    puVar12[10] = auVar15._8_4_;
    puVar12[0xb] = auVar15._12_4_;
    auVar39._8_8_ = uVar44;
    auVar39._0_8_ = DAT_007649c0;
    auVar27._8_8_ = auVar15._8_8_;
    auVar27._0_8_ = 0x47;
    auVar15 = _pcpyld(auVar27,auVar39);
    puVar12[0xc] = auVar15._0_4_;
    puVar12[0xd] = auVar15._4_4_;
    puVar12[0xe] = auVar15._8_4_;
    puVar12[0xf] = auVar15._12_4_;
    auVar40._8_8_ = uVar44;
    auVar40._0_8_ = DAT_007649e0;
    auVar28._8_8_ = auVar15._8_8_;
    auVar28._0_8_ = 0x14;
    auVar15 = _pcpyld(auVar28,auVar40);
    puVar12[0x10] = auVar15._0_4_;
    puVar12[0x11] = auVar15._4_4_;
    puVar12[0x12] = auVar15._8_4_;
    puVar12[0x13] = auVar15._12_4_;
    auVar41._8_8_ = uVar44;
    auVar41._0_8_ = DAT_007649d8;
    auVar29._8_8_ = auVar15._8_8_;
    auVar29._0_8_ = 8;
    auVar15 = _pcpyld(auVar29,auVar41);
    puVar12[0x14] = auVar15._0_4_;
    puVar12[0x15] = auVar15._4_4_;
    puVar12[0x16] = auVar15._8_4_;
    puVar12[0x17] = auVar15._12_4_;
    uVar34 = auVar15._8_8_;
  }
  else {
    *puVar36 = in_zero_lo;
    puVar12[5] = in_zero_hi;
    puVar12[6] = in_zero_udw;
    puVar12[7] = in_zero_udw_00;
    auVar32._8_8_ = extraout_v0_udw_02;
    auVar32._0_8_ = 0x5000000200000000;
    auVar11._8_8_ = 0;
    auVar11._0_8_ = uVar44;
    puVar12[8] = in_zero_lo;
    puVar12[9] = in_zero_hi;
    puVar12[10] = in_zero_udw;
    puVar12[0xb] = in_zero_udw_00;
    auVar15 = _pcpyld(auVar32,auVar11 << 0x40);
    puVar12[0xc] = auVar15._0_4_;
    puVar12[0xd] = auVar15._4_4_;
    puVar12[0xe] = auVar15._8_4_;
    puVar12[0xf] = auVar15._12_4_;
    auVar50._8_8_ = auVar49._8_8_;
    auVar50._0_8_ = 0xe;
    auVar33._8_8_ = auVar15._8_8_;
    auVar42._8_8_ = uVar44;
    auVar42._0_8_ = 0x1000000000008001;
    auVar15 = _pcpyld(auVar50,auVar42);
    puVar12[0x10] = auVar15._0_4_;
    puVar12[0x11] = auVar15._4_4_;
    puVar12[0x12] = auVar15._8_4_;
    puVar12[0x13] = auVar15._12_4_;
    auVar33._0_8_ = 0x47;
    auVar43._8_8_ = auVar15._8_8_;
    auVar43._0_8_ = DAT_007649c0;
    auVar15 = _pcpyld(auVar33,auVar43);
    puVar12[0x14] = auVar15._0_4_;
    puVar12[0x15] = auVar15._4_4_;
    puVar12[0x16] = auVar15._8_4_;
    puVar12[0x17] = auVar15._12_4_;
    uVar34 = auVar15._8_8_;
  }
  DAT_00764974 = DAT_00764974 + 0x14;
  auVar51._0_8_ = (long)(int)puVar36;
  auVar51._8_8_ = uVar53;
  *(unsigned char *)(param_1 + 0x2b) =
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x20) >> 5) |
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x18) >> 2);
  puVar12 = DAT_00764974;
  uVar2 = *(unsigned int *)(*(int *)(*(int *)(param_1 + 4) + 0x18) + (unsigned int)*(unsigned char *)(param_1 + 0x2b) * 4);
  if (DAT_00764a78 != uVar2) {
    auVar7._8_8_ = 0;
    auVar7._0_8_ = uVar53;
    auVar51 = auVar7 << 0x40;
    auVar30._0_8_ = ((unsigned long long)uVar2 << 0x20) | 0x50000000;
    auVar30._8_8_ = uVar34;
    auVar15 = _pcpyld(auVar51,auVar30);
    *DAT_00764974 = auVar15._0_4_;
    puVar12[1] = auVar15._4_4_;
    puVar12[2] = auVar15._8_4_;
    puVar12[3] = auVar15._12_4_;
    DAT_00764974 = DAT_00764974 + 4;
    DAT_00764a78 = uVar2;
  }
  puVar12 = DAT_00764974;
  auVar52._8_8_ = auVar51._8_8_;
  iVar56 = **(int **)(param_1 + 0x30);
  auVar52._0_8_ = (((long)*(int *)(*(int *)(param_1 + 4) + 0x14) | 0x2000000U) << 0x20) | 0x3000000;
  auVar31._0_8_ = ((unsigned long long)*(unsigned int *)(iVar56 + 0x20) << 0x20) | 0x50000000;
  auVar31._8_8_ = uVar34;
  auVar15 = _pcpyld(auVar52,auVar31);
  *DAT_00764974 = auVar15._0_4_;
  puVar12[1] = auVar15._4_4_;
  puVar12[2] = auVar15._8_4_;
  puVar12[3] = auVar15._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  do {
    DI();
    SYNC(0x10);
  } while ((Status & 0x10000) != 0);
  *(int *)(iVar56 + 0x18) = *(int *)(iVar56 + 0x18) + 1;
  EI();
  *(int *)(iVar56 + 0x1c) = *(int *)(iVar56 + 0x1c) + 1;
  if (*(int *)(iVar56 + 0x1c) == 1) {
    FUN_003f36a0();
  }
  return 1;
}
