/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a8500 func_003a8500 src/promoted/code1_003a.c - romwright draft, unreviewed */

unsigned char FUN_003a8500(int param_1)

{
  int iVar1;
  unsigned int uVar2;
  long lVar3;
  long lVar4;
  unsigned long long uVar5;
  unsigned long long uVar6;
  
  uVar2 = FUN_003a7f80();
  *(unsigned int *)(param_1 + 0xc) = uVar2;
  iVar1 = *(int *)(param_1 + 0xc);
  if (iVar1 == 0) {
    return 0;
  }
  lVar3 = FUN_004115d0();
  if (lVar3 != 0) {
    lVar4 = FUN_00412ca0(lVar3);
    if (lVar4 != 0) {
      uVar5 = FUN_003cfa70();
      uVar6 = FUN_00412fb0(lVar4,0,uVar5,0);
      FUN_00412e90(uVar6,*(unsigned int *)uVar5,0,0);
      lVar4 = FUN_004125e0(uVar6);
      if (lVar4 != 0) {
        uVar5 = FUN_00412e90(lVar4,*(unsigned int *)uVar5,0,0);
        FUN_003cf9b0(uVar5,1,0x3a8180);
        FUN_003cfa30(uVar5,iVar1);
        *(int *)(param_1 + 0x10) = (int)lVar3;
        return *(int *)(param_1 + 0x10) != 0;
      }
    }
    FUN_00411670(lVar3);
  }
  lVar3 = 0;
  *(int *)(param_1 + 0x10) = (int)lVar3;
  return *(int *)(param_1 + 0x10) != 0;
}
