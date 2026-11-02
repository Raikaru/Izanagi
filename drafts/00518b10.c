/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00518b10 func_00518b10 src/cri/sfd_mps.c - romwright draft, unreviewed */

long FUN_00518b10(unsigned long long param_1,int param_2,unsigned long long param_3,long param_4,unsigned long long param_5)

{
  int iVar1;
  unsigned int uVar2;
  int iVar3;
  long lVar4;
  long lVar5;
  int iVar6;
  char *pcVar7;
  
  lVar4 = FUN_00525098(param_1,5);
  if (lVar4 == 0) {
    return 1;
  }
  iVar1 = *(int *)((int)param_1 + 0x1f7c);
  if (*(int *)(iVar1 + 0x34) != -1) {
    iVar3 = *(int *)(iVar1 + 0x2c);
  }
  else {
    uVar2 = FUN_00518ce8(param_1,param_2);
    *(unsigned int *)(iVar1 + 0x34) = uVar2;
    iVar3 = *(int *)(iVar1 + 0x2c);
  }
  if (iVar3 == -1) {
    *(int *)(iVar1 + 0x2c) = param_2;
  }
  iVar3 = FUN_00525098(param_1,0x1d);
  if (iVar3 != -1) {
    lVar4 = FUN_00525098(param_1,0x37);
    if (lVar4 == 0) {
      iVar6 = *(int *)(iVar1 + 0x34);
      if (param_2 != *(int *)(iVar1 + 0x2c)) goto LAB_00518c44;
    }
    else {
      iVar6 = *(int *)(iVar1 + 0x34);
      if (*(int *)(iVar1 + 0x24) <= param_2) {
        *(int *)(iVar1 + 0x24) = param_2;
        goto LAB_00518c48;
      }
    }
    if (iVar6 != iVar3) {
      if (param_4 < 4) {
        *(int *)(iVar1 + 0x24) = param_2;
        goto LAB_00518c48;
      }
      pcVar7 = (char *)param_3;
      if (*pcVar7 != '\0') {
        *(int *)(iVar1 + 0x24) = param_2;
        goto LAB_00518c48;
      }
      if (pcVar7[1] != '\0') {
        *(int *)(iVar1 + 0x24) = param_2;
        goto LAB_00518c48;
      }
      if (pcVar7[2] != '\x01') {
        *(int *)(iVar1 + 0x24) = param_2;
        goto LAB_00518c48;
      }
      if ((pcVar7[3] != -0x4d) && (pcVar7[3] != -0x48)) {
        *(int *)(iVar1 + 0x24) = param_2;
        goto LAB_00518c48;
      }
      *(int *)(iVar1 + 0x34) = iVar3;
      iVar6 = iVar3;
    }
  }
  else {
    iVar6 = *(int *)(iVar1 + 0x34);
  }
LAB_00518c44:
  *(int *)(iVar1 + 0x24) = param_2;
LAB_00518c48:
  lVar4 = 1;
  if (iVar6 == param_2) {
    lVar5 = FUN_00520dc8(param_1,param_3,param_4);
    lVar4 = 0;
    if (0 < lVar5) {
      lVar4 = FUN_00519008(param_1,*(unsigned int *)((int)param_1 + 0x1f88),param_3,lVar5,param_5);
      if (lVar4 != 1) {
        FUN_00520e38(param_1,lVar5);
      }
      else {
        FUN_00521178(param_1,param_3,lVar5);
      }
    }
  }
  return lVar4;
}
