/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x001f5680 func_001f5680 src/Battle/btlTarget_inferred_001f42a0.c - romwright draft, unreviewed */

int FUN_001f5680(void)

{
  unsigned short uVar1;
  int iVar2;
  
  if (*(int *)(DAT_0076449c + 0xc68) == 0) {
    iVar2 = -1;
  }
  else {
    uVar1 = *(unsigned short *)(*(int *)(DAT_0076449c + 0xc68) + 2);
    if ((uVar1 >= 6) || (uVar1 == 0)) {
      iVar2 = -1;
    }
    else {
      iVar2 = uVar1 + 0x11;
    }
  }
  return iVar2;
}
