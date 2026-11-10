/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004eb318 SVM_TestAndSet src/cri/ps2_rna.c - romwright draft, unreviewed */

void FUN_004eb318(long param_1,long param_2)

{
  int iVar1;
  
  if (param_1 < 0) {
    FUN_004cd3f0(0x75c6b8,param_1);
    return;
  }
  if (param_2 < 0) {
    FUN_004cd3f0(0x75c6e8);
    return;
  }
  iVar1 = (int)param_1 * 2 + (int)param_2;
  if (0x20 < iVar1) {
    FUN_004cd3f0(0x75c718,0x20);
    return;
  }
  DAT_0073a768 = (int)param_1;
  DAT_0073a76c = (int)param_2;
  DAT_0073a744 = iVar1 * 0x4100;
  DAT_0073a748 = 1;
  DAT_0073a740 = iVar1;
  return;
}
