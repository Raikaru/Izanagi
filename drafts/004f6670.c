/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f6670 func_004f6670 src/sce/rofs_pfs.c - romwright draft, unreviewed */

int FUN_004f6670(unsigned long long param_1,unsigned long long param_2,long param_3)

{
  int iVar1;
  long lVar2;
  long lVar3;
  
  iVar1 = 0;
  if (0 < param_3) {
    lVar2 = FUN_004f64f0();
    if (lVar2 < 0) {
      iVar1 = FUN_004f53c0(0x75da28);
      return iVar1;
    }
    lVar3 = FUN_004f68b8(param_1,0x152,0x75daa0,0,lVar2);
    if (lVar3 != 1) {
      iVar1 = FUN_004f53c0(0x75da28,0x156,0x75dac0,0,0xffffffffffffff31);
      return iVar1;
    }
    iVar1 = (int)lVar2 << 0xb;
  }
  return iVar1;
}
