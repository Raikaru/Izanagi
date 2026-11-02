/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e9fb0 func_004e9fb0 src/cri/mfci.c - romwright draft, unreviewed */

void FUN_004e9fb0(void)

{
  char *pcVar1;
  int iVar2;
  
  pcVar1 = &DAT_00738d68;
  iVar2 = 0x27;
  do {
    if (*pcVar1 != '\0') {
      FUN_004e9fa8(pcVar1);
    }
    iVar2 = iVar2 - 1;
    pcVar1 = pcVar1 + 0x38;
  } while (-1 < iVar2);
  return;
}
