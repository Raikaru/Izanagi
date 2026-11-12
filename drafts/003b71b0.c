/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b71b0 func_003b71b0 src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned long long FUN_003b71b0(unsigned long long param_1,int *param_2)

{
  long lVar1;
  
  lVar1 = FUN_003df240(param_1,param_2 + 0xb,4);
  if (lVar1 == 0) {
    param_1 = 0;
  }
  else {
    lVar1 = FUN_003df240(param_1,param_2 + 0xc,4);
    if (lVar1 == 0) {
      param_1 = 0;
    }
    else {
      lVar1 = FUN_003df240(param_1,param_2 + 0xd,4);
      if (lVar1 != 0) {
        if ((param_2[0xc] != 0) &&
           (lVar1 = FUN_003e2ab0(param_1,param_2[0xe],*param_2 + param_2[0xc] * 2 + param_2[0xd] * 2
                                ), lVar1 == 0)) {
          param_1 = 0;
        }
      }
      else {
        param_1 = 0;
      }
    }
  }
  return param_1;
}
