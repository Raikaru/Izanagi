/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003ca5a0 func_003ca5a0 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003ca5a0(unsigned long long param_1,int param_2)

{
  int iVar1;
  unsigned int uVar2;
  int *piVar3;
  unsigned int *puVar4;
  
  DAT_00764808 = param_2;
  iVar1 = FUN_003e1220(0x18,DAT_00763b98,4,DAT_00763b9c,0x886610,0x40507);
  piVar3 = (int *)((int)&DAT_008872e0 + DAT_00764808);
  *piVar3 = iVar1;
  if (*piVar3 == 0) {
    param_1 = 0;
  }
  else {
    uVar2 = FUN_003e1220(0x18,DAT_00763ba0,4,DAT_00763ba4,0x8865e0,0x40507);
    iVar1 = DAT_00764808;
    puVar4 = (unsigned int *)((int)&DAT_008872e0 + DAT_00764808);
    *(unsigned int *)((int)&DAT_008872e4 + DAT_00764808) = uVar2;
    if (*(int *)((int)&DAT_008872e4 + iVar1) != 0) {
      DAT_008872e8 = 1;
      DAT_0076480c = DAT_0076480c + 1;
      *(unsigned int *)((int)&DAT_008872e8 + DAT_00764808) = 0;
    }
    else {
      FUN_003e12f0(*puVar4);
      param_1 = 0;
      *(unsigned int *)((int)&DAT_008872e0 + DAT_00764808) = 0;
    }
  }
  return param_1;
}
