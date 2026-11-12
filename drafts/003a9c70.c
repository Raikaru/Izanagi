/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a9c70 _rpPTankSkyClonePipeAllocCallBack src/promoted/code1_003a.c - romwright draft, unreviewed */

long FUN_003a9c70(int param_1,int param_2,unsigned long long param_3)

{
  long lVar1;
  int iVar2;
  unsigned int *puVar3;
  int iVar4;
  unsigned int *puStack_8;
  
  iVar2 = param_2 * 0x40 + 0x1c;
  if ((param_3 & 2) != 0) {
    iVar2 = iVar2 + param_2 * 4;
  }
  lVar1 = (*DAT_008873e8)(iVar2 + 0x10,0x3012f);
  if (lVar1 != 0) {
    puVar3 = (unsigned int *)(((int)lVar1 + 0xfU) & 0xfffffff0);
    puVar3[1] = 0;
    puVar3[2] = 0;
    *puVar3 = 0;
    *(unsigned int **)(param_1 + 0x4c) = puVar3;
    FUN_0043f9c8(param_1 + 4,0,0x48);
    *(unsigned int **)(param_1 + 0xc) = puVar3 + 4;
    *(unsigned int *)(param_1 + 0x10) = 0x40;
    *(unsigned int *)(param_1 + 0x50) = 1;
    iVar2 = 0;
    *(unsigned int *)(param_1 + 0x54) = 0;
    *(unsigned int *)(param_1 + 0x58) = 8;
    puStack_8 = *(unsigned int **)(param_1 + 0xc);
    iVar4 = *(int *)(param_1 + 0x10);
    if (0 < param_2) {
      do {
        iVar2 = iVar2 + 1;
        puStack_8[10] = 0x3f800000;
        puStack_8[5] = 0x3f800000;
        *puStack_8 = 0x3f800000;
        puStack_8[4] = 0;
        puStack_8[2] = 0;
        puStack_8[1] = 0;
        puStack_8[9] = 0;
        puStack_8[8] = 0;
        puStack_8[6] = 0;
        puStack_8[0xe] = 0;
        puStack_8[0xd] = 0;
        puStack_8[0xc] = 0;
        puStack_8[3] = puStack_8[3] | 0x20003;
        puStack_8 = (unsigned int *)((int)puStack_8 + iVar4);
      } while (iVar2 < param_2);
    }
    if ((param_3 & 2) != 0) {
      *(int *)(param_1 + 0x24) = *(int *)(param_1 + 0xc) + param_2 * 0x40;
      *(unsigned int *)(param_1 + 0x28) = 4;
      *(int *)(param_1 + 0x50) = *(int *)(param_1 + 0x50) + 1;
      *(unsigned int *)(param_1 + 0x58) = *(unsigned int *)(param_1 + 0x58) | 2;
      FUN_0043f9c8(*(unsigned int *)(param_1 + 0x24),0xff,param_2 << 2);
    }
  }
  else {
    lVar1 = 0;
  }
  return lVar1;
}
