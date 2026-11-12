/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d7ac0 func_003d7ac0 src/rw/rpuvanim.c - romwright draft, unreviewed */

unsigned long long FUN_003d7ac0(unsigned long long param_1,unsigned long long param_2)

{
  long lVar1;
  
  lVar1 = FUN_003e2ab0(param_2,param_1,0x20);
  if (lVar1 == 0) {
    param_1 = 0;
  }
  else {
    lVar1 = FUN_003df240(param_2,(int)param_1 + 0x20,0x20);
    if (lVar1 == 0) {
      param_1 = 0;
    }
  }
  return param_1;
}
