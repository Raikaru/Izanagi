/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d5330 func_003d5330 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned long long FUN_003d5330(unsigned long long param_1)

{
  long lVar1;
  long lVar2;
  unsigned long long uVar3;
  
  lVar1 = FUN_003e2f60(2,1,param_1);
  if (lVar1 != 0) {
    lVar2 = FUN_003df050(lVar1,0x1b,0,0);
    if (lVar2 == 0) {
      uVar3 = 0;
    }
    else {
      uVar3 = FUN_003d53c0(lVar1);
      FUN_003e2e40(lVar1,0);
    }
  }
  else {
    uVar3 = 0;
  }
  return uVar3;
}
