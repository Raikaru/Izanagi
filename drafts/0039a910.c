/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039a910 func_0039a910 src/promoted/code1_0039.c - romwright draft, unreviewed */

void FUN_0039a910(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  
  iVar1 = *(int *)(param_1 + DAT_007646d0);
  iVar3 = (unsigned int)(*(int *)(iVar1 + 0x80) != 4) * 0x40;
  iVar2 = iVar1 + iVar3;
  switch(*(unsigned int *)(iVar2 + 4)) {
  default:
    iVar4 = 5;
    break;
  case 1:
    iVar4 = 0;
    break;
  case 2:
    iVar4 = 1;
    break;
  case 5:
    iVar4 = 2;
    break;
  case 6:
    iVar4 = 3;
    break;
  case 7:
    iVar4 = 4;
  }
  switch(*(unsigned int *)(iVar2 + 8)) {
  default:
    iVar2 = 5;
    break;
  case 1:
    iVar2 = 0;
    break;
  case 2:
    iVar2 = 1;
    break;
  case 5:
    iVar2 = 2;
    break;
  case 6:
    iVar2 = 3;
    break;
  case 7:
    iVar2 = 4;
  }
  *(unsigned long long *)(iVar3 + iVar1 + 0x28) = (unsigned long long)*(unsigned char *)(iVar4 + iVar2 * 6 + 0x64f280) | 0x8000000000;
  return;
}
