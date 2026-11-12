/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea308 mfCiReqRd src/cri/mfci.c - romwright draft, unreviewed */

unsigned int FUN_004ea308(long param_1,int param_2,long param_3)

{
  int iVar1;
  int iVar2;
  unsigned long long uVar3;
  int iVar4;
  int aiStack_40 [4];
  
  if (param_1 == 0) {
    uVar3 = 0x75bf40;
  }
  else if (param_2 < 0) {
    uVar3 = 0x75bf60;
  }
  else {
    if (param_3 != 0) {
      iVar4 = (int)param_1;
      if (param_2 == 0) {
        *(unsigned char *)(iVar4 + 1) = 1;
        return 0;
      }
      if (*(char *)(iVar4 + 1) == '\x02') {
        return 0;
      }
      FUN_004e9e20();
      *(unsigned int *)(iVar4 + 0x14) = 0;
      iVar1 = *(int *)(iVar4 + 0xc) - *(int *)(iVar4 + 0x10);
      if (param_2 < iVar1) {
        iVar1 = param_2;
      }
      iVar2 = iVar1 * *(int *)(iVar4 + 4);
      *(int *)(iVar4 + 0x18) = iVar1;
      if (iVar2 == 0) {
        *(unsigned char *)(iVar4 + 1) = 1;
        FUN_004e9e38();
        return 0;
      }
      *(int *)(iVar4 + 0x30) = *(int *)(iVar4 + 0x10) * *(int *)(iVar4 + 4);
      *(int *)(iVar4 + 0x34) = iVar2;
      *(unsigned char *)(iVar4 + 1) = 2;
      iVar2 = FUN_004e9eb0(iVar4 + 0x1c,aiStack_40);
      aiStack_40[0] = aiStack_40[0] - *(int *)(iVar4 + 0x30);
      iVar1 = *(int *)(iVar4 + 0x34);
      if (aiStack_40[0] < *(int *)(iVar4 + 0x34)) {
        iVar1 = aiStack_40[0];
      }
      FUN_004e9e38();
      FUN_0043f810(param_3,iVar2 + *(int *)(iVar4 + 0x30),*(unsigned int *)(iVar4 + 0x34));
      FUN_0043f9c8((int)param_3 + iVar1,0,*(int *)(iVar4 + 0x34) - iVar1);
      FUN_004e9e20();
      *(unsigned char *)(iVar4 + 1) = 1;
      *(int *)(iVar4 + 0x10) = *(int *)(iVar4 + 0x10) + *(int *)(iVar4 + 0x18);
      *(int *)(iVar4 + 0x14) = *(int *)(iVar4 + 0x18) * *(int *)(iVar4 + 4);
      FUN_004e9e38();
      return *(unsigned int *)(iVar4 + 0x18);
    }
    uVar3 = 0x75bf80;
  }
  FUN_004e9e68(param_1,uVar3);
  return 0;
}
