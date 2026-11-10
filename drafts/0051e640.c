/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0051e640 func_0051e640 src/cri/sfd_mpv.c - romwright draft, unreviewed */

long FUN_0051e640(unsigned long long param_1)

{
  long lVar1;
  long lVar2;
  unsigned long long uVar3;
  int iVar4;
  unsigned int *puVar5;
  
  iVar4 = (int)param_1;
  puVar5 = (unsigned int *)(iVar4 + 0x23a8);
  lVar1 = FUN_00525098(param_1,5);
  lVar2 = 0;
  if (lVar1 != 0) {
    *(unsigned int **)(iVar4 + 0x1fc0) = puVar5;
    lVar2 = FUN_0051e7a0(param_1,puVar5);
    if (lVar2 == 0) {
      FUN_005208c8(param_1);
      lVar2 = FUN_005070d0();
      if (lVar2 == 0) {
        lVar2 = FUN_00517ad0(0,0xffffffffff000f0a);
        return lVar2;
      }
      lVar1 = FUN_00504a28(lVar2,0x51eb88,param_1);
      if (lVar1 != 0) {
        FUN_0051ed58(lVar2);
        lVar2 = FUN_00517ad0(0,0xffffffffff000f0b);
        return lVar2;
      }
      uVar3 = FUN_00525098(param_1,0);
      FUN_005072d8(lVar2,1,uVar3);
      uVar3 = FUN_00525098(param_1,1);
      FUN_005072d8(lVar2,2,uVar3);
      FUN_005072d8(lVar2,6,*(unsigned int *)(iVar4 + 0x38));
      *puVar5 = (int)lVar2;
      lVar2 = FUN_005230c8();
      if (lVar2 != 0) {
        FUN_0051a518(param_1,DAT_00938920,DAT_00938924,DAT_00938928);
      }
      lVar2 = 0;
    }
  }
  return lVar2;
}
