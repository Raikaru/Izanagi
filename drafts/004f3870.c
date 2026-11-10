/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f3870 func_004f3870 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned long long FUN_004f3870(long param_1)

{
  unsigned int uVar1;
  unsigned long long uVar2;
  
  if (param_1 == 0) {
    uVar2 = FUN_004f53c0(0x75d6c8,0x3ae,0x75d6e0,0,0xffffffffffffff9c);
    return uVar2;
  }
  *(int *)(DAT_00743a50 + 0x34) = (int)param_1;
  FUN_004f69b8();
  uVar1 = FUN_004f69c8();
  *(unsigned int *)(DAT_00743a50 + 0x38) = uVar1;
  return 0;
}
