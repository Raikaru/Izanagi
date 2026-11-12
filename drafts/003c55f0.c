/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c55f0 func_003c55f0 src/promoted/code1_003c.c - romwright draft, unreviewed */

long FUN_003c55f0(long param_1)

{
  int iVar1;
  long lVar2;
  int iVar3;
  int *piVar4;
  int uStack_10;
  unsigned int uStack_c;
  int uStack_8;
  unsigned int uStack_4;
  
  lVar2 = (*DAT_008873f8)(DAT_007647d8,0x30502);
  if (lVar2 != 0) {
    piVar4 = (int *)lVar2;
    piVar4[1] = 0;
    if (param_1 == 0) {
      piVar4[2] = 0;
      *piVar4 = 0;
    }
    else {
      iVar3 = (int)param_1 * 0x14;
      iVar1 = (*DAT_008873e8)(iVar3,0x1030502);
      piVar4[2] = iVar1;
      if (piVar4[2] != 0) {
        *piVar4 = (int)param_1;
      }
      else {
        (*DAT_008873fc)(DAT_007647d8,lVar2);
        uStack_8 = 2;
        uStack_4 = FUN_003df590(0xffffffff80000013,iVar3);
        FUN_003df4d0(&uStack_8);
        lVar2 = 0;
      }
    }
  }
  else {
    uStack_10 = 2;
    uStack_c = FUN_003df590(0xffffffff80000013,0xc);
    FUN_003df4d0(&uStack_10);
    lVar2 = 0;
  }
  return lVar2;
}
