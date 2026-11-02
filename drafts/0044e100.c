/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0044e100 dpsub src/middleware/soft_float.c - romwright draft, unreviewed */

void FUN_0044e100(unsigned long long param_1,unsigned long long param_2)

{
  unsigned long long uVar1;
  unsigned char auStack_80 [32];
  unsigned char auStack_60 [4];
  unsigned int uStack_5c;
  unsigned char auStack_40 [32];
  unsigned long long uStack_20;
  unsigned long long uStack_18;
  
  uStack_20 = param_1;
  uStack_18 = param_2;
  FUN_0044ddf0(&uStack_20,auStack_80);
  FUN_0044ddf0(&uStack_18,auStack_60);
  uStack_5c = uStack_5c ^ 1;
  uVar1 = FUN_0044de80(auStack_80,auStack_60,auStack_40);
  FUN_0044dd18(uVar1);
  return;
}
