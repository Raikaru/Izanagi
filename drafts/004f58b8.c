/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f58b8 func_004f58b8 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned long long FUN_004f58b8(unsigned long long param_1,long param_2)

{
  int iVar1;
  unsigned long long uVar2;
  long lVar3;
  long lVar4;
  unsigned char auStack_60 [16];
  unsigned char auStack_50 [32];
  
  if (param_2 == 0) {
    uVar2 = FUN_004f53c0(0x75d6c8,0x975,0x75d9b8,0,0xffffffffffffff9c);
  }
  else {
    FUN_0043f9c8(param_2,0,0x20);
    lVar3 = FUN_004f2c08(auStack_60,auStack_50,9,0x20,param_1);
    if (lVar3 == 0) {
      uVar2 = FUN_004f53c0(0x75d6c8,0x97b);
    }
    else {
      lVar3 = FUN_004f2e80(auStack_60,0,0x75d7c0,0,0xffffffffffffff9c);
      if (lVar3 == 0) {
        uVar2 = FUN_004f53c0(0x75d6c8,0x981,0x75d798);
      }
      else {
        iVar1 = *(int *)((int)lVar3 + 0x10);
        lVar4 = FUN_004f8240(auStack_50,iVar1,lVar3,0,0xffffffffffffff97);
        uVar2 = 0;
        if (lVar4 == 0) {
          uVar2 = FUN_004f53c0(0x75d6c8,0x98a,0x75d7e8,param_1,0xffffffffffffff95);
        }
        else {
          *(int *)param_2 = (int)lVar3;
          ((int *)param_2)[1] = *(int *)((iVar1 + (int)lVar4 * 0x30) - 0x10);
        }
      }
    }
  }
  return uVar2;
}
