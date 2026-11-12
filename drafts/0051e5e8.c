/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051e5e8 sfmpv_UpdateFlowCnt src/cri/sfd_mpv.c - romwright draft, unreviewed */

void FUN_0051e5e8(unsigned long long param_1)

{
  unsigned long long uVar1;
  int iVar2;
  int iStack_20;
  int uStack_1c;
  unsigned char auStack_18 [8];
  
  iVar2 = (int)param_1;
  FUN_00515280(param_1,*(unsigned int *)(iVar2 + 0x1fc8),&iStack_20);
  if (iStack_20 != 0) {
    FUN_00515a10(iStack_20,&uStack_1c,auStack_18);
    uVar1 = FUN_00515bc8(*(unsigned long long *)(iVar2 + 0x9a0),uStack_1c);
    *(unsigned long long *)(iVar2 + 0x9a0) = uVar1;
  }
  return;
}
