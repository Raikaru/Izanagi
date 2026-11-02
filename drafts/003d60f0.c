/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d60f0 func_003d60f0 src/rw/rtanim.c - romwright draft, unreviewed */

unsigned int FUN_003d60f0(int *param_1)

{
  unsigned int *puVar1;
  unsigned int *puVar2;
  
  puVar1 = (unsigned int *)FUN_003df8a0(param_1[1]);
  for (puVar2 = (unsigned int *)FUN_003df890(param_1[1]); puVar2 != puVar1; puVar2 = puVar2 + 1) {
    (**(code **)(*param_1 + 0x1c))(*puVar2);
  }
  FUN_003df7f0(param_1[1]);
  return 1;
}
