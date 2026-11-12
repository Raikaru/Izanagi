/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bbbe0 func_003bbbe0 src/promoted/code1_003b.c - romwright draft, unreviewed */

long FUN_003bbbe0(int param_1,long param_2,unsigned long long param_3)

{
  long lVar1;
  long lVar2;
  unsigned char *puVar3;
  int iVar4;
  int uStack_20;
  unsigned int uStack_1c;
  int uStack_18;
  unsigned int uStack_14;
  int uStack_10;
  unsigned int uStack_c;
  int uStack_8;
  unsigned int uStack_4;
  
  if (param_2 != 2) {
    if (param_2 != 1) {
      uStack_18 = 0x102;
      uStack_14 = FUN_003df590();
      FUN_003df4d0(&uStack_18);
      return 0;
    }
    iVar4 = (param_1 + 1) * 0xc + 0x44;
    lVar1 = (*DAT_008873e8)(iVar4,0x3000c);
    if (lVar1 == 0) {
      uStack_8 = 0x102;
      uStack_4 = FUN_003df590(0xffffffff80000013,iVar4);
      FUN_003df4d0(&uStack_8);
      return 0;
    }
    *(int *)((int)lVar1 + 8) = param_1 + 2;
  }
  else {
    iVar4 = (param_1 + 2) * 0xc + 0x44;
    lVar1 = (*DAT_008873e8)(iVar4,0x3000c);
    if (lVar1 == 0) {
      uStack_10 = 0x102;
      uStack_c = FUN_003df590(0xffffffff80000013,iVar4);
      FUN_003df4d0(&uStack_10);
      return 0;
    }
    *(int *)((int)lVar1 + 8) = param_1 + 3;
  }
  puVar3 = (unsigned char *)lVar1;
  *(unsigned int *)(puVar3 + 0x10) = 0;
  puVar3[0x18] = 0;
  *(int *)(puVar3 + 0xc) = (int)param_2;
  *puVar3 = 0xc;
  lVar2 = FUN_003b9a40(lVar1,param_3);
  if (lVar2 != 0) {
    *puVar3 = 0xc;
    lVar2 = (*DAT_008873e8)(param_1 * 0xc,0x3000c);
    *(int *)(puVar3 + 0x14) = (int)lVar2;
    if (lVar2 != 0) {
      FUN_0043f810(*(unsigned int *)(puVar3 + 0x14),param_3,param_1 * 0xc);
    }
    else {
      (*DAT_008873ec)(lVar1);
      uStack_20 = 0x102;
      uStack_1c = FUN_003df590(0xffffffff80000013,param_1 * 0xc);
      FUN_003df4d0(&uStack_20);
      lVar1 = 0;
    }
  }
  else {
    (*DAT_008873ec)(lVar1);
    lVar1 = 0;
  }
  return lVar1;
}
