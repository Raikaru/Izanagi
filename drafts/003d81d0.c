/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d81d0 func_003d81d0 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned int FUN_003d81d0(unsigned long long param_1,unsigned long long param_2,int param_3)

{
  int iVar1;
  unsigned int uVar2;
  int iStack_10;
  int iStack_c;
  unsigned short uStack_6;
  unsigned short uStack_4;
  unsigned char uStack_2;
  unsigned char uStack_1;
  
  uStack_1 = 0x42;
  FUN_003e2ab0(param_1,&uStack_1,1);
  uStack_2 = 0x4d;
  FUN_003e2ab0(param_1,&uStack_2,1);
  uVar2 = (unsigned int)param_2;
  if (param_2 < 9) {
    iVar1 = (1 << (uVar2 & 0x1f)) << 2;
  }
  else {
    iVar1 = 0;
  }
  iStack_c = iVar1 + 0x36 +
             *(int *)(param_3 + 8) * (((*(int *)(param_3 + 4) * uVar2 + 0x1f) & 0xffffffe0) >> 3);
  FUN_003df240(param_1,&iStack_c,4);
  uStack_4 = 0;
  FUN_003df270(param_1,&uStack_4,2);
  uStack_6 = 0;
  FUN_003df270(param_1,&uStack_6,2);
  if (param_2 < 9) {
    iStack_10 = (1 << (uVar2 & 0x1f)) << 2;
  }
  else {
    iStack_10 = 0;
  }
  iStack_10 = iStack_10 + 0x36;
  FUN_003df240(param_1,&iStack_10,4);
  return 0xe;
}
