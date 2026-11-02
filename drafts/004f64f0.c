/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f64f0 func_004f64f0 src/sce/rofs_pfs.c - romwright draft, unreviewed */

int FUN_004f64f0(long param_1,unsigned long long param_2,unsigned long long param_3,long param_4)

{
  int iVar1;
  int iVar2;
  long lVar3;
  long lVar4;
  
  if ((param_1 == 0) || (param_4 == 0)) {
    iVar1 = FUN_004f53c0(0x75da28,299,0x75daa0,0,0xffffffffffffff9c);
    return iVar1;
  }
  lVar3 = FUN_004f63c0(param_1,299,param_3,0);
  if (lVar3 < 1) {
    iVar1 = FUN_004f53c0(0x75da28,0x130,0x75da70);
    return iVar1;
  }
  lVar4 = FUN_004f6948(param_1,param_2,lVar3,0,0xffffffffffffff36);
  if (lVar4 != 0) {
    iVar1 = FUN_004f53c0(0x75da28,0x136,0x75dab0);
    return iVar1;
  }
  if (lVar3 == 0) {
    trap(7);
  }
  iVar2 = FUN_004f6290(param_1,((int)param_3 << 0xb) / (int)lVar3,param_4,0,0);
  iVar1 = iVar2 * (int)lVar3;
  if (0 < iVar2) {
    if (iVar1 < 0) {
      iVar1 = iVar1 + 0x7ff;
    }
    iVar2 = iVar1 >> 0xb;
  }
  return iVar2;
}
