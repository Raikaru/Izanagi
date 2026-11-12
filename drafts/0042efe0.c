/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0042efe0 func_0042efe0 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_0042efe0(long param_1)

{
  unsigned char bVar1;
  int iVar2;
  unsigned int uVar3;
  long lVar4;
  int iVar5;
  
  if (0 < DAT_0070dcd0) {
    FUN_004244c8(0x7546b0);
  }
  FUN_0042dfe0();
  iVar2 = FUN_00421830(DAT_0070dcf0);
  if (DAT_0070dcf0 == iVar2) {
    FUN_00424f28(0);
    iVar2 = 0;
    if (DAT_0070dd10 < 0) {
      while( 1 ) {
        while (lVar4 = FUN_004256d0(0x894ed0,0xffffffff8000059c,0), lVar4 < 0) {
          if (0 < DAT_0070dcd0) {
            FUN_004244c8(0x754678);
          }
          iVar5 = 0x100000;
          do {
            iVar5 = iVar5 - 1;
          } while (iVar5 != -1);
        }
        bVar1 = 0x10 < iVar2;
        if (DAT_00894ef4 != 0) break;
        iVar2 = iVar2 + 1;
        if (bVar1) {
          FUN_00421800(DAT_0070dcf0);
          uVar3 = FUN_0042edc8(param_1);
          return uVar3;
        }
        iVar5 = 0x100000;
        do {
          iVar5 = iVar5 - 1;
        } while (iVar5 != -1);
      }
      DAT_0070dd10 = 0;
    }
    lVar4 = FUN_0042ba20();
    DAT_0070dd00 = 0;
    if (lVar4 != 0) {
      FUN_0042ba70();
    }
    DAT_00894f10 = (unsigned int)param_1;
    FUN_00424e78(0x894f10,4);
    lVar4 = FUN_004258b0(0x894ed0,0,0,0x894f10,4,0x70f740,4,0);
    if (-1 < lVar4) {
      if (0 < DAT_0070dcd0) {
        FUN_004244c8(0x754698);
      }
      uVar3 = uRam2070f740;
      FUN_00421800(DAT_0070dcf0);
      return uVar3;
    }
    FUN_00421800(DAT_0070dcf0);
  }
  uVar3 = 6;
  if (param_1 == 8) {
    uVar3 = 0xffffffff;
  }
  return uVar3;
}
