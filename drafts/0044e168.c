/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0044e168 func_0044e168 src/middleware/soft_float.c - romwright draft, unreviewed */

void FUN_0044e168(unsigned long long param_1,unsigned long long param_2)

{
  unsigned long long uVar1;
  unsigned char auStack_80 [32];
  unsigned char auStack_60 [32];
  unsigned char auStack_40 [32];
  unsigned long long uStack_20;
  unsigned long long uStack_18;
  
  uStack_20 = param_1;
  uStack_18 = param_2;
  FUN_0044ddf0(&uStack_20,auStack_80);
  FUN_0044ddf0(&uStack_18,auStack_60);
  uVar1 = FUN_0044e1c0(auStack_80,auStack_60,auStack_40);
  FUN_0044dd18(uVar1);
  return;
}
