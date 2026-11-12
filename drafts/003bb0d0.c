/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bb0d0 func_003bb0d0 src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned long long FUN_003bb0d0(unsigned long long param_1)

{
  unsigned char bVar1;
  long lVar2;
  unsigned int *puVar3;
  int iVar4;
  int uStack_10;
  unsigned int uStack_c;
  int uStack_8;
  unsigned int uStack_4;
  
  if (DAT_00764744 == 0) {
    lVar2 = FUN_003b8e50();
    if (lVar2 == 0) {
      uStack_8 = 0x102;
      uStack_4 = FUN_003df590(3);
      FUN_003df4d0(&uStack_8);
      return 0;
    }
    iVar4 = 4;
    puVar3 = (unsigned int *)0x8864d0;
LAB_003bb124:
    lVar2 = FUN_003b90b0(iVar4);
    *puVar3 = (int)lVar2;
    if (lVar2 == 0) {
      iVar4 = iVar4 - 1;
      if (3 < iVar4) {
        puVar3 = (unsigned int *)(iVar4 * 4 + 0x8864c0);
        do {
          (*DAT_008873ec)(*puVar3);
          iVar4 = iVar4 - 1;
          *puVar3 = 0;
          puVar3 = puVar3 + -1;
        } while (3 < iVar4);
      }
      bVar1 = 0;
    }
    else {
      iVar4 = iVar4 + 1;
      puVar3 = puVar3 + 1;
      if (10 >= iVar4) goto LAB_003bb124;
      bVar1 = 1;
    }
    if (!bVar1) {
      uStack_10 = 0x102;
      uStack_c = FUN_003df590(3);
      FUN_003df4d0(&uStack_10);
      return 0;
    }
  }
  DAT_00764744 = DAT_00764744 + 1;
  return param_1;
}
