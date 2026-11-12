/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003ce2e0 func_003ce2e0 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned int FUN_003ce2e0(int param_1)

{
  unsigned int uVar1;
  int *piVar2;
  unsigned char bVar3;
  unsigned int *puVar4;
  unsigned int uVar5;
  
  piVar2 = (int *)(param_1 + DAT_00764820);
  bVar3 = 0;
  if (((unsigned int *)*piVar2 != (unsigned int *)0x0) &&
     ((unsigned int)*(unsigned short *)(*(int *)(param_1 + 0x54) + 4) == *(unsigned int *)*piVar2)) {
    bVar3 = 1;
  }
  if (bVar3) {
    puVar4 = (unsigned int *)*piVar2;
  }
  else {
    piVar2 = (int *)FUN_003ce050(piVar2,*(unsigned short *)(*(int *)(param_1 + 0x54) + 4));
    puVar4 = (unsigned int *)*piVar2;
  }
  uVar1 = *puVar4;
  uVar5 = 0;
  if (uVar1 != 0) {
    do {
      if (puVar4[1] != 0) {
        FUN_003e1ea0();
      }
      uVar5 = uVar5 + 1;
      puVar4 = puVar4 + 1;
    } while (uVar5 < uVar1);
  }
  return 1;
}
