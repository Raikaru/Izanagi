/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004c0928 ACSSVR_CreatSnd src/cri/acssvr.c - romwright draft, unreviewed */

unsigned long long FUN_004c0928(int *param_1,unsigned long long param_2,int param_3,int param_4)

{
  long lVar1;
  unsigned int uVar2;
  int *piVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  unsigned long long uVar7;
  
  uVar7 = 0;
  lVar1 = FUN_004c0e40();
  if (lVar1 == 0) {
    FUN_004bd6a0(0x7573e8);
  }
  else {
    piVar3 = param_1;
    for (iVar4 = 0; iVar5 = param_3, iVar6 = param_4, iVar4 < 0x10; iVar4 = iVar4 + 1) {
      if (*piVar3 == 0) {
        iVar5 = param_3 + 0xd0;
        iVar6 = param_4 - 0xd0;
        uVar7 = FUN_004be418(param_3,param_4);
        *piVar3 = (int)uVar7;
        break;
      }
      piVar3 = piVar3 + 1;
    }
    if (iVar4 == 0x10) {
      uVar2 = 0x757418;
    }
    else {
      lVar1 = FUN_004be4a8(uVar7,param_2,iVar5,iVar6);
      if (lVar1 != 0) {
        return uVar7;
      }
      uVar2 = 0x757458;
    }
    FUN_004bd6a0(uVar2);
    FUN_004c0a40(param_1,uVar7);
  }
  return 0;
}
