/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f6750 func_004f6750 src/sce/rofs_pfs.c - romwright draft, unreviewed */

int FUN_004f6750(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,long param_4)

{
  int iVar1;
  long lVar2;
  long lVar3;
  
  iVar1 = 0;
  if (0 < param_4) {
    lVar2 = FUN_004f6728();
    if (lVar2 < 0) {
      iVar1 = FUN_004f53c0(0x75da28);
      return iVar1;
    }
    lVar3 = FUN_004f68b8(param_2,0x17e,0x75dad0,0,lVar2);
    if (lVar3 != 1) {
      iVar1 = FUN_004f53c0(0x75da28,0x183,0x75dac0,0,0xffffffffffffff31);
      return iVar1;
    }
    iVar1 = (int)lVar2 << 0xb;
  }
  return iVar1;
}
