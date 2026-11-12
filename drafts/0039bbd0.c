/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039bbd0 func_0039bbd0 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_0039bbd0(unsigned int *param_1)

{
  unsigned long long uVar1;
  unsigned long long uVar2;
  unsigned long long uVar3;
  unsigned long long uVar4;
  unsigned int *puVar5;
  unsigned int uVar6;
  int uStack_8;
  unsigned int uStack_4;
  
  uVar1 = FUN_004115d0();
  *(unsigned int *)((int)uVar1 + 0x2c) = *param_1;
  *(unsigned int *)((int)uVar1 + 0x30) = param_1[1];
  uVar2 = FUN_00412ca0(uVar1);
  uVar3 = FUN_003d3a80();
  uVar2 = FUN_00412fb0(uVar2,0,uVar3,0);
  uVar4 = FUN_00412e90(uVar2,*(unsigned int *)uVar3,0,0);
  uVar6 = 0;
  for (puVar5 = param_1; (uVar6 < 10 && (puVar5[2] != 0)); puVar5 = puVar5 + 2) {
    FUN_003d3130(uVar4,puVar5[2],puVar5[3]);
    uVar6 = uVar6 + 1;
  }
  switch(param_1[0x16]) {
  default:
    uStack_8 = 0x131;
    uStack_4 = FUN_003df590(0);
    FUN_003df4d0(&uStack_8);
    break;
  case 1:
  case 2:
    FUN_003d35f0(uVar4,param_1[0x17],param_1[0x18],param_1[0x19]);
    break;
  case 3:
  case 4:
  case 5:
    FUN_003d3460(uVar4,param_1[0x17],param_1[0x18],param_1[0x19]);
    break;
  case 6:
    FUN_003d3780(uVar4,param_1[0x17],param_1[0x18]);
  }
  uVar2 = FUN_004125e0(uVar2);
  uVar2 = FUN_00412e90(uVar2,*(unsigned int *)uVar3,0,0);
  uVar2 = FUN_003d38e0(uVar2,param_1[0x1a]);
  FUN_003d3920(uVar2,*(unsigned int *)param_1[0x1b],((unsigned int *)param_1[0x1b])[1]);
  uVar2 = FUN_003d39a0(uVar2,1,param_1[0x1c]);
  uVar2 = FUN_003d39a0(uVar2,2,param_1[0x1d]);
  uVar2 = FUN_003d39a0(uVar2,3,param_1[0x1e]);
  uVar2 = FUN_003d39a0(uVar2,4,param_1[0x1f]);
  FUN_003d39a0(uVar2,5,param_1[0x20]);
  return uVar1;
}
