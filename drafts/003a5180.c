/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a5180 func_003a5180 src/promoted/code1_003a.c - romwright draft, unreviewed */

unsigned char FUN_003a5180(int param_1)

{
  int iVar1;
  unsigned char bVar2;
  int iVar3;
  unsigned int uVar4;
  int iVar5;
  int iVar6;
  
  iVar1 = *(int *)(*(int *)(param_1 + DAT_00764700) + 0x9c);
  iVar5 = *(int *)(param_1 + DAT_00764700) + 0x50;
  if (*(int *)(iVar1 + 0xd8) != 0) {
    if (*(int *)(iVar1 + 0xe4) < *(int *)(iVar1 + 0xe8)) {
      *(int *)(iVar1 + 0xe4) = *(int *)(iVar1 + 0xe4) + 1;
    }
    else {
      *(unsigned int *)(iVar1 + 0xe4) = 0;
    }
    if (*(int *)(iVar1 + 0xe8) != 0) {
      uVar4 = 0;
      iVar3 = 0;
      do {
        uVar4 = uVar4 + 3;
        *(unsigned int *)(iVar5 + 4) =
             *(unsigned int *)(iVar1 + *(int *)(iVar1 + 0xe4) * 0x24 + iVar3 + 0x140);
        *(unsigned int *)(iVar5 + 0xc) =
             *(unsigned int *)(iVar1 + *(int *)(iVar1 + 0xe4) * 0x24 + iVar3 + 0x144);
        iVar6 = iVar1 + *(int *)(iVar1 + 0xe4) * 0x24 + iVar3;
        iVar3 = iVar3 + 0xc;
        *(unsigned int *)(iVar5 + 0x14) = *(unsigned int *)(iVar6 + 0x148);
        iVar5 = iVar5 + 0x18;
      } while (uVar4 < 9);
    }
    bVar2 = *(int *)(*(int *)(iVar1 + 0xe4) * 0x20 + iVar1 + 0x10) == 0;
  }
  else {
    bVar2 = 1;
  }
  return bVar2;
}
