/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00514cc0 func_00514cc0 src/cri/sfd_buf.c - romwright draft, unreviewed */

unsigned long long FUN_00514cc0(int param_1,int param_2,unsigned int *param_3,unsigned long long param_4)

{
  int iVar1;
  int iVar2;
  int uStack_30;
  unsigned int uStack_2c;
  int uStack_20;
  unsigned int uStack_1c;
  
  param_3[1] = 0;
  param_3[3] = 0;
  iVar2 = param_1 + 0x1310 + param_2 * 0x74;
  param_3[5] = 0;
  param_3[6] = 0;
  *param_3 = 0;
  iVar1 = *(int *)(iVar2 + 4);
  param_3[2] = 0;
  param_3[4] = 0;
  iVar2 = *(int *)(iVar2 + 0x14);
  if ((iVar1 != 0) && (iVar2 != 0)) {
    FUN_00514f78(iVar2,param_4,&uStack_30,&uStack_20);
    *param_3 = uStack_30;
    param_3[3] = uStack_1c;
    param_3[1] = uStack_2c;
    param_3[2] = uStack_20;
  }
  return 0;
}
