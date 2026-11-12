/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004c0718 ACSSVR_Init src/cri/acssvr.c - romwright draft, unreviewed */

unsigned int FUN_004c0718(unsigned long long param_1)

{
  long lVar1;
  unsigned long long uVar2;
  int iVar3;
  int iVar4;
  
  lVar1 = FUN_004c0e40();
  iVar4 = (int)param_1;
  if (lVar1 != 0) {
    iVar3 = *(int *)(iVar4 + 0xa4);
  }
  else if (*(int *)(iVar4 + 0xa8) != 0) {
    iVar3 = *(int *)(iVar4 + 0xa4);
  }
  else {
    FUN_004c0e08(param_1);
    *(unsigned int *)(iVar4 + 0xa8) = 1;
    FUN_004ca720();
    FUN_004de340();
    FUN_004f0c78(2,4,0x4c0e50,param_1,0x757368);
    uVar2 = FUN_004c0e78(iVar4 + 0x44,0x60);
    *(int *)(iVar4 + 0x40) = (int)uVar2;
    lVar1 = FUN_004c0f08(uVar2);
    if (lVar1 == 0) {
      FUN_004bd6a0(0x757378);
      FUN_004c07f0(param_1);
      return 0;
    }
    FUN_004c1030(*(unsigned int *)(iVar4 + 0x40),param_1);
    *(unsigned int *)(iVar4 + 0xa8) = 0;
    iVar3 = *(int *)(iVar4 + 0xa4);
  }
  *(int *)(iVar4 + 0xa4) = iVar3 + 1;
  return 1;
}
