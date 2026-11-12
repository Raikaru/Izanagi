/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051f228 func_0051f228 src/cri/sfd_mpv.c - romwright draft, unreviewed */

long FUN_0051f228(unsigned long long param_1)

{
  int iVar1;
  unsigned int uVar2;
  long lVar3;
  int aiStack_30 [4];
  
  iVar1 = *(int *)((int)param_1 + 0x1fc0);
  lVar3 = FUN_0051f298(param_1,iVar1,aiStack_30);
  if (lVar3 == 0) {
    *(unsigned int *)(iVar1 + 0x74) = 2;
    if ((aiStack_30[0] == 0) || (lVar3 = FUN_00525098(param_1,0x30), lVar3 == 0)) {
      uVar2 = 0xc0;
    }
    else {
      uVar2 = 200;
    }
    *(unsigned int *)(iVar1 + 0x78) = uVar2;
    lVar3 = 0;
  }
  return lVar3;
}
