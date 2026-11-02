/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051ace8 func_0051ace8 src/cri/sfd_mpv.c - romwright draft, unreviewed */

long FUN_0051ace8(unsigned long long param_1,int *param_2,unsigned int *param_3,unsigned int *param_4)

{
  int iVar1;
  long lVar2;
  unsigned long long uVar3;
  unsigned int uVar4;
  int iStack_60;
  int iStack_5c;
  int iStack_58;
  int iStack_54;
  unsigned int uStack_40;
  int iStack_3c;
  int iStack_38;
  int uStack_34;
  
  *param_2 = 0;
  uVar4 = *(unsigned int *)((int)param_1 + 0x1fc8);
  *param_3 = 0;
  *param_4 = 0;
  lVar2 = FUN_00514ca8(param_1,uVar4,&iStack_60);
  if (lVar2 != 0) {
    return lVar2;
  }
  if (iStack_5c == 0) {
    return 0;
  }
  iVar1 = FUN_0051b1d8(&iStack_60,0xce,&uStack_40);
  if (iVar1 != iStack_60) {
    if (iVar1 != 0) {
      iVar1 = FUN_0051b0e8(&iStack_60);
      *param_2 = iVar1;
    }
    else {
      iVar1 = (iStack_5c + iStack_54) - 3;
      if (iVar1 < 0) {
        iVar1 = 0;
      }
      *param_2 = iVar1;
    }
    if (0 < *param_2) {
      *param_4 = 1;
      return 0;
    }
    return 0;
  }
  *param_2 = 4;
  *param_3 = uStack_40;
  if ((uStack_40 & 0x80) != 0) {
    return 0;
  }
  FUN_005150c8(param_1,uVar4,&iStack_3c,&iStack_38);
  lVar2 = FUN_0051af28(&iStack_60,uStack_40,iStack_3c);
  if (lVar2 != 0) {
    iStack_3c = 0;
    if (iStack_54 == 0) {
      iStack_60 = iStack_60 + iStack_5c;
    }
    else {
      iStack_60 = iStack_58 + iStack_54;
    }
    if (iStack_38 != iStack_60) {
      iStack_38 = iStack_60;
      uVar3 = FUN_0051b330(&iStack_60,0xcc,&uStack_34);
      iStack_3c = (int)uVar3;
      FUN_00515138(param_1,uVar4,uVar3,iStack_38);
    }
  }
  if (iStack_3c != 0) {
    lVar2 = FUN_00503c58();
    uStack_34 = (unsigned int)lVar2;
    if (lVar2 != 8) {
      if ((8 < lVar2) || (lVar2 != 4)) {
        iVar1 = FUN_0051b0e8(&iStack_60,iStack_3c);
        *param_2 = iVar1;
        return 0;
      }
      uStack_40 = uStack_40 & 0x48;
      uVar4 = 4;
    }
    else {
      uStack_40 = uStack_40 & 0x40;
      uVar4 = 8;
    }
    if ((uStack_40 == 0) ||
       ((iVar1 = FUN_0051b1d8(&iStack_60,uVar4,&uStack_40), iVar1 != 0 && (iVar1 != iStack_3c)))) {
      iVar1 = FUN_0051b0e8(&iStack_60,iStack_3c);
      *param_2 = iVar1;
      return 0;
    }
  }
  lVar2 = FUN_0051b150(param_1);
  return lVar2;
}
