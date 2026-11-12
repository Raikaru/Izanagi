/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f1a08 func_004f1a08 src/sce/rofs_hn.c - romwright draft, unreviewed */

unsigned int FUN_004f1a08(long param_1)

{
  unsigned int uVar1;
  
  FUN_004f54a0();
  if (param_1 == 0) {
    uVar1 = FUN_004f53c0(0x75d408,0x15c,0x75d428,0,0xffffffffffffff9b);
    return uVar1;
  }
  if (*(short *)((int)param_1 + 0x34) != 1) {
    uVar1 = FUN_004f53c0(0x75d408,0x15f,0x75d428,0,0xffffffffffffff9b);
    return uVar1;
  }
  return *(unsigned int *)((int)param_1 + 0x14);
}
