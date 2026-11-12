/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4a40 func_004f4a40 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned long long FUN_004f4a40(unsigned long long param_1,long param_2)

{
  int iVar1;
  unsigned long long uVar2;
  long lVar3;
  int *piVar4;
  
  if (param_2 == 0) {
    uVar2 = FUN_004f53c0(0x75d6c8,0x6b1,0x75d870,0,0xffffffffffffff9c);
    return uVar2;
  }
  lVar3 = FUN_004f2e80(param_1,1);
  uVar2 = 0;
  piVar4 = (int *)param_2;
  if (lVar3 == 0) {
    piVar4[1] = 0;
    uVar2 = 0xffffffffffffff97;
    *piVar4 = 0;
  }
  else {
    iVar1 = *(int *)lVar3;
    *piVar4 = (int)(int *)lVar3 + 0x16;
    piVar4[1] = iVar1;
  }
  return uVar2;
}
