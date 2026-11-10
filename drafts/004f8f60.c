/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f8f60 rsu_ScrambleVariKey src/promoted/code1_004f.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x004f8fa8) */

void FUN_004f8f60(int param_1,unsigned long long param_2,int *param_3)

{
  unsigned char auStack_40 [16];
  short sStack_30;
  unsigned short uStack_2e;
  unsigned short auStack_2c [6];
  
  FUN_004f8d98(param_1 * 0x100001,auStack_40);
  FUN_004f8b18(auStack_40,4,&sStack_30,&uStack_2e,auStack_2c);
  *param_3 = (int)sStack_30 % 9;
  FUN_004f8d60(uStack_2e,param_2);
  FUN_004f8d60(auStack_2c[0],(int)param_2 + 2);
  return;
}
