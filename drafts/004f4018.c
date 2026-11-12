/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4018 func_004f4018 src/sce/rofs_mai.c - romwright draft, unreviewed */

int FUN_004f4018(unsigned long long param_1,int param_2)

{
  long lVar1;
  long lVar2;
  int iVar3;
  unsigned long long uVar4;
  unsigned long long uVar5;
  unsigned long long uVar6;
  int iVar7;
  unsigned char auStack_70 [16];
  unsigned char auStack_60 [32];
  
  lVar1 = FUN_004f2c08(auStack_70,auStack_60,9,0x20,param_1);
  if (lVar1 == 0) {
    uVar5 = 0x75d7c0;
    uVar4 = 0x4df;
    param_1 = 0;
    uVar6 = 0xffffffffffffff9c;
  }
  else {
    lVar1 = FUN_004f2e80(auStack_70,0);
    if (lVar1 == 0) {
      uVar5 = 0x75d798;
      uVar4 = 0x4e5;
      param_1 = 0;
      uVar6 = 0xffffffffffffff97;
    }
    else {
      if (param_2 == 0) {
        param_2 = *(int *)((int)lVar1 + 0x10);
      }
      else {
        lVar2 = FUN_004426e8(0x75d7d0,param_2 + 0xc);
        if ((lVar2 != 0) && (lVar2 = FUN_004426e8((int)lVar1 + 0x16,param_2 + 0xc), lVar2 != 0)) {
          uVar5 = 0x75d7e0;
          uVar4 = 0x4f4;
          param_1 = 0;
          uVar6 = 0xffffffffffffff94;
          FUN_004f53c0(0x75d6c8,uVar4,uVar5,param_1,uVar6);
          return 0;
        }
      }
      lVar2 = FUN_004f8240(auStack_60,param_2,lVar1);
      if (lVar2 == 0) {
        uVar5 = 0x75d7e8;
        uVar4 = 0x4fe;
        uVar6 = 0xffffffffffffff95;
      }
      else {
        iVar3 = 0;
        iVar7 = *(int *)(DAT_00743a50 + 0x24);
        if (0 < *(int *)(DAT_00743a50 + 4) - 1) {
          do {
            if (*(short *)(iVar7 + 0x34) == 0) {
              param_2 = param_2 + (int)lVar2 * 0x30;
              uVar4 = FUN_004f5860(*(unsigned int *)(param_2 - 0x14),*(unsigned int *)(param_2 - 0x18));
              FUN_004f16b0(iVar7,lVar2,*(unsigned int *)(param_2 - 0x10),uVar4,lVar1);
              if (iVar7 != 0) {
                return iVar7;
              }
              FUN_004f53c0(0x75d6c8,0x515,0x75d800,0,0xffffffffffffff9a);
              return 0;
            }
            iVar3 = iVar3 + 1;
            iVar7 = iVar7 + 0x3c;
          } while (iVar3 < *(int *)(DAT_00743a50 + 4) - 1);
        }
        uVar5 = 0x75d810;
        uVar4 = 0x51c;
        param_1 = 0;
        uVar6 = 0xffffffffffffff9a;
      }
    }
  }
  FUN_004f53c0(0x75d6c8,uVar4,uVar5,param_1,uVar6);
  return 0;
}
