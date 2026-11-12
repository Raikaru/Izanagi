/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0050d4d0 func_0050d4d0 src/cri/mwsfdset.c - romwright draft, unreviewed */

int FUN_0050d4d0(int param_1)

{
  int iVar1;
  long lVar2;
  
  lVar2 = FUN_0050d428();
  if (lVar2 != 1) {
    FUN_0050b8f0(0xfffffffffffffff4);
    FUN_00511220(0x75f2b8);
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)(param_1 + 4);
    if (iVar1 == 2) {
      lVar2 = FUN_00524d50(*(unsigned int *)(param_1 + 0x3c));
      iVar1 = 4;
      if (-1 < lVar2) {
        iVar1 = 1;
      }
      if ((lVar2 == 4) || (lVar2 == 6)) {
        iVar1 = 2;
      }
    }
  }
  return iVar1;
}
