/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x005152e0 func_005152e0 src/cri/sfd_buf.c - romwright draft, unreviewed */

void FUN_005152e0(int param_1,int param_2,int param_3)

{
  int iVar1;
  
  param_1 = param_1 + param_2 * 0x74;
  iVar1 = *(int *)(param_1 + 0x1344);
  if (-1 < iVar1) {
    *(int *)(param_1 + 0x1344) = iVar1 + param_3;
  }
  return;
}
