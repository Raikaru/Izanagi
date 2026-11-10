/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004244c8 func_004244c8 src/promoted/code1_0042.c - romwright draft, unreviewed */

void FUN_004244c8(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4,
                 unsigned long long param_5,unsigned long long param_6,unsigned long long param_7,unsigned long long param_8)

{
  long lVar1;
  unsigned int auStack_c0 [4];
  unsigned long long uStack_38;
  unsigned long long uStack_30;
  unsigned long long uStack_28;
  unsigned long long uStack_20;
  unsigned long long uStack_18;
  unsigned long long uStack_10;
  unsigned long long uStack_8;
  
  auStack_c0[0] = 0;
  uStack_38 = param_2;
  uStack_30 = param_3;
  uStack_28 = param_4;
  uStack_20 = param_5;
  uStack_18 = param_6;
  uStack_10 = param_7;
  uStack_8 = param_8;
  lVar1 = FUN_0042ba20();
  FUN_004235c8(0x424338,auStack_c0,0xffffffffffffffff,param_1,&uStack_38);
  if (lVar1 != 0) {
    FUN_0042ba70();
  }
  return;
}
