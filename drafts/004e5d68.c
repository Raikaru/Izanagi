/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e5d68 dvCiGetFileSize src/cri/dvci.c - romwright draft, unreviewed */

int FUN_004e5d68(long param_1)

{
  long lVar1;
  unsigned int uVar2;
  unsigned char auStack_50 [4];
  int iStack_4c;
  
  if (param_1 == 0) {
    uVar2 = 0x75b068;
  }
  else {
    FUN_004e6eb0(param_1,auStack_50);
    if (iStack_4c != 0) {
      return iStack_4c;
    }
    FUN_004e6640(param_1,0x733c80);
    FUN_004e4618(0x8200);
    FUN_0042e7c8(0);
    FUN_004e4618(0x8201);
    lVar1 = FUN_004e5a68(auStack_50,0x733c80);
    if (lVar1 != 0) {
      return iStack_4c;
    }
    FUN_004cd3f0(0x75b098,0x733c80);
    uVar2 = 0x75b0c0;
  }
  FUN_004e5a38(0,uVar2);
  return 0;
}
