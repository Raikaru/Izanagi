/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e58a0 func_004e58a0 src/cri/dtx.c - romwright draft, unreviewed */

unsigned int FUN_004e58a0(int param_1,int param_2,int param_3,int param_4,int param_5)

{
  int iVar1;
  int iVar2;
  
  FUN_004f0608();
  iVar1 = 0;
  if (0 < param_3) {
    do {
      iVar2 = iVar1 + 1;
      (&DAT_00923780)[iVar1] = *(unsigned int *)(iVar1 * 4 + param_2);
      iVar1 = iVar2;
    } while (iVar2 < param_3);
  }
  FUN_004258b0(0x732d30,param_1 + 0x400,0,0x923780,param_3 << 2,0x923880,param_5 << 2,0);
  iVar1 = 0;
  if (0 < param_5) {
    do {
      iVar2 = iVar1 + 1;
      *(unsigned int *)(iVar1 * 4 + param_4) = (&DAT_00923880)[iVar1];
      iVar1 = iVar2;
    } while (iVar2 < param_5);
  }
  FUN_004f0620();
  return DAT_00923880;
}
