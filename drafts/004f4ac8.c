/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4ac8 func_004f4ac8 src/sce/rofs_mai.c - romwright draft, unreviewed */

void FUN_004f4ac8(unsigned long long param_1,long param_2,unsigned long long param_3)

{
  int iVar1;
  long lVar2;
  long lVar3;
  int iVar4;
  unsigned int uVar5;
  unsigned char *puVar6;
  unsigned char auStack_240 [16];
  char acStack_230 [512];
  
  lVar2 = FUN_004f2c08(auStack_240,acStack_230,9,0x200,param_1);
  if (lVar2 == 0) {
    puVar6 = &LAB_0075d7c0;
    uVar5 = 0x6ce;
    FUN_004f53c0(0x75d6c8,uVar5,puVar6,0,0xffffffffffffff9c);
    return;
  }
  lVar2 = FUN_004f2e80(auStack_240,0);
  if (lVar2 == 0) {
    FUN_004f53c0(0x75d6c8,0x6d4,0x75d798,0,0xffffffffffffff97);
    return;
  }
  iVar4 = (int)lVar2;
  if (param_2 == 0) {
    iVar1 = *(int *)(iVar4 + 0x10);
  }
  else {
    iVar1 = FUN_004f1750(param_2,param_3);
    if (iVar1 == 0) {
      puVar6 = (unsigned char *)0x75d888;
      uVar5 = 0x6df;
      FUN_004f53c0(0x75d6c8,uVar5,puVar6,0,0xffffffffffffff9c);
      return;
    }
  }
  if ((acStack_230[0] != '/') && (acStack_230[0] != '\\')) {
    lVar3 = FUN_004f8240(acStack_230,*(unsigned int *)(iVar4 + 0x10),lVar2);
    if (lVar3 != 0) {
      iVar4 = *(int *)(iVar4 + 0x10) + (int)lVar3 * 0x30;
      if ((*(unsigned char *)(iVar4 - 0xc) & 2) == 0) {
        FUN_004f53c0(0x75d6c8,0x707,0x75d8b8,0,0xffffffffffffff96);
        return;
      }
      uVar5 = *(unsigned int *)(iVar4 - 0x18);
      iVar4 = *(int *)(iVar4 - 0x10);
      FUN_004f7d88(lVar2,*(unsigned int *)(DAT_00743a50 + 0x48),iVar4,iVar1,uVar5);
      return;
    }
    puVar6 = &LAB_0075d7e8;
    uVar5 = 0x6fe;
  }
  else {
    iVar4 = FUN_004f8478(lVar2,*(unsigned int *)(DAT_00743a50 + 0x48),*(unsigned int *)(iVar4 + 8),
                         *(unsigned int *)(iVar4 + 0xc),acStack_230);
    if (0 < iVar4) {
      uVar5 = 0x800;
      FUN_004f7d88(lVar2,*(unsigned int *)(DAT_00743a50 + 0x48),iVar4,iVar1,uVar5);
      return;
    }
    puVar6 = (unsigned char *)0x75d8a0;
    uVar5 = 0x6f2;
    param_1 = 0;
  }
  FUN_004f53c0(0x75d6c8,uVar5,puVar6,param_1,0xffffffffffffff95);
  return;
}
