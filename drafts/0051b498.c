/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051b498 func_0051b498 src/cri/sfd_mpv.c - romwright draft, unreviewed */

long FUN_0051b498(unsigned long long param_1,long param_2,unsigned int param_3,long param_4,unsigned int *param_5)

{
  unsigned int uVar1;
  int iVar2;
  long lVar3;
  long lVar4;
  int iVar5;
  unsigned char auStack_70 [16];
  int iStack_60;
  int aiStack_5c [3];
  
  iVar5 = (int)param_1;
  *param_5 = 0;
  uVar1 = *(unsigned int *)(iVar5 + 0x1fc8);
  iVar2 = *(int *)(iVar5 + 0x1fc0);
  *(unsigned int *)(iVar5 + 0x978) = 0;
  lVar3 = FUN_005213a8();
  if (lVar3 == 0) {
    lVar3 = FUN_0051bbe8(param_1);
    if (lVar3 == 1) {
      FUN_0051f5d8(param_1);
      return 0;
    }
    *(unsigned int *)(iVar5 + 0x978) = 1;
    lVar3 = FUN_0051b150(param_1);
    return lVar3;
  }
  if ((*(int *)(iVar2 + 0x78) != 0xcc) || (*(int *)(iVar2 + 0x170) == 0)) {
    param_3 = param_3 & 0xcc;
  }
  lVar3 = FUN_00515280(param_1,uVar1,&iStack_60);
  if (lVar3 != 0) {
    return 0;
  }
  lVar3 = 0;
  if (iStack_60 == 0) {
    return 0;
  }
  if ((param_3 & 200) != 0) {
    FUN_0051f5f8(param_1,1);
  }
  if (param_3 == 0x80) {
    FUN_0051b7b8(param_1);
    lVar4 = FUN_00515c60(param_1);
    if (lVar4 == 0) {
      lVar4 = FUN_00515cc8(param_1,iStack_60);
      if (lVar4 != 0) {
        FUN_0051ba98(param_1,iStack_60);
        *param_5 = 1;
        return lVar3;
      }
      goto LAB_0051b5e8;
    }
    lVar4 = FUN_0051b870(param_1);
  }
  else {
LAB_0051b5e8:
    if (param_4 == 0) {
      lVar4 = FUN_0051bba0(param_1,param_2,param_3);
      if (lVar4 != 0) {
        FUN_0051f5d8(param_1);
        return 0;
      }
      if (param_2 < 5) {
        *(unsigned int *)(iVar5 + 0x978) = 1;
        return 0;
      }
    }
    if ((param_3 & 0x4c) != 0) {
      FUN_0051b768(param_1,auStack_70);
      lVar3 = FUN_0051bc98(param_1,auStack_70,iStack_60,param_3,aiStack_5c);
      if (lVar3 != 0) {
        return lVar3;
      }
      if (aiStack_5c[0] == 0) {
        if ((param_3 & *(unsigned int *)(iVar2 + 0x78)) != 0) {
          *(unsigned int *)(iVar2 + 0x78) = 0xcc;
        }
        *(unsigned int *)(iVar2 + 0x170) = 1;
      }
      lVar3 = 0;
      if ((param_3 == 0x40) && (aiStack_5c[0] == -2)) {
        *(unsigned int *)(iVar2 + 0x78) = 0xc0;
      }
      *param_5 = 1;
      return lVar3;
    }
    if ((param_3 & 2) == 0) {
      if (param_3 == 0x80) {
        return 0;
      }
      lVar4 = FUN_0051e468(param_1,iStack_60,0xcc);
      if (lVar4 < 1) {
        return 0;
      }
      *param_5 = 1;
      return lVar3;
    }
    FUN_0051b768(param_1,auStack_70);
    lVar3 = FUN_0051d238(param_1,auStack_70);
    if (lVar3 == 0) {
      lVar3 = FUN_0051d910(param_1,iStack_60);
      return lVar3;
    }
    lVar3 = FUN_0051d7a8(param_1);
    lVar4 = lVar3;
  }
  if (lVar4 != 0) {
    return lVar3;
  }
  *param_5 = 1;
  return lVar3;
}
