/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00396890 HAnimDestructor src/promoted/code1_0039.c - romwright draft, unreviewed */

int FUN_00396890(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  unsigned int *puVar4;
  
  puVar4 = (unsigned int *)(param_1 + DAT_007646c8);
  iVar1 = puVar4[1];
  if (iVar1 != 0) {
    iVar3 = 0;
    if (0 < *(int *)(iVar1 + 4)) {
      iVar2 = 0;
      do {
        iVar3 = iVar3 + 1;
        *(unsigned int *)(*(int *)(iVar1 + 0x10) + iVar2 + 0xc) = 0;
        iVar2 = iVar2 + 0x10;
      } while (iVar3 < *(int *)(iVar1 + 4));
    }
    if (*(int *)(iVar1 + 0x14) != param_1) {
      puVar4[1] = 0;
    }
    else {
      FUN_00397120();
      puVar4[1] = 0;
    }
  }
  *puVar4 = 0xffffffff;
  return param_1;
}
