/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bc9c0 func_003bc9c0 src/promoted/code1_003b.c - romwright draft, unreviewed */

int FUN_003bc9c0(int *param_1,long param_2,unsigned long long param_3,int param_4)

{
  int iVar1;
  long lVar2;
  int iVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  
  iVar1 = *param_1;
  iVar6 = -1;
  if (0 < iVar1) {
    iVar3 = param_1[1];
    iVar4 = 0;
    do {
      iVar6 = iVar4;
      if (*(int *)(iVar3 + 0xc) == 0) break;
      iVar4 = iVar6 + 1;
      iVar3 = iVar3 + 0x10;
      iVar6 = -1;
    } while (iVar4 < iVar1);
  }
  if (iVar6 == -1) {
    if (param_1[1] != 0) {
      lVar2 = (*DAT_008873e8)((iVar1 + 1) * 0x10,0x3011f);
      if (lVar2 == 0) {
        return -1;
      }
      FUN_0043f810(lVar2,param_1[1],*param_1 << 4);
      (*DAT_008873ec)(param_1[1]);
      param_1[1] = (int)lVar2;
    }
    else {
      iVar1 = (*DAT_008873e8)((iVar1 + 1) * 0x10,0x3011f);
      param_1[1] = iVar1;
      if (param_1[1] == 0) {
        return -1;
      }
    }
    iVar6 = *param_1;
    *param_1 = iVar6 + 1;
  }
  piVar5 = (int *)(param_1[1] + iVar6 * 0x10);
  iVar1 = FUN_003bd110(param_3);
  iVar1 = (*DAT_008873e8)(param_4 * iVar1,0x3011f);
  piVar5[3] = iVar1;
  if (piVar5[3] != 0) {
    iVar1 = FUN_003bd110(param_3);
    FUN_0043f9c8(piVar5[3],0,param_4 * iVar1);
    *piVar5 = 0;
    if (param_2 != 0) {
      iVar1 = (*DAT_008873d4)(param_2);
      iVar1 = (*DAT_008873e8)(iVar1 + 1,0x30002);
      *piVar5 = iVar1;
      if (*piVar5 != 0) {
        (*DAT_008873ac)(*piVar5,param_2);
      }
    }
    piVar5[1] = (int)param_3;
    piVar5[2] = param_4;
  }
  else {
    iVar6 = -1;
  }
  return iVar6;
}
