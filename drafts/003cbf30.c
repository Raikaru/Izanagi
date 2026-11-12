/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003cbf30 func_003cbf30 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003cbf30(unsigned long long param_1,int param_2)

{
  int *piVar1;
  int *piVar2;
  
  *(unsigned int *)(param_2 + DAT_00764804) = 0;
  piVar2 = *(int **)(param_2 + 0x2c);
  while (piVar2 != (int *)(param_2 + 0x2c)) {
    piVar1 = (int *)*piVar2;
    *(int *)piVar2[1] = *piVar2;
    *(int *)(*piVar2 + 4) = piVar2[1];
    *(int *)piVar2[-2] = piVar2[-3];
    *(int *)(piVar2[-3] + 4) = piVar2[-2];
    (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_00764808));
    piVar2 = piVar1;
  }
  **(unsigned int **)(param_2 + 0x38) = *(unsigned int *)(param_2 + 0x34);
  *(unsigned int *)(*(int *)(param_2 + 0x34) + 4) = *(unsigned int *)(param_2 + 0x38);
  return param_1;
}
