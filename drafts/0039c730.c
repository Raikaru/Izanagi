/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039c730 func_0039c730 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_0039c730(int param_1)

{
  unsigned char bVar1;
  int iVar2;
  unsigned char auVar3 [16];
  unsigned long long uVar4;
  unsigned char auVar5 [16];
  unsigned char auVar6 [16];
  unsigned char auVar7 [16];
  int *piVar8;
  unsigned int uVar9;
  unsigned long long in_v0_udw;
  int iVar10;
  unsigned char *pbVar11;
  unsigned int uVar12;
  unsigned long long in_v1_udw;
  unsigned char auVar13 [16];
  unsigned char auVar14 [16];
  unsigned long long in_a0_udw;
  unsigned char auVar15 [16];
  unsigned long long in_a1_udw;
  unsigned int uVar16;
  unsigned char auVar17 [16];
  unsigned long long uVar18;
  unsigned long long *puVar19;
  unsigned int uVar20;
  unsigned int uVar21;
  unsigned int *puVar22;
  unsigned int uVar23;
  unsigned int uVar24;
  unsigned int uVar25;
  unsigned int uVar26;
  unsigned int uVar27;
  unsigned int uVar28;
  unsigned int uVar29;
  unsigned int uVar30;
  unsigned int uVar31;
  unsigned int uVar32;
  unsigned int uVar33;
  unsigned int uVar34;
  unsigned int uVar35;
  
  piVar8 = DAT_008864b8;
  iVar10 = *(int *)(param_1 + 0x2c) - (*(int *)(param_1 + 0xc) + 0x10);
  auVar17._0_8_ = (long)-(iVar10 >> 0x1f);
  auVar17._8_8_ = in_v1_udw;
  iVar2 = *DAT_008864b8;
  auVar15._8_8_ = 0;
  auVar15._0_8_ = in_a0_udw;
  auVar15 = auVar15 << 0x40;
  uVar35 = iVar2 * 4;
  FUN_003f3eb0(0,uVar35 + 3);
  puVar22 = DAT_00764974;
  if (uVar35 + 1 != 0) {
    auVar14._8_8_ = auVar17._8_8_;
    auVar14._0_8_ = (unsigned long long)(uVar35 + 1) | 0x10000000;
    auVar5._8_8_ = 0;
    auVar5._0_8_ = auVar15._8_8_;
    auVar15 = auVar5 << 0x40;
    auVar17 = _pcpyld(auVar15,auVar14);
    *DAT_00764974 = auVar17._0_4_;
    puVar22[1] = auVar17._4_4_;
    puVar22[2] = auVar17._8_4_;
    puVar22[3] = auVar17._12_4_;
    DAT_00764974 = DAT_00764974 + 4;
  }
  auVar13._8_8_ = auVar17._8_8_;
  auVar13._0_8_ = (long)(int)((uVar35 & 0xff) << 0x10) | 0x6c0002bc;
  auVar6._8_8_ = 0;
  auVar6._0_8_ = auVar15._8_8_;
  auVar3._8_8_ = in_v0_udw;
  auVar3._0_8_ = 0x1000101;
  auVar17 = _ppacw(auVar13,auVar3);
  pbVar11 = (unsigned char *)(piVar8[4] + (iVar10 / 0xc) * 2);
  bVar1 = *pbVar11;
  auVar7._8_8_ = 0;
  auVar7._0_8_ = in_a1_udw;
  auVar15 = _ppacw(auVar7 << 0x40,auVar6 << 0x40);
  uVar35 = (unsigned int)pbVar11[1];
  auVar15 = _ppacw(auVar17,auVar15);
  *DAT_00764974 = auVar15._0_4_;
  DAT_00764974[1] = auVar15._4_4_;
  DAT_00764974[2] = auVar15._8_4_;
  DAT_00764974[3] = auVar15._12_4_;
  iVar10 = (unsigned int)bVar1 << 1;
  uVar18 = 0;
  if (uVar35 != 0) {
    do {
      uVar12 = (unsigned int)((unsigned char *)(piVar8[5] + iVar10))[1];
      uVar9 = (unsigned int)*(unsigned char *)(piVar8[5] + iVar10);
      uVar16 = 0;
      if (uVar12 != 0) {
        do {
          puVar19 = (unsigned long long *)(DAT_0088649c + (uVar9 + uVar16) * 0x40);
          uVar31 = *(unsigned int *)(puVar19 + 2);
          uVar32 = *(unsigned int *)((int)puVar19 + 0x14);
          uVar33 = *(unsigned int *)(puVar19 + 3);
          uVar34 = *(unsigned int *)((int)puVar19 + 0x1c);
          uVar27 = *(unsigned int *)(puVar19 + 4);
          uVar28 = *(unsigned int *)((int)puVar19 + 0x24);
          uVar29 = *(unsigned int *)(puVar19 + 5);
          uVar30 = *(unsigned int *)((int)puVar19 + 0x2c);
          uVar23 = *(unsigned int *)(puVar19 + 6);
          uVar24 = *(unsigned int *)((int)puVar19 + 0x34);
          uVar25 = *(unsigned int *)(puVar19 + 7);
          uVar26 = *(unsigned int *)((int)puVar19 + 0x3c);
          pbVar11 = (unsigned char *)(uVar9 + piVar8[3] + uVar16);
          uVar16 = uVar16 + 1;
          uVar4 = *puVar19;
          uVar20 = *(unsigned int *)(puVar19 + 1);
          uVar21 = *(unsigned int *)((int)puVar19 + 0xc);
          puVar22 = DAT_00764974 + (unsigned int)*pbVar11 * 0x10 + 4;
          *puVar22 = (int)uVar4;
          puVar22[1] = (int)((unsigned long long)uVar4 >> 0x20);
          puVar22[2] = uVar20;
          puVar22[3] = uVar21;
          puVar22[4] = uVar31;
          puVar22[5] = uVar32;
          puVar22[6] = uVar33;
          puVar22[7] = uVar34;
          puVar22[8] = uVar27;
          puVar22[9] = uVar28;
          puVar22[10] = uVar29;
          puVar22[0xb] = uVar30;
          puVar22[0xc] = uVar23;
          puVar22[0xd] = uVar24;
          puVar22[0xe] = uVar25;
          puVar22[0xf] = uVar26;
        } while (uVar16 < uVar12);
      }
      uVar18 = (unsigned long long)((int)uVar18 + 1);
      iVar10 = iVar10 + 2;
    } while (uVar18 < uVar35);
  }
  DAT_00764974 = DAT_00764974 + iVar2 * 0x10 + 4;
  return 1;
}
