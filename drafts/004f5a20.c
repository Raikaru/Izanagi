/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f5a20 func_004f5a20 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned long long FUN_004f5a20(long param_1,long param_2,long param_3)

{
  unsigned long long uVar1;
  int *piVar2;
  
  if (param_1 == 0) {
    uVar1 = FUN_004f53c0(0x75d6c8,0x9a4,0x75d9d0,0,0xffffffffffffff9c);
    return uVar1;
  }
  if (param_2 < 0) {
    uVar1 = FUN_004f53c0(0x75d6c8,0x9a7,0x75d9d0,0,0xffffffffffffff9c);
    return uVar1;
  }
  if (param_3 == 0) {
    uVar1 = FUN_004f53c0(0x75d6c8,0x9aa,0x75d9d0,0,0xffffffffffffff9c);
    return uVar1;
  }
  piVar2 = (int *)param_3;
  if (*piVar2 == 0) {
    uVar1 = FUN_004f53c0(0x75d6c8,0x9ad,0x75d9d0,0,0xffffffffffffff9c);
    return uVar1;
  }
  FUN_004f5890(*piVar2,param_1,param_2,piVar2[1]);
  piVar2[1] = piVar2[1] + (int)param_2;
  return 0;
}
