/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f50b0 func_004f50b0 src/sce/rofs_mai.c - romwright draft, unreviewed */

int FUN_004f50b0(unsigned long long param_1,int *param_2,int param_3)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  long lVar4;
  int *piVar5;
  int iVar6;
  
  if ((-1 < param_3) && (param_2 != (int *)0x0)) {
    lVar4 = FUN_004f2e80(param_1,1);
    if (lVar4 == 0) {
      iVar3 = FUN_004f53c0(0x75d6c8,0x7b7,0x75d798,0,0xffffffffffffff97);
      return iVar3;
    }
    piVar5 = *(int **)((int)lVar4 + 0x10);
    iVar3 = *piVar5;
    if (param_3 < iVar3) {
      iVar3 = param_3;
    }
    piVar5 = piVar5 + 6;
    iVar6 = iVar3;
    if (0 < iVar3) {
      do {
        iVar2 = *piVar5;
        param_2[2] = (int)piVar5 + 0xe;
        iVar6 = iVar6 - 1;
        *param_2 = iVar2;
        param_2[1] = piVar5[1];
        piVar1 = piVar5 + 3;
        piVar5 = piVar5 + 0xc;
        *(char *)(param_2 + 3) = (char)*piVar1;
        param_2 = param_2 + 4;
      } while (iVar6 != 0);
    }
    return iVar3;
  }
  iVar3 = FUN_004f53c0(0x75d6c8,0x7b3,0x75d8e8,0,0xffffffffffffff9c);
  return iVar3;
}
