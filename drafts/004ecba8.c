/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ecba8 func_004ecba8 src/cri/ps2_rna.c - romwright draft, unreviewed */

int FUN_004ecba8(int param_1)

{
  int *piVar1;
  int iVar2;
  
  if ((DAT_0073a748 != 1) || (iVar2 = 0, 0 < DAT_0073a740)) {
    piVar1 = *(int **)(*(int *)(param_1 + 8) + 8);
    iVar2 = (**(code **)(*piVar1 + 0x24))(piVar1,0);
    iVar2 = iVar2 / 2;
  }
  return iVar2;
}
