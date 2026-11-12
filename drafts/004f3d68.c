/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f3d68 func_004f3d68 src/sce/rofs_mai.c - romwright draft, unreviewed */

void FUN_004f3d68(long param_1)

{
  long lVar1;
  
  if (param_1 == 0) {
    FUN_004f53c0(0x75d6c8,0x47d,0x75d788,0,0xffffffffffffff9c);
    return;
  }
  lVar1 = FUN_004426e8(param_1,0x75d6c0);
  if (lVar1 == 0) {
    FUN_004f53c0(0x75d6c8);
    return;
  }
  FUN_004f3e00(param_1,0x482,0x75d788,0,0xffffffffffffff99);
  return;
}
