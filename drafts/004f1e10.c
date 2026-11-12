/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f1e10 func_004f1e10 src/sce/rofs_hn.c - romwright draft, unreviewed */

void FUN_004f1e10(long param_1)

{
  int iVar1;
  
  if (param_1 == 0) {
    FUN_004f53c0(0x75d408,0x208,0x75d478,0,0xffffffffffffff9b);
    return;
  }
  iVar1 = (int)param_1;
  if (*(short *)(iVar1 + 0x34) != 0) {
    if (*(int *)(iVar1 + 0x1c) == 0) {
      FUN_004f53c0(0x75d408,0x210,0x75d478,0,0xffffffffffffff9b);
      return;
    }
    if (*(short *)(iVar1 + 0x38) == 2) {
      if (*(short *)(iVar1 + 0x36) == 1) {
        FUN_004f5390();
        (**(code **)(*(int *)(*(int *)(iVar1 + 8) + 0x38) + 0x14))(**(unsigned int **)(iVar1 + 0x1c));
        *(unsigned short *)(iVar1 + 0x36) = 2;
        FUN_004f53a8(param_1);
        return;
      }
      *(unsigned short *)(iVar1 + 0x36) = 2;
    }
    else {
      *(unsigned short *)(iVar1 + 0x38) = 0;
    }
    return;
  }
  FUN_004f53c0(0x75d408,0x20c,0x75d478,0,0xffffffffffffff9b);
  return;
}
