/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00430f80 func_00430f80 src/promoted/code1_0043.c - romwright draft, unreviewed */

unsigned int *
FUN_00430f80(long param_1,unsigned int param_2,unsigned long long param_3,unsigned long long param_4,unsigned long long param_5,
            unsigned long long param_6,unsigned long long param_7,unsigned long long param_8)

{
  unsigned char bVar1;
  unsigned int *puVar2;
  unsigned int *puVar3;
  unsigned long long *puVar4;
  int iVar5;
  unsigned int uVar6;
  unsigned int *puVar7;
  unsigned int *puVar8;
  unsigned int **ppuVar9;
  unsigned int uVar10;
  unsigned int uVar11;
  unsigned int *puVar12;
  unsigned int *apuStack_f0 [4];
  unsigned long long uStack_30;
  unsigned long long uStack_28;
  unsigned long long uStack_20;
  unsigned long long uStack_18;
  unsigned long long uStack_10;
  unsigned long long uStack_8;
  
  uStack_30 = param_3;
  puVar7 = &DAT_00895284;
  uStack_28 = param_4;
  iVar5 = 5;
  uStack_20 = param_5;
  uStack_18 = param_6;
  uStack_10 = param_7;
  uStack_8 = param_8;
  DAT_00895280 = &DAT_00895280;
  puVar4 = &uStack_30;
  do {
    iVar5 = iVar5 - 1;
    *puVar7 = *(unsigned int *)puVar4;
    puVar3 = DAT_00895284;
    puVar2 = DAT_0070f8e8;
    puVar12 = DAT_0070f8e4;
    puVar8 = DAT_0070f8e0;
    puVar7 = puVar7 + 1;
    puVar4 = puVar4 + 1;
  } while (-1 < iVar5);
  bVar1 = param_1 == 0;
  uVar11 = 0;
  if (bVar1) {
    uVar11 = DAT_0070f8c8;
  }
  if (param_2 == 0x8160) {
    if (DAT_00895284 == (unsigned int *)0x0) {
      DAT_0070f8e0 = DAT_00895288;
      DAT_0070f8ec = DAT_0089528c;
      DAT_0070f8f8 = &LAB_007690f0;
      puVar12 = puVar8;
    }
    else {
      DAT_0070f8e4 = DAT_00895288;
      DAT_0070f8f0 = DAT_0089528c;
      DAT_0070f8fc = &LAB_007690f0;
    }
  }
  else {
    puVar12 = (unsigned int *)0x0;
    if (param_2 == 0x8170) {
      DAT_0070f8e8 = DAT_00895284;
      DAT_0070f8f4 = DAT_00895288;
      DAT_0070f900 = &LAB_007690f0;
      puVar12 = puVar2;
    }
  }
  apuStack_f0[0] = (unsigned int *)0x0;
  if (((param_2 == 0x8130) || (param_2 == 0x81a0)) || (param_2 == 0x81b0)) {
    puVar7 = (unsigned int *)0x40;
    uVar10 = 0x40;
    uVar6 = param_2 | (unsigned int)DAT_00895284;
    puVar8 = DAT_00895288;
  }
  else {
    if (param_2 == 0x8140) {
      FUN_004258b0(0x8952c0,(unsigned int)DAT_00895284 | 0x8140,bVar1,0x895280,0x40,DAT_00895288,0x40,uVar11
                  );
      if ((param_2 == 0x8160) || (param_2 == 0x8170)) {
        apuStack_f0[0] = puVar12;
      }
      return apuStack_f0[0];
    }
    if ((param_2 == 0x81c0) || (param_2 == 0x81d0)) {
      *(unsigned short *)((int)DAT_00895284 + 2) = DAT_00895288._0_2_;
      if (param_2 == 0x81d0) {
        puVar3[1] = DAT_00895294;
      }
      uVar10 = DAT_00895290;
      ppuVar9 = DAT_0089528c;
      if (DAT_0089528c == (unsigned int **)0x0) {
        ppuVar9 = apuStack_f0;
        uVar10 = 4;
      }
      FUN_004258b0(0x8952c0,param_2,bVar1,DAT_00895284,((int)DAT_00895288 + 1) * 8,ppuVar9,uVar10,
                   uVar11);
      if (DAT_0089528c != (unsigned int **)0x0) {
        apuStack_f0[0] = *DAT_0089528c;
      }
      if ((param_2 == 0x8160) || (param_2 == 0x8170)) {
        apuStack_f0[0] = puVar12;
      }
      return apuStack_f0[0];
    }
    puVar7 = DAT_00895288;
    puVar8 = DAT_00895284;
    if (0xf0 < param_2 - 0x9000) {
      puVar7 = (unsigned int *)0x40;
      puVar8 = &DAT_00895280;
    }
    uVar10 = 0x10;
    uVar6 = param_2;
  }
  FUN_004258b0(0x8952c0,uVar6,bVar1,puVar8,puVar7,0x895280,uVar10,uVar11);
  apuStack_f0[0] = DAT_00895280;
  if ((param_2 == 0x8160) || (param_2 == 0x8170)) {
    apuStack_f0[0] = puVar12;
  }
  return apuStack_f0[0];
}
