/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004beec8 ACSSND_SetOutVol src/cri/acssnd.c - romwright draft, unreviewed */

void FUN_004beec8(unsigned long long param_1,unsigned long long param_2)

{
  unsigned int uVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = (int)param_1;
  if (*(int *)(iVar3 + 0x50) == 0) {
    FUN_004bd6a0(0x756f48);
    return;
  }
  FUN_004bd658();
  *(int *)(iVar3 + 0xbc) = (int)param_2;
  iVar2 = iVar3 + *(int *)(iVar3 + 0x10) * 0xc;
  if (*(char *)(iVar2 + 0x1c) != '\x01') {
    FUN_004c01c0(param_1,1);
    uVar1 = *(unsigned int *)(iVar2 + 0x20);
    FUN_004bd790(uVar1,param_2,*(unsigned int *)(iVar3 + 0xb0),0,*(unsigned int *)(iVar3 + 0xb8));
    FUN_004bd9d8(uVar1,0x4c0658,param_1,2);
    FUN_004bd9e8(uVar1,0x4c0658,param_1,5);
    *(unsigned char *)(iVar2 + 0x1c) = 3;
    *(unsigned char *)(iVar2 + 0x1d) = 0;
  }
  FUN_004bd670();
  return;
}
