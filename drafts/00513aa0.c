/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00513aa0 func_00513aa0 src/cri/sfd_adxt.c - romwright draft, unreviewed */

unsigned long long FUN_00513aa0(unsigned long long param_1,long param_2)

{
  unsigned int *puVar1;
  unsigned int uVar2;
  int iVar3;
  long lVar4;
  unsigned long long uVar5;
  int iVar6;
  long lStack_50;
  unsigned long long uStack_48;
  int aiStack_40 [4];
  
  puVar1 = *(unsigned int **)((int)param_1 + 0x2004);
  uVar2 = *puVar1;
  uVar5 = 1;
  if (param_2 != 1) {
    if (param_2 != 0) {
      if (param_2 != 2) {
        return 0;
      }
      lVar4 = FUN_00513380(uVar2);
      if (lVar4 == 0) {
        return 0;
      }
      uVar5 = FUN_004d4bd0(uVar2);
      FUN_00513c68(param_1,aiStack_40,uVar5);
      iVar6 = aiStack_40[0] + puVar1[0xc];
      iVar3 = FUN_004d63a0(uVar2,iVar6);
      puVar1[0xc] = iVar6 - iVar3;
      lStack_50 = (long)aiStack_40[0];
      uStack_48 = uVar5;
      FUN_00527b28((int)param_1 + 0x1020,&lStack_50);
      return 0;
    }
    puVar1[0xc] = 0;
    uVar5 = 0;
  }
  FUN_00513be0(param_1,uVar5);
  return 0;
}
