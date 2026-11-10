/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00398410 func_00398410 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_00398410(unsigned long long param_1)

{
  int iVar1;
  int *piVar2;
  unsigned int uVar3;
  
  uVar3 = 0;
  do {
    piVar2 = (int *)((int)param_1 + uVar3 * 0x40);
    iVar1 = piVar2[8];
    if (iVar1 != 5) {
      if (iVar1 != 4) {
        if (iVar1 != 2) {
          if (iVar1 == 1) {
            if (piVar2[1] != 0) {
              FUN_003ef3a0();
            }
            if (piVar2[2] != 0) {
              FUN_003ef3a0();
            }
          }
        }
        else if (piVar2[1] != 0) {
          FUN_003ef3a0();
        }
      }
      else if (*piVar2 != 0) {
        FUN_003ef3a0();
      }
    }
    else {
      *piVar2 = 0;
      piVar2[1] = 0;
    }
    uVar3 = (uVar3 + 1) & 0xff;
  } while (uVar3 < 2);
  FUN_0043f9c8(param_1,0,0x90);
  return param_1;
}
