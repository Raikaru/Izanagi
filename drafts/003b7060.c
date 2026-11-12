/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b7060 RpRandom src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned int FUN_003b7060(void)

{
  unsigned int uVar1;
  int iVar2;
  unsigned int *puVar3;
  unsigned int uVar4;
  unsigned int *puVar5;
  
  **(int **)((int)&DAT_008872e4 + DAT_00764708) =
       **(int **)((int)&DAT_008872e4 + DAT_00764708) + **(int **)((int)&DAT_008872e8 + DAT_00764708)
  ;
  iVar2 = DAT_00764708;
  puVar5 = (unsigned int *)((int)&DAT_008872e0 + DAT_00764708);
  uVar1 = **(unsigned int **)((int)&DAT_008872e4 + DAT_00764708);
  puVar3 = *(unsigned int **)((int)&DAT_008872e4 + DAT_00764708) + 1;
  *(unsigned int **)((int)&DAT_008872e4 + DAT_00764708) = puVar3;
  if (puVar3 < *(unsigned int **)(iVar2 + 0x8872ec)) {
    uVar4 = *(int *)((int)&DAT_008872e8 + iVar2) + 4;
    *(unsigned int *)((int)&DAT_008872e8 + iVar2) = uVar4;
    if (*(unsigned int *)(iVar2 + 0x8872ec) <= uVar4) {
      *(unsigned int *)((int)&DAT_008872e8 + iVar2) = *puVar5;
    }
  }
  else {
    *(unsigned int *)((int)&DAT_008872e4 + iVar2) = *puVar5;
    *(int *)((int)&DAT_008872e8 + iVar2) = *(int *)((int)&DAT_008872e8 + iVar2) + 4;
  }
  return uVar1 >> 1;
}
