/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c0520 func_003c0520 src/promoted/code1_003c.c - romwright draft, unreviewed */

int FUN_003c0520(int param_1)

{
  unsigned int *puVar1;
  unsigned int *puVar2;
  unsigned int uVar3;
  long lVar4;
  unsigned int *puVar5;
  int iStack_10;
  int iStack_c;
  int iStack_8;
  
  iStack_10 = param_1;
  lVar4 = FUN_003c0640();
  iStack_c = (int)lVar4;
  if (lVar4 == 0) {
    iStack_c = 0;
  }
  else {
    uVar3 = FUN_003e90f0(*(unsigned int *)(iStack_10 + 4));
    *(unsigned int *)(iStack_c + 4) = uVar3;
    if (*(int *)(iStack_c + 4) != 0) {
      iStack_8 = 1;
      puVar5 = (unsigned int *)(iStack_10 + 8);
      puVar2 = *(unsigned int **)(iStack_10 + 8);
      do {
        if (puVar2 == puVar5) break;
        puVar1 = (unsigned int *)*puVar2;
        lVar4 = FUN_003bea60(puVar2 + -0x10,&iStack_10);
        puVar2 = puVar1;
      } while (lVar4 != 0);
      FUN_003e9140(*(unsigned int *)(iStack_10 + 4));
      if (iStack_8 == 0) {
        FUN_003c0700(iStack_c);
        iStack_c = 0;
      }
      else {
        *(unsigned int *)(iStack_c + 0x28) = *(unsigned int *)(iStack_10 + 0x28);
        FUN_003e3c90(0x70af90,iStack_c,iStack_10);
      }
    }
    else {
      FUN_003c0700();
      iStack_c = 0;
    }
  }
  return iStack_c;
}
