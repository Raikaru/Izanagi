/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00394e70 func_00394e70 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_00394e70(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4)

{
  int iVar1;
  unsigned int *puVar2;
  unsigned long long uVar3;
  int iVar4;
  unsigned char auStack_70 [32];
  unsigned char auStack_50 [64];
  int uStack_10;
  unsigned int uStack_c;
  unsigned int uStack_8;
  unsigned int uStack_4;
  
  iVar1 = ((unsigned int *)param_2)[6];
  if (iVar1 != 5) {
    if (iVar1 != 3) {
      if (iVar1 == 1) {
        iVar4 = (int)param_1;
        iVar1 = *(int *)(iVar4 + 0x18);
        uVar3 = FUN_003e9700(*(unsigned int *)(iVar4 + 4));
        FUN_003e0960(auStack_50,uVar3);
        FUN_003e42e0(auStack_70,param_2,2,auStack_50);
        if (*(int *)(iVar1 + 0x18) >= 2) {
          FUN_003941e0(iVar1,iVar4 + 0x4c,auStack_70,param_2,param_3,param_4);
        }
        else {
          FUN_00392bf0(iVar1,auStack_70,param_2,param_3,param_4);
        }
      }
    }
    else {
      FUN_00394c70(param_1,param_2,param_2,param_3,param_4);
    }
  }
  else {
    puVar2 = (unsigned int *)FUN_003bfae0(*(unsigned int *)param_2);
    uStack_10 = *puVar2;
    uStack_c = puVar2[1];
    uStack_8 = puVar2[2];
    uStack_4 = puVar2[3];
    FUN_00394c70(param_1,&uStack_10,param_2,param_3,param_4);
  }
  return param_1;
}
