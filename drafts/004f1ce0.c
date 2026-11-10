/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f1ce0 func_004f1ce0 src/sce/rofs_hn.c - romwright draft, unreviewed */

int FUN_004f1ce0(long param_1)

{
  int iVar1;
  
  FUN_004f54a0();
  if (param_1 == 0) {
    iVar1 = FUN_004f53c0(0x75d408,0x1e5,0x75d458,0,0xffffffffffffff9b);
    return iVar1;
  }
  iVar1 = (int)param_1;
  if (*(short *)(iVar1 + 0x34) != 0) {
    return *(int *)(iVar1 + 0x24) * 0x800 + *(int *)(iVar1 + 0x28);
  }
  iVar1 = FUN_004f53c0(0x75d408,0x1e8,0x75d458,0,0xffffffffffffff9b);
  return iVar1;
}
