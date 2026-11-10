/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003cac30 func_003cac30 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003cac30(unsigned long long param_1)

{
  int iVar1;
  int *piVar2;
  long lVar3;
  int *piVar4;
  int iVar5;
  
  iVar5 = (int)param_1;
  piVar4 = (int *)(iVar5 + DAT_007647fc);
  lVar3 = (*(code *)piVar4[1])();
  if (lVar3 != 0) {
    iVar1 = *piVar4;
    if (iVar1 != 0) {
      piVar4 = *(int **)(iVar5 + 100);
      while (piVar2 = piVar4, piVar2 != (int *)(iVar5 + 100)) {
        piVar4 = (int *)*piVar2;
        if ((piVar2[-1] != 0) && (piVar2[2] != 0)) {
          *(int *)piVar2[1] = *piVar2;
          *(int *)(*piVar2 + 4) = piVar2[1];
          *(int *)piVar2[-2] = piVar2[-3];
          *(int *)(piVar2[-3] + 4) = piVar2[-2];
          (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e0 + DAT_00764808));
        }
      }
      FUN_003caa50(iVar1,param_1);
    }
  }
  else {
    param_1 = 0;
  }
  return param_1;
}
