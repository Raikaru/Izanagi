/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004c0a40 ACSSVR_DestroySnd src/cri/acssvr.c - romwright draft, unreviewed */

void FUN_004c0a40(int *param_1,int param_2)

{
  unsigned char bVar1;
  int iVar2;
  
  if (param_2 != 0) {
    FUN_004be9c0(param_2);
    iVar2 = 0;
    while (bVar1 = iVar2 < 0x10, iVar2 = iVar2 + 1, bVar1) {
      if (*param_1 == param_2) {
        FUN_004be480(param_2);
        *param_1 = 0;
        return;
      }
      param_1 = param_1 + 1;
    }
  }
  return;
}
