/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e7668 func_004e7668 src/cri/htci.c - romwright draft, unreviewed */

void FUN_004e7668(void)

{
  long lVar1;
  char *pcVar2;
  int iVar3;
  
  lVar1 = FUN_004f1278(0x733f64);
  if (lVar1 != 0) {
    pcVar2 = (char *)0x733fd0;
    iVar3 = 0xc;
    do {
      if (*pcVar2 == '\x01') {
        FUN_004e75a0(pcVar2);
      }
      iVar3 = iVar3 - 1;
      pcVar2 = pcVar2 + 0x30;
    } while (-1 < iVar3);
    DAT_00733f64 = 0;
  }
  return;
}
