/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c4a80 func_003c4a80 src/promoted/code1_003c.c - romwright draft, unreviewed */

int FUN_003c4a80(int *param_1,int param_2)

{
  long lVar1;
  int iVar2;
  int uStack_8;
  unsigned int uStack_4;
  
  if (param_1[1] >= param_1[2]) {
    iVar2 = (param_1[2] + 0x14) * 4;
    if (*param_1 != 0) {
      lVar1 = (*DAT_008873f0)(*param_1,iVar2,0x1030008);
    }
    else {
      lVar1 = (*DAT_008873e8)(iVar2,0x1030008);
    }
    if (lVar1 != 0) {
      *param_1 = (int)lVar1;
      param_1[2] = param_1[2] + 0x14;
      *(int *)((int)lVar1 + param_1[1] * 4) = param_2;
      *(short *)(param_2 + 0x18) = *(short *)(param_2 + 0x18) + 1;
      param_1[1] = param_1[1] + 1;
      iVar2 = param_1[1] - 1;
    }
    else {
      uStack_8 = 2;
      uStack_4 = FUN_003df590(0xffffffff80000013,iVar2);
      FUN_003df4d0(&uStack_8);
      iVar2 = -1;
    }
  }
  else {
    *(int *)(*param_1 + param_1[1] * 4) = param_2;
    *(short *)(param_2 + 0x18) = *(short *)(param_2 + 0x18) + 1;
    param_1[1] = param_1[1] + 1;
    iVar2 = param_1[1] - 1;
  }
  return iVar2;
}
