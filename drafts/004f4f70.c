/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4f70 func_004f4f70 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned char FUN_004f4f70(unsigned long long param_1)

{
  long lVar1;
  unsigned int uVar2;
  unsigned long long uVar3;
  unsigned char auStack_40 [16];
  unsigned char auStack_30 [32];
  
  lVar1 = FUN_004f2c08(auStack_40,auStack_30,9,0x20,param_1);
  uVar3 = 0x75d7c0;
  if (lVar1 == 0) {
    uVar2 = 0x77f;
  }
  else {
    lVar1 = FUN_004f2e80(auStack_40,0,0x75d7c0,0,0xffffffffffffff9c);
    uVar2 = 0x785;
    uVar3 = 0x75d798;
    if (lVar1 != 0) {
      lVar1 = FUN_004f8240(auStack_30,*(unsigned int *)((int)lVar1 + 0x10),lVar1,0,0xffffffffffffff97)
      ;
      return lVar1 != 0;
    }
  }
  FUN_004f53c0(0x75d6c8,uVar2,uVar3);
  return 0;
}
