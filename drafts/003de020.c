/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003de020 func_003de020 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned int FUN_003de020(unsigned long long param_1,unsigned int param_2,long param_3)

{
  int iVar1;
  int iVar2;
  unsigned char auStack_30 [16];
  unsigned char auStack_20 [16];
  unsigned char auStack_10 [16];
  
  iVar2 = (int)param_1;
  iVar1 = *(int *)(iVar2 + 0x50);
  if (*(int *)(iVar2 + 0x30) != 0) {
    *(unsigned int *)(iVar2 + 0x40) = 4;
    *(unsigned int *)(iVar2 + 0x38) = 3;
  }
  if (param_3 != 0) {
    if (param_3 != 2) {
      if (param_3 != 1) {
        return 0xffffffff;
      }
      (**(code **)(iVar1 + 0x38))(auStack_10,param_1,param_2);
    }
    else {
      (**(code **)(iVar1 + 0x38))(auStack_20,param_1,param_2,3);
    }
  }
  else {
    (**(code **)(iVar1 + 0x38))(auStack_30,param_1,param_2,1);
  }
  return 0;
}
