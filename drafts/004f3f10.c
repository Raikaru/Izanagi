/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f3f10 func_004f3f10 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned long long FUN_004f3f10(long param_1)

{
  unsigned long long uVar1;
  long lVar2;
  
  if (param_1 == 0) {
    uVar1 = FUN_004f53c0(0x75d6c8,0x4b3,0x75d7a8,0,0xffffffffffffff9c);
    return uVar1;
  }
  lVar2 = FUN_004426e8(param_1,0x75d6c0);
  if (lVar2 == 0) {
    uVar1 = FUN_004f53c0(0x75d6c8,0x4b8);
    return uVar1;
  }
  lVar2 = FUN_004f2e80(param_1,0,0x75d7a8,0,0xffffffffffffff99);
  if (lVar2 == 0) {
    uVar1 = FUN_004f53c0(0x75d6c8,0x4be,0x75d798,0,0xffffffffffffff99);
    return uVar1;
  }
  *(int *)(DAT_00743a50 + 0x2c) = (int)lVar2;
  return 0;
}
