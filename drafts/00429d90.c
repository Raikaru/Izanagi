/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00429d90 func_00429d90 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_00429d90(unsigned int param_1)

{
  unsigned int uVar1;
  long lVar2;
  
  if (DAT_0070c698 < 0) {
    uVar1 = 0;
  }
  else {
    DAT_00891000 = param_1;
    lVar2 = FUN_004258b0(0x890f80,2,0,0x891000,4,0x890fc0,4,0);
    uVar1 = DAT_00890fc0;
    if (lVar2 < 0) {
      uVar1 = 0xffffffff;
    }
  }
  return uVar1;
}
