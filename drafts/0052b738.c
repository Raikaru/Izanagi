/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0052b738 sfx_InitHn src/cri/sfx_lib.c - romwright draft, unreviewed */

long FUN_0052b738(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4)

{
  long lVar1;
  long lVar2;
  unsigned long long uVar3;
  
  lVar1 = FUN_0052b850();
  if (lVar1 != 0) {
    lVar2 = FUN_0052b940(param_2,param_3,param_4);
    if (lVar2 != 1) {
      FUN_0052b9a0(0,0,0x760dc0);
    }
    else {
      FUN_0052b8a0(lVar1,param_1,param_2);
      lVar2 = FUN_0052bf58();
      uVar3 = 0x760df0;
      if (lVar2 != 0) {
        *(int *)((int)lVar1 + 0x24) = (int)lVar2;
        lVar2 = FUN_0052a498(0,0,0x760df0);
        uVar3 = 0x760e10;
        if (lVar2 != 0) {
          *(int *)((int)lVar1 + 0x30) = (int)lVar2;
          DAT_00745c70 = DAT_00745c70 + 1;
          return lVar1;
        }
      }
      FUN_0052b9a0(0,0,uVar3);
      FUN_0052b950(lVar1);
    }
  }
  return 0;
}
