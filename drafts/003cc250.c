/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003cc250 func_003cc250 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned int FUN_003cc250(unsigned long long param_1,unsigned int *param_2)

{
  short *psVar1;
  unsigned int uVar2;
  
  psVar1 = (short *)*param_2;
  if (*psVar1 != 0) {
    *(unsigned int *)(psVar1 + 0xc) = *(unsigned int *)(psVar1 + 2);
    if ((*(unsigned int *)(psVar1 + 6) & 1) == 0) {
      (*DAT_00887300)(1,0);
    }
    uVar2 = 1;
  }
  else {
    uVar2 = 0;
  }
  return uVar2;
}
