/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x005091a8 MWSFCRE_ResetSfdHn src/cri/mwsfdcre.c - romwright draft, unreviewed */

unsigned int FUN_005091a8(unsigned long long param_1)

{
  long lVar1;
  unsigned int uVar2;
  int iVar3;
  
  iVar3 = (int)param_1;
  uVar2 = *(unsigned int *)(iVar3 + 0x3c);
  *(unsigned int *)(iVar3 + 0x3c) = 0;
  lVar1 = FUN_00522ff8(uVar2);
  if (lVar1 != 0) {
    uVar2 = 0x75e208;
  }
  else {
    *(unsigned int *)(iVar3 + 0x3c) = uVar2;
    lVar1 = FUN_00517b80(uVar2,0x50ba08,param_1);
    if (lVar1 == 0) {
      FUN_00508eb0(param_1);
      return 0;
    }
    FUN_0050b8f0(0xfffffffffffffed1);
    uVar2 = 0x75e240;
  }
  FUN_00511220(uVar2);
  return 0xffffffff;
}
