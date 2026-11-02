/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00447b88 func_00447b88 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

unsigned long long FUN_00447b88(unsigned long long param_1)

{
  unsigned long long uVar1;
  unsigned long long uVar2;
  unsigned long long uVar3;
  unsigned long long uVar4;
  unsigned long long uVar5;
  int iVar6;
  int iVar7;
  
  uVar5 = (long)param_1 >> 0x20;
  iVar6 = 0;
  if ((long)uVar5 < 0x100000) {
    if ((uVar5 & 0x7fffffff) == 0 && (int)param_1 == 0) {
      return DAT_00755970;
    }
    if ((long)uVar5 < 0) {
      uVar1 = FUN_0044e100(param_1,param_1);
      uVar1 = FUN_0044e3b8(uVar1,0);
      return uVar1;
    }
    iVar6 = -0x36;
    param_1 = FUN_0044e168(param_1,0x4350000000000000);
    uVar5 = (long)param_1 >> 0x20;
  }
  uVar4 = param_1;
  if ((long)uVar5 < 0x7ff00000) {
    iVar7 = (iVar6 + ((int)uVar5 >> 0x14)) - 0x3ff;
    iVar6 = iVar7 >> 0x1f;
    uVar1 = FUN_0044e658(iVar7 - iVar6);
    uVar2 = FUN_0044e168(uVar1,DAT_00755978);
    uVar3 = FUN_004476b0((param_1 & 0xffffffff) |
                         (((uVar5 & 0xfffff) | (unsigned long long)(unsigned int)((iVar6 + 0x3ff) * 0x100000)) << 0x20));
    uVar3 = FUN_0044e168(uVar3,DAT_00755980);
    param_1 = FUN_0044e0a8(uVar2,uVar3);
    uVar4 = FUN_0044e168(uVar1,DAT_00755988);
  }
  uVar1 = FUN_0044e0a8(param_1,uVar4);
  return uVar1;
}
