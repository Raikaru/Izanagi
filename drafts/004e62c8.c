/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e62c8 func_004e62c8 src/cri/dvci.c - romwright draft, unreviewed */

void FUN_004e62c8(long param_1)

{
  int iVar1;
  int iVar2;
  
  iVar2 = 0;
  if (param_1 == 0) {
    FUN_004e5a38(0,0x75b1c8);
    return;
  }
  iVar1 = (int)param_1;
  if (1 < *(unsigned char *)(iVar1 + 2)) {
    if (*(int *)(iVar1 + 0x1c) != 0) {
      FUN_004e3c10();
    }
    do {
      FUN_004e4190();
      FUN_004e5bd8();
      FUN_004e63b0(param_1);
      if (*(char *)(iVar1 + 2) != '\x02') {
        *(unsigned int *)(iVar1 + 0x14) = 0;
        *(unsigned char *)(iVar1 + 2) = 0;
        return;
      }
      iVar2 = iVar2 + 1;
    } while (iVar2 < 0xa0000);
    FUN_004cd3f0(0x75b228);
    *(unsigned int *)(iVar1 + 0x14) = 0;
    *(unsigned char *)(iVar1 + 2) = 0;
  }
  return;
}
