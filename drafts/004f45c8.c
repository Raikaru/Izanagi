/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f45c8 func_004f45c8 src/sce/rofs_mai.c - romwright draft, unreviewed */

void FUN_004f45c8(void)

{
  int *piVar1;
  int iVar2;
  
  FUN_004f6888();
  if (DAT_00743a50 == (int *)0x0) {
    FUN_004f68a0();
    return;
  }
  if (*DAT_00743a50 != 0) {
    if (DAT_00743a58 == 0) {
      iVar2 = 0;
      DAT_00743a58 = 1;
      FUN_004f68a0();
      piVar1 = DAT_00743a50 + 0x423;
      if (0 < DAT_00743a50[1]) {
        do {
          if (*(short *)((int)piVar1 + 0x36) == 1) {
            FUN_004f43a0(piVar1);
          }
          iVar2 = iVar2 + 1;
          piVar1 = piVar1 + 0xf;
        } while (iVar2 < DAT_00743a50[1]);
      }
      DAT_00743a50[5] = DAT_00743a50[5] + 1;
      DAT_00743a58 = 0;
      return;
    }
    FUN_004f68a0();
    return;
  }
  FUN_004f68a0();
  return;
}
