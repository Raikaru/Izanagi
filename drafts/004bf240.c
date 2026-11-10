/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bf240 ACSSND_AttachAhx src/cri/acssnd.c - romwright draft, unreviewed */

void FUN_004bf240(unsigned long long param_1)

{
  unsigned long long uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  
  iVar3 = (int)param_1;
  if (*(int *)(iVar3 + 0x50) == 0) {
    FUN_004bd6a0(0x756fe0);
    return;
  }
  if (*(int *)(iVar3 + 0x98) != 0) {
    iVar4 = 0;
    if (0 < *(int *)(iVar3 + 0xa0)) {
      iVar5 = 0;
      iVar2 = *(int *)(iVar3 + 0x94);
      while( 1 ) {
        if (iVar2 == 1) {
          uVar1 = FUN_004bf6a8(param_1);
          uVar1 = FUN_004deec0(uVar1,0);
          iVar2 = *(int *)(iVar3 + 0x6c);
        }
        else {
          uVar1 = FUN_004bf510(param_1,iVar4);
          iVar2 = *(int *)(iVar3 + 0x6c);
        }
        iVar4 = iVar4 + 1;
        iVar2 = iVar2 + iVar5;
        iVar5 = iVar5 + 0x3000;
        FUN_004db250(uVar1,iVar2,0x3000);
        if (*(int *)(iVar3 + 0xa0) <= iVar4) break;
        iVar2 = *(int *)(iVar3 + 0x94);
      }
    }
    return;
  }
  FUN_004bd6a0(0x757010);
  return;
}
