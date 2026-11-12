/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e5bd8 func_004e5bd8 src/cri/dvci.c - romwright draft, unreviewed */

void FUN_004e5bd8(void)

{
  long lVar1;
  char *pcVar2;
  int iVar3;
  
  lVar1 = FUN_004f1278(0x7330d0);
  if (lVar1 != 0) {
    pcVar2 = (char *)0x733140;
    iVar3 = 0x27;
    do {
      if (*pcVar2 == '\x01') {
        FUN_004e5b10(pcVar2);
      }
      iVar3 = iVar3 - 1;
      pcVar2 = pcVar2 + 0x48;
    } while (-1 < iVar3);
    DAT_007330d0 = 0;
  }
  return;
}
