/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d2300 func_003d2300 src/promoted/code1_003d.c - romwright draft, unreviewed */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

unsigned long long FUN_003d2300(int param_1)

{
  unsigned char bVar1;
  unsigned int uVar2;
  int iVar3;
  unsigned char bVar4;
  unsigned char auVar5 [16];
  unsigned char auVar6 [16];
  unsigned char auVar7 [16];
  unsigned char auVar8 [16];
  unsigned char auVar9 [16];
  unsigned int *puVar10;
  unsigned int uVar11;
  unsigned int uVar12;
  unsigned char in_zero_qw [16];
  unsigned int uVar13;
  unsigned long long uVar14;
  unsigned long long extraout_v0_udw;
  unsigned char auVar15 [16];
  unsigned char auVar16 [16];
  unsigned char auVar17 [16];
  unsigned char auVar18 [16];
  unsigned long long extraout_v0_udw_00;
  unsigned char auVar19 [16];
  unsigned long long extraout_v0_udw_01;
  unsigned char auVar20 [16];
  unsigned char auVar21 [16];
  unsigned char auVar22 [16];
  unsigned long long in_v1_udw;
  unsigned long long uVar24;
  unsigned char auVar23 [16];
  unsigned long long uVar25;
  unsigned long long in_a0_udw;
  unsigned char auVar26 [16];
  unsigned char auVar27 [16];
  unsigned long long in_a1_udw;
  unsigned char auVar28 [16];
  unsigned char auVar29 [16];
  unsigned char auVar30 [16];
  unsigned char auVar31 [16];
  
  FUN_003f3eb0(0,0xd);
  puVar10 = DAT_00764974;
  auVar16._8_8_ = 0;
  auVar16._0_8_ = in_v1_udw;
  auVar30._8_8_ = extraout_v0_udw;
  auVar30._0_8_ = 0x1000000a;
  auVar30 = _pcpyld(auVar16 << 0x40,auVar30);
  auVar6._8_8_ = 0;
  auVar6._0_8_ = in_a1_udw;
  auVar15._8_8_ = extraout_v0_udw;
  auVar15._0_8_ = 0x6c0103fa01000404;
  *DAT_00764974 = auVar30._0_4_;
  puVar10[1] = auVar30._4_4_;
  puVar10[2] = auVar30._8_4_;
  puVar10[3] = auVar30._12_4_;
  auVar16 = _pcpyld(auVar15,auVar6 << 0x40);
  puVar10[4] = auVar16._0_4_;
  puVar10[5] = auVar16._4_4_;
  puVar10[6] = auVar16._8_4_;
  puVar10[7] = auVar16._12_4_;
  puVar10 = DAT_00764974;
  auVar17._8_8_ = auVar16._8_8_;
  DAT_00764974 = DAT_00764974 + 8;
  bVar4 = *(char *)(param_1 + 0x47) != -1;
  DAT_00764a34 = (unsigned int)bVar4;
  if (bVar4 || DAT_00764a38 != 0) {
    DAT_00763cd8 = DAT_00763cd8 | 0x40;
  }
  else {
    DAT_00763cd8 = DAT_00763cd8 & 0xffffffffffffffbf;
  }
  uVar14 = DAT_00763cd8;
  auVar31._8_8_ = auVar30._8_8_;
  auVar31._0_8_ = 0xfc007fffffffffff;
  auVar17._4_4_ = DAT_0088cb28._4_4_;
  auVar17._0_4_ = (unsigned int)DAT_0088cb28;
  auVar16 = _pcpyld(in_zero_qw,auVar31);
  auVar26._0_8_ =
       (CONCAT44(DAT_0088cb20._4_4_,(unsigned int)DAT_0088cb20) & auVar16._0_8_) |
       (((long)(int)((unsigned int)DAT_00763cd8 | (unsigned int)*(unsigned char *)(param_1 + 0x29)) & 0x7ffU) << 0x2f);
  auVar26._8_8_ = in_a0_udw;
  auVar16 = _pcpyld(auVar17,auVar26);
  DAT_0088cb20._0_4_ = auVar16._0_4_;
  DAT_0088cb20._4_4_ = auVar16._4_4_;
  DAT_0088cb28._0_4_ = auVar16._8_4_;
  DAT_0088cb28._4_4_ = auVar16._12_4_;
  *DAT_00764974 = (unsigned int)DAT_0088cb20;
  puVar10[9] = auVar16._4_4_;
  puVar10[10] = auVar16._8_4_;
  puVar10[0xb] = auVar16._12_4_;
  auVar5._8_8_ = 0;
  auVar5._0_8_ = auVar16._8_8_;
  auVar18._8_8_ = auVar17._8_8_;
  auVar18._0_8_ = 0x6c0103fb01000404;
  auVar16 = _pcpyld(auVar18,auVar5 << 0x40);
  puVar10[0xc] = auVar16._0_4_;
  puVar10[0xd] = auVar16._4_4_;
  puVar10[0xe] = auVar16._8_4_;
  puVar10[0xf] = auVar16._12_4_;
  DAT_00764974 = DAT_00764974 + 8;
  if ((uVar14 & 0x10) == 0) {
    uVar13 = 0x3b808081;
  }
  else {
    uVar13 = 0x3b011b50;
  }
  FUN_003cc560(uVar13,param_1 + 0x44);
  puVar10 = DAT_00764974;
  auVar7._8_8_ = 0;
  auVar7._0_8_ = in_a0_udw;
  auVar19._8_8_ = extraout_v0_udw_00;
  auVar19._0_8_ = 0x6c0103fc01000404;
  auVar16 = _pcpyld(auVar19,auVar7 << 0x40);
  *DAT_00764974 = auVar16._0_4_;
  puVar10[1] = auVar16._4_4_;
  puVar10[2] = auVar16._8_4_;
  puVar10[3] = auVar16._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  uVar24 = auVar16._8_8_;
  FUN_003cc680(*(unsigned int *)(param_1 + 0x1c),*(unsigned int *)(param_1 + 0x3c));
  puVar10 = DAT_00764974;
  bVar1 = *(unsigned char *)(param_1 + 0x28);
  auVar8._8_8_ = 0;
  auVar8._0_8_ = in_a1_udw;
  auVar23._8_8_ = uVar24;
  auVar23._0_8_ = 0x6c0303fd01000404;
  uVar14 = (unsigned long long)bVar1 & 7;
  auVar16 = _pcpyld(auVar23,auVar8 << 0x40);
  *DAT_00764974 = auVar16._0_4_;
  puVar10[1] = auVar16._4_4_;
  puVar10[2] = auVar16._8_4_;
  puVar10[3] = auVar16._12_4_;
  auVar6 = _DAT_0088cbb0;
  auVar16 = _DAT_0088cb90;
  puVar10 = DAT_00764974;
  if (((DAT_00764a84 != 0) && ((bVar1 & 4) == 0)) || ((DAT_00763d00 != 0 && ((bVar1 & 4) != 0)))) {
    uVar14 = uVar14 | 8;
    DAT_00764974[4] = DAT_0088cbb0;
    puVar10[5] = auVar6._4_4_;
    puVar10[6] = auVar6._8_4_;
    puVar10[7] = auVar6._12_4_;
    uVar12 = DAT_0088cbcc;
    uVar11 = DAT_0088cbc8;
    uVar13 = DAT_0088cbc4;
    puVar10[8] = DAT_0088cbc0;
    puVar10[9] = uVar13;
    puVar10[10] = uVar11;
    puVar10[0xb] = uVar12;
    auVar16 = auVar6;
    puVar10 = DAT_00764974;
  }
  else {
    DAT_00764974[4] = DAT_0088cb90;
    puVar10[5] = auVar16._4_4_;
    puVar10[6] = auVar16._8_4_;
    puVar10[7] = auVar16._12_4_;
    uVar12 = DAT_0088cbac;
    uVar11 = DAT_0088cba8;
    uVar13 = DAT_0088cba4;
    puVar10[8] = DAT_0088cba0;
    puVar10[9] = uVar13;
    puVar10[10] = uVar11;
    puVar10[0xb] = uVar12;
    puVar10 = DAT_00764974;
  }
  DAT_00764974 = puVar10 + 0xc;
  DAT_00764a7c = 0;
  auVar27._8_8_ = auVar16._8_8_;
  uVar25 = 0;
  if (DAT_00764a2c == 3) {
    uVar25 = 0x20;
    DAT_00764a7c = 0x20;
  }
  auVar20._8_8_ = extraout_v0_udw_01;
  auVar20._0_8_ = uVar14;
  auVar28._4_8_ = in_a1_udw;
  auVar28._0_4_ = DAT_00764a80;
  auVar28._12_4_ = 0;
  auVar28 = auVar28 << 0x20;
  auVar27._0_8_ = ((unsigned long long)DAT_00764a80 << 0x20) | uVar25;
  auVar16 = _pcpyld(auVar27,auVar20);
  *DAT_00764974 = auVar16._0_4_;
  puVar10[0xd] = auVar16._4_4_;
  puVar10[0xe] = auVar16._8_4_;
  puVar10[0xf] = auVar16._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  *(unsigned char *)(param_1 + 0x2b) =
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x20) >> 5) |
       (unsigned char)((int)(*(unsigned char *)(param_1 + 0x28) & 0x18) >> 2);
  puVar10 = DAT_00764974;
  uVar2 = *(unsigned int *)(*(int *)(*(int *)(param_1 + 4) + 0x18) + (unsigned int)*(unsigned char *)(param_1 + 0x2b) * 4);
  if (DAT_00764a78 != uVar2) {
    auVar9._8_8_ = 0;
    auVar9._0_8_ = in_a1_udw;
    auVar28 = auVar9 << 0x40;
    auVar21._0_8_ = ((unsigned long long)uVar2 << 0x20) | 0x50000000;
    auVar21._8_8_ = extraout_v0_udw_01;
    auVar16 = _pcpyld(auVar28,auVar21);
    *DAT_00764974 = auVar16._0_4_;
    puVar10[1] = auVar16._4_4_;
    puVar10[2] = auVar16._8_4_;
    puVar10[3] = auVar16._12_4_;
    DAT_00764974 = DAT_00764974 + 4;
    DAT_00764a78 = uVar2;
  }
  auVar29._8_8_ = auVar28._8_8_;
  *(unsigned int *)(**(int **)(param_1 + 0x30) + 0x1c) = 1;
  puVar10 = DAT_00764974;
  iVar3 = **(int **)(param_1 + 0x30);
  auVar29._0_8_ = (((long)*(int *)(*(int *)(param_1 + 4) + 0x14) | 0x2000000U) << 0x20) | 0x3000000;
  auVar22._0_8_ = ((unsigned long long)*(unsigned int *)(iVar3 + 0x20) << 0x20) | 0x50000000;
  auVar22._8_8_ = extraout_v0_udw_01;
  auVar16 = _pcpyld(auVar29,auVar22);
  *DAT_00764974 = auVar16._0_4_;
  puVar10[1] = auVar16._4_4_;
  puVar10[2] = auVar16._8_4_;
  puVar10[3] = auVar16._12_4_;
  DAT_00764974 = DAT_00764974 + 4;
  do {
    DI();
    SYNC(0x10);
  } while ((Status & 0x10000) != 0);
  *(int *)(iVar3 + 0x18) = *(int *)(iVar3 + 0x18) + 1;
  EI();
  *(int *)(iVar3 + 0x1c) = *(int *)(iVar3 + 0x1c) + 1;
  if (*(int *)(iVar3 + 0x1c) == 1) {
    FUN_003f36a0();
  }
  return 1;
}
