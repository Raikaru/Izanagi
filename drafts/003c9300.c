/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c9300 func_003c9300 src/promoted/code1_003c.c - romwright draft, unreviewed */

void FUN_003c9300(int *param_1)

{
  int *piVar1;
  int *piVar2;
  int iVar3;
  
  if (*param_1 != -2) {
    if (*param_1 == -1) {
      if (param_1[0xd] != 0) {
        FUN_003e1ea0();
      }
      piVar2 = (int *)param_1[0xe];
      if ((int *)param_1[0xe] != param_1 + 0xe) {
        do {
          piVar1 = (int *)*piVar2;
          FUN_003ca9d0(piVar2);
          piVar2 = piVar1;
        } while (piVar1 != param_1 + 0xe);
        piVar2 = (int *)param_1[0x10];
      }
      else {
        piVar2 = (int *)param_1[0x10];
      }
      while (piVar2 != param_1 + 0x10) {
        piVar1 = (int *)*piVar2;
        FUN_003ca960(piVar2);
        piVar2 = piVar1;
      }
      FUN_003e3c20(0x70b040,param_1);
      if (param_1[2] != 0) {
        (*DAT_008873ec)();
        param_1[2] = 0;
      }
      if (param_1[3] != 0) {
        (*DAT_008873ec)();
        param_1[3] = 0;
      }
      if (param_1[0xc] != 0) {
        (*DAT_008873ec)();
        param_1[0xc] = 0;
      }
      if (param_1[1] != 0) {
        (*DAT_008873ec)();
        param_1[1] = 0;
      }
      iVar3 = 0;
      piVar2 = param_1;
      do {
        if (piVar2[4] != 0) {
          (*DAT_008873ec)();
          piVar2[4] = 0;
        }
        iVar3 = iVar3 + 1;
        piVar2 = piVar2 + 1;
      } while (iVar3 < 8);
      if (param_1[0x1e] != 0) {
        FUN_003c5760();
        param_1[0x1e] = 0;
      }
      (*DAT_008873ec)(param_1);
    }
    else {
      if (param_1[2] != 0) {
        FUN_003c9300();
      }
      param_1[2] = 0;
      if (param_1[3] != 0) {
        FUN_003c9300();
      }
      param_1[3] = 0;
      (*DAT_008873ec)(param_1);
    }
  }
  else {
    (*DAT_008873ec)();
  }
  return;
}
