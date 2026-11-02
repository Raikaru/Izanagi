/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b42e0 _rpPTankGENInstance src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned int FUN_003b42e0(unsigned long long param_1,unsigned long long param_2,unsigned int param_3,unsigned long long param_4)

{
  int iVar1;
  code *pcVar2;
  unsigned char auStack_50 [16];
  int aiStack_40 [2];
  int aiStack_38 [2];
  int aiStack_30 [2];
  int aiStack_28 [2];
  unsigned char auStack_20 [12];
  unsigned char auStack_14 [20];
  
  if ((param_4 & 0x1e8000) != 0) {
    param_3 = **(unsigned int **)((int)param_1 + DAT_00764700);
  }
  iVar1 = *(int *)((int)param_1 + DAT_00764700);
  if (*(int *)(iVar1 + 0x24) == 0) {
    pcVar2 = *(code **)(iVar1 + 0x28);
  }
  else {
    FUN_0043f9c8(auStack_50,0,0x48);
    (**(code **)(iVar1 + 0x24))(auStack_50,param_1,param_2,param_3,param_4);
    pcVar2 = *(code **)(iVar1 + 0x28);
  }
  if ((pcVar2 != (code *)0x0) && (aiStack_40[0] != 0)) {
    (*pcVar2)(aiStack_40,auStack_20,auStack_14,param_3,param_2);
  }
  if ((*(code **)(iVar1 + 0x2c) != (code *)0x0) && (aiStack_38[0] != 0)) {
    (**(code **)(iVar1 + 0x2c))(aiStack_38,param_3,param_2);
  }
  if ((*(code **)(iVar1 + 0x30) != (code *)0x0) && (aiStack_30[0] != 0)) {
    (**(code **)(iVar1 + 0x30))(aiStack_30,param_3,param_2);
  }
  if ((*(code **)(iVar1 + 0x34) != (code *)0x0) && (aiStack_28[0] != 0)) {
    (**(code **)(iVar1 + 0x34))(aiStack_28,param_3,param_2);
  }
  if (*(code **)(iVar1 + 0x38) != (code *)0x0) {
    (**(code **)(iVar1 + 0x38))(auStack_50,param_1,param_2,param_3,param_4);
  }
  return 1;
}
