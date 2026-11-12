/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b6e70 func_003b6e70 src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned long long FUN_003b6e70(unsigned long long param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  
  iVar2 = (*DAT_008873e8)(0x7c,0x4010c);
  iVar1 = DAT_00764708;
  piVar3 = (int *)((int)&DAT_008872e0 + DAT_00764708);
  *piVar3 = iVar2;
  if (*piVar3 != 0) {
    *(int *)((int)&DAT_008872e4 + iVar1) = *piVar3 + 0xc;
    *(int *)((int)&DAT_008872e8 + iVar1) = *piVar3;
    *(int *)(iVar1 + 0x8872ec) = *piVar3 + 0x7c;
    FUN_003b6f00(0xffffffff9a319039);
  }
  else {
    param_1 = 0;
  }
  return param_1;
}
