/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00394d70 func_00394d70 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_00394d70(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4)

{
  int iVar1;
  unsigned int *puVar2;
  int uStack_10;
  unsigned int uStack_c;
  unsigned int uStack_8;
  unsigned int uStack_4;
  
  iVar1 = ((unsigned int *)param_2)[6];
  if (iVar1 != 5) {
    if (iVar1 != 4) {
      if (iVar1 != 3) {
        if (iVar1 == 1) {
          FUN_00390680();
        }
      }
      else {
        FUN_00391c10(param_1,param_2,param_2,param_3,param_4);
      }
    }
    else {
      FUN_00391400();
    }
  }
  else {
    puVar2 = (unsigned int *)FUN_003bfae0(*(unsigned int *)param_2);
    uStack_10 = *puVar2;
    uStack_c = puVar2[1];
    uStack_8 = puVar2[2];
    uStack_4 = puVar2[3];
    FUN_00391c10(param_1,&uStack_10,param_2,param_3,param_4);
  }
  return param_1;
}
