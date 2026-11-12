/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00412e90 func_00412e90 src/promoted/code1_0041.c - romwright draft, unreviewed */

int * FUN_00412e90(long param_1,long param_2,int *param_3,long param_4)

{
  unsigned char bVar1;
  long lVar2;
  int iVar3;
  int *piVar4;
  int iVar5;
  
  bVar1 = param_1 != 0 && param_2 != 0;
  iVar5 = (int)param_1;
  if (param_1 != 0 && param_2 != 0) {
    bVar1 = *(int *)(iVar5 + 4) != 0;
  }
  if (bVar1) {
    iVar3 = *(int *)(iVar5 + 4);
    piVar4 = *(int **)(iVar5 + 8);
    if (param_3 != (int *)0x0) {
      for (; (piVar4 != param_3 && (0 < iVar3)); iVar3 = iVar3 - 1) {
        piVar4 = piVar4 + 10;
      }
      piVar4 = piVar4 + 10;
      iVar3 = iVar3 - 1;
    }
    for (; 0 < iVar3; iVar3 = iVar3 - 1) {
      if (((unsigned int *)*piVar4 != (unsigned int *)0x0) &&
         (lVar2 = (*DAT_008873c8)(*(unsigned int *)*piVar4,param_2), lVar2 == 0)) {
        if (param_4 == 0) {
          return piVar4;
        }
        *(int *)param_4 = *(int *)(iVar5 + 4) - iVar3;
        return piVar4;
      }
      piVar4 = piVar4 + 10;
    }
  }
  if (param_4 != 0) {
    *(int *)param_4 = -1;
  }
  return (int *)0x0;
}
