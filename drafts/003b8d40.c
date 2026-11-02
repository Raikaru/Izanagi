/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b8d40 _rpSkinPipelinesAttach src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned long long FUN_003b8d40(unsigned long long param_1,long param_2)

{
  unsigned char bVar1;
  int iVar2;
  unsigned int uVar3;
  int iVar4;
  unsigned long long uVar5;
  
  iVar4 = (int)param_1;
  bVar1 = (*(unsigned int *)(*(int *)(iVar4 + 0x18) + 8) & 0x80) == 0;
  iVar2 = FUN_003c1b80(0x120);
  if (param_2 != 0) {
    if (param_2 != 3) {
      if (param_2 != 2) {
        if (param_2 == 1) {
          uVar5 = 0x11002;
        }
        else {
          uVar5 = 0x11002;
        }
      }
      else if (bVar1) {
        uVar5 = 0x11013;
      }
      else {
        uVar5 = 0x11014;
      }
    }
    else {
      uVar5 = 0x11002;
    }
  }
  else {
    uVar5 = 0x11002;
    if (*(char *)(iVar4 + iVar2) != '\0') {
      if (bVar1) {
        uVar5 = 0x11013;
      }
      else {
        uVar5 = 0x11014;
      }
    }
  }
  uVar3 = FUN_0039b6e0(uVar5);
  *(unsigned int *)(iVar4 + 0x6c) = uVar3;
  return param_1;
}
