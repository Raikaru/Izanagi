/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b6f00 RpRandomSeed src/promoted/code1_003b.c - romwright draft, unreviewed */

void FUN_003b6f00(unsigned int param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  
  iVar2 = 1;
  iVar1 = 4;
  **(unsigned int **)((int)&DAT_008872e0 + DAT_00764708) = param_1;
  do {
    iVar2 = iVar2 + 6;
    piVar3 = (int *)(*(int *)((int)&DAT_008872e0 + DAT_00764708) + iVar1);
    *piVar3 = piVar3[-1] * 0x41c64e6d + 0x3039;
    piVar3 = (int *)(*(int *)((int)&DAT_008872e0 + DAT_00764708) + iVar1);
    piVar3[1] = *piVar3 * 0x41c64e6d + 0x3039;
    iVar4 = *(int *)((int)&DAT_008872e0 + DAT_00764708) + iVar1;
    *(int *)(iVar4 + 8) = *(int *)(iVar4 + 4) * 0x41c64e6d + 0x3039;
    iVar4 = *(int *)((int)&DAT_008872e0 + DAT_00764708) + iVar1;
    *(int *)(iVar4 + 0xc) = *(int *)(iVar4 + 8) * 0x41c64e6d + 0x3039;
    iVar4 = *(int *)((int)&DAT_008872e0 + DAT_00764708) + iVar1;
    *(int *)(iVar4 + 0x10) = *(int *)(iVar4 + 0xc) * 0x41c64e6d + 0x3039;
    iVar4 = *(int *)((int)&DAT_008872e0 + DAT_00764708) + iVar1;
    iVar1 = iVar1 + 0x18;
    *(int *)(iVar4 + 0x14) = *(int *)(iVar4 + 0x10) * 0x41c64e6d + 0x3039;
    iVar4 = DAT_00764708;
  } while (iVar2 < 0x1f);
  iVar1 = 0;
  piVar3 = (int *)((int)&DAT_008872e0 + DAT_00764708);
  *(int *)((int)&DAT_008872e4 + DAT_00764708) = *piVar3 + 0xc;
  *(int *)((int)&DAT_008872e8 + iVar4) = *piVar3;
  do {
    FUN_003b7060();
    iVar1 = iVar1 + 1;
  } while (iVar1 < 0x136);
  return;
}
