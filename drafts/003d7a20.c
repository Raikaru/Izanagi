/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d7a20 func_003d7a20 src/rw/rpuvanim.c - romwright draft, unreviewed */

long FUN_003d7a20(unsigned long long param_1)

{
  long lVar1;
  long lVar2;
  
  lVar1 = (*DAT_008873f8)(DAT_00764838,0x30135);
  if (lVar1 != 0) {
    lVar2 = FUN_003e2910(param_1,lVar1,0x20);
    if (lVar2 == 0) {
      lVar1 = 0;
    }
    else {
      lVar2 = FUN_003df360(param_1,(int)lVar1 + 0x20,0x20);
      if (lVar2 == 0) {
        lVar1 = 0;
      }
      else {
        *(unsigned int *)((int)lVar1 + 0x40) = 1;
      }
    }
  }
  else {
    lVar1 = 0;
  }
  return lVar1;
}
