/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003e0e20 func_003e0e20 src/promoted/code1_003e.c - romwright draft, unreviewed */

unsigned int * FUN_003e0e20(unsigned int *param_1,unsigned int *param_2,long param_3)

{
  unsigned int uVar1;
  int iVar2;
  unsigned int *puVar3;
  unsigned int *puVar4;
  unsigned int auStack_90 [16];
  unsigned int auStack_50 [18];
  int uStack_8;
  unsigned int uStack_4;
  
  if (param_3 != 2) {
    if (param_3 != 1) {
      if (param_3 != 0) {
        uStack_8 = 1;
        uStack_4 = FUN_003df590(0xffffffff80000003,0x70b6f0);
        FUN_003df4d0(&uStack_8);
        param_1 = (unsigned int *)0x0;
      }
      else {
        iVar2 = 8;
        puVar3 = param_1;
        do {
          iVar2 = iVar2 - 1;
          uVar1 = param_2[1];
          *puVar3 = *param_2;
          puVar3[1] = uVar1;
          param_2 = param_2 + 2;
          puVar3 = puVar3 + 2;
        } while (0 < iVar2);
      }
    }
    else {
      FUN_003e05f0(auStack_50,param_2,param_1);
      puVar4 = auStack_50;
      iVar2 = 8;
      puVar3 = param_1;
      do {
        iVar2 = iVar2 - 1;
        uVar1 = puVar4[1];
        *puVar3 = *puVar4;
        puVar4 = puVar4 + 2;
        puVar3[1] = uVar1;
        puVar3 = puVar3 + 2;
      } while (0 < iVar2);
    }
  }
  else {
    FUN_003e05f0(auStack_90,param_1,param_2);
    puVar4 = auStack_90;
    iVar2 = 8;
    puVar3 = param_1;
    do {
      iVar2 = iVar2 - 1;
      uVar1 = puVar4[1];
      *puVar3 = *puVar4;
      puVar4 = puVar4 + 2;
      puVar3[1] = uVar1;
      puVar3 = puVar3 + 2;
    } while (0 < iVar2);
  }
  return param_1;
}
