/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039b830 func_0039b830 src/promoted/code1_0039.c - romwright draft, unreviewed */

void FUN_0039b830(int *param_1)

{
  int iVar1;
  unsigned int uVar2;
  
  uVar2 = 0;
  if (param_1[1] != 0) {
    iVar1 = 0;
    do {
      if (*(int *)(*param_1 + iVar1 + 0x10) != 0) {
        FUN_00411670();
      }
      uVar2 = uVar2 + 1;
      iVar1 = iVar1 + 0x14;
    } while (uVar2 < (unsigned int)param_1[1]);
  }
  (*DAT_008873ec)(*param_1);
  param_1[2] = 0;
  param_1[1] = 0;
  *param_1 = 0;
  param_1[3] = 0;
  return;
}
