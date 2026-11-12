/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f5020 func_004f5020 src/sce/rofs_mai.c - romwright draft, unreviewed */

int FUN_004f5020(unsigned long long param_1)

{
  int iVar1;
  long lVar2;
  
  lVar2 = FUN_004f2e80(param_1,1);
  if (lVar2 == 0) {
    iVar1 = FUN_004f53c0(0x75d6c8,0x79d,0x75d798,0,0xffffffffffffff97);
    return iVar1;
  }
  if (*(int *)lVar2 != 0) {
    return *(int *)((int *)lVar2)[4] - 2;
  }
  iVar1 = FUN_004f53c0(0x75d6c8,0x7a0,0x75d8d0,0,0xffffffffffffff97);
  return iVar1;
}
