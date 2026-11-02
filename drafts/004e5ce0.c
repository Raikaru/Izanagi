/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e5ce0 func_004e5ce0 src/cri/dvci.c - romwright draft, unreviewed */

unsigned long long FUN_004e5ce0(long param_1)

{
  unsigned long long uVar1;
  long lVar2;
  unsigned char auStack_50 [4];
  int iStack_4c;
  
  uVar1 = 0;
  if (param_1 != 0) {
    FUN_004e6eb0(param_1,auStack_50);
    if (iStack_4c == 0) {
      FUN_004e6640(param_1,0x733c80);
      FUN_004e4618(0x8100);
      FUN_0042e7c8(0);
      FUN_004e4618(0x8101);
      lVar2 = FUN_004e5a68(auStack_50,0x733c80);
      if (lVar2 == 0) {
        return 0;
      }
    }
    uVar1 = 1;
  }
  return uVar1;
}
