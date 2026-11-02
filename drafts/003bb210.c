/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bb210 func_003bb210 src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned long long FUN_003bb210(unsigned long long param_1)

{
  long lVar1;
  unsigned long long uVar2;
  long lVar3;
  int iVar4;
  unsigned char auStack_30 [32];
  int iStack_10;
  unsigned int uStack_c;
  int uStack_8;
  unsigned int uStack_4;
  
  lVar1 = FUN_003e2910(param_1,auStack_30,0x28);
  if (lVar1 != 0x28) {
    uVar2 = 0;
  }
  else {
    iVar4 = iStack_10 * 0xc;
    lVar1 = (*DAT_008873e8)(iVar4,0x10102);
    if (lVar1 != 0) {
      lVar3 = FUN_003df300(param_1,lVar1,iVar4);
      if (lVar3 == 0) {
        uVar2 = 0;
      }
      else {
        uVar2 = FUN_003bbbe0(iStack_10,uStack_c,lVar1);
        (*DAT_008873b0)((unsigned char *)uVar2 + 0x18,auStack_30,0x20);
        *(unsigned char *)uVar2 = 0xc;
        (*DAT_008873ec)(lVar1);
      }
    }
    else {
      uStack_8 = 0x102;
      uStack_4 = FUN_003df590(0xffffffff80000013,iVar4);
      FUN_003df4d0(&uStack_8);
      uVar2 = 0;
    }
  }
  return uVar2;
}
