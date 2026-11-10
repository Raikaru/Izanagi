/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051b8c0 sfmpv_ConcatSub src/cri/sfd_mpv.c - romwright draft, unreviewed */

unsigned int FUN_0051b8c0(unsigned long long param_1)

{
  int iVar1;
  int iVar2;
  long lVar3;
  int iVar4;
  
  iVar4 = (int)param_1;
  iVar1 = *(int *)(iVar4 + 0x1fc0);
  lVar3 = FUN_00525098(param_1,6);
  if (lVar3 == 0) {
    lVar3 = FUN_0051b990();
  }
  else {
    lVar3 = FUN_0051b9f0(param_1);
    if (lVar3 < 0) {
      return 0xffffffff;
    }
  }
  if (0 < lVar3) {
    FUN_00515d10(param_1);
    *(int *)(iVar1 + 0x88) = *(int *)(iVar1 + 0x88) + 1;
    lVar3 = FUN_00520808(param_1);
    if (lVar3 != 0) {
      iVar2 = FUN_00521168(param_1);
      *(int *)(iVar4 + 0xe90) = iVar2 + 1;
    }
  }
  FUN_005256c0(iVar4 + 0xd6c,0x7fffffff);
  FUN_005256c0(iVar4 + 0xd98,0xffffffffffffffff);
  *(unsigned int *)(iVar1 + 0x78) = 0xc0;
  return 0;
}
