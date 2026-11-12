/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004c07f0 ACSSVR_Finish src/cri/acssvr.c - romwright draft, unreviewed */

void FUN_004c07f0(unsigned int *param_1)

{
  unsigned int uVar1;
  unsigned int *puVar2;
  int iVar3;
  
  iVar3 = param_1[0x29];
  if (param_1[0x2a] == 0) {
    param_1[0x29] = iVar3 - 1;
    iVar3 = param_1[0x29];
  }
  if (-1 < iVar3) {
    if ((iVar3 == 0) || (param_1[0x2a] == 1)) {
      iVar3 = 0xf;
      uVar1 = *param_1;
      puVar2 = param_1;
      while( 1 ) {
        puVar2 = puVar2 + 1;
        iVar3 = iVar3 - 1;
        FUN_004c0a40(param_1,uVar1);
        if (iVar3 < 0) break;
        uVar1 = *puVar2;
      }
      if (param_1[0x10] != 0) {
        FUN_004c0f88();
        FUN_004c0ee0(param_1[0x10]);
        param_1[0x10] = 0;
      }
      FUN_004f0b88(2,4);
      FUN_004f0b88(5,param_1[0x2b]);
      FUN_004de3f0();
      FUN_004ca8b0();
      param_1[0x2a] = 0;
    }
    return;
  }
  FUN_004bd6a0(0x7573b0);
  return;
}
