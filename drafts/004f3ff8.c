/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f3ff8 RFMAI_GetDefVolumeG src/sce/rofs_mai.c - romwright draft, unreviewed */

int FUN_004f3ff8(void)

{
  int iVar1;
  
  iVar1 = *(int *)(DAT_00743a50 + 0x2c) + 0x16;
  if (*(int *)(DAT_00743a50 + 0x2c) == 0) {
    iVar1 = 0;
  }
  return iVar1;
}
