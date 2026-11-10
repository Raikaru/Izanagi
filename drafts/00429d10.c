/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00429d10 func_00429d10 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_00429d10(unsigned int param_1,unsigned int param_2,unsigned int param_3)

{
  unsigned int uVar1;
  long lVar2;
  
  uVar1 = 0;
  if (-1 < DAT_0070c698) {
    DAT_00891000 = param_2;
    DAT_00891004 = param_1;
    DAT_00891008 = param_3;
    lVar2 = FUN_004258b0(0x890f80,4,0,0x891000,0xc,0x890fc0,4,0);
    uVar1 = DAT_00890fc0;
    if (lVar2 < 0) {
      uVar1 = 0;
    }
  }
  return uVar1;
}
