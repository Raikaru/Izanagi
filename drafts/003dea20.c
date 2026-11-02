/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dea20 func_003dea20 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned long long FUN_003dea20(int param_1,int param_2,int param_3,unsigned long long param_4,unsigned long long param_5)

{
  unsigned char bVar1;
  int iVar2;
  unsigned int uVar3;
  long lVar4;
  unsigned long long uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  
  lVar4 = FUN_003dd900(param_5);
  if (lVar4 != 0) {
    uVar5 = 0;
  }
  else {
    uVar5 = (*DAT_008873e8)(0x5c,0x401be);
    iVar2 = (int)uVar5;
    *(code **)(iVar2 + 0x14) = FUN_003de280;
    *(code **)(iVar2 + 0x18) = FUN_003de2d0;
    *(code **)(iVar2 + 0x1c) = FUN_003de2c0;
    *(code **)(iVar2 + 0x28) = FUN_003de2f0;
    *(code **)(iVar2 + 0x2c) = FUN_003de4c0;
    *(code **)(iVar2 + 0x30) = FUN_003de4d0;
    *(code **)(iVar2 + 0x34) = FUN_003de6a0;
    *(code **)(iVar2 + 0x38) = FUN_003de740;
    *(code **)(iVar2 + 0x3c) = FUN_003de8c0;
    *(code **)(iVar2 + 0x40) = FUN_003de9b0;
    *(code **)(iVar2 + 0x44) = FUN_003dd590;
    *(code **)(iVar2 + 0x48) = FUN_003de9c0;
    *(code **)(iVar2 + 0x4c) = FUN_003dea10;
    uVar3 = (*DAT_008873f4)(param_1,0x70,0x401be);
    *(unsigned int *)(iVar2 + 0x58) = uVar3;
    iVar6 = *(int *)(iVar2 + 0x58);
    iVar9 = 0;
    if (0 < param_1) {
      if (8 < param_1) {
        bVar1 = 0;
        if ((-1 < param_1) && (param_1 < 0x7fffffff)) {
          bVar1 = 1;
        }
        if (bVar1) {
          iVar7 = 0;
          iVar8 = iVar6;
          do {
            *(int *)(iVar8 + 0x50) = iVar2;
            *(int *)(iVar8 + 0x68) = param_2 + iVar7;
            iVar9 = iVar9 + 8;
            *(int *)(iVar8 + 0x6c) = param_3;
            *(int *)(iVar8 + 0xc0) = iVar2;
            *(int *)(iVar8 + 0xd8) = param_2 + iVar7 + param_3;
            *(int *)(iVar8 + 0xdc) = param_3;
            *(int *)(iVar8 + 0x130) = iVar2;
            *(int *)(iVar8 + 0x148) = param_2 + iVar7 + param_3 * 2;
            *(int *)(iVar8 + 0x14c) = param_3;
            *(int *)(iVar8 + 0x1a0) = iVar2;
            *(int *)(iVar8 + 0x1b8) = param_2 + iVar7 + param_3 * 3;
            *(int *)(iVar8 + 0x1bc) = param_3;
            *(int *)(iVar8 + 0x210) = iVar2;
            iVar10 = iVar7 + param_3 * 6;
            *(int *)(iVar8 + 0x228) = param_2 + iVar7 + param_3 * 4;
            *(int *)(iVar8 + 0x22c) = param_3;
            iVar11 = iVar7 + param_3 * 7;
            *(int *)(iVar8 + 0x280) = iVar2;
            *(int *)(iVar8 + 0x298) = param_2 + iVar7 + param_3 * 5;
            *(int *)(iVar8 + 0x29c) = param_3;
            iVar7 = iVar7 + param_3 * 8;
            *(int *)(iVar8 + 0x2f0) = iVar2;
            *(int *)(iVar8 + 0x308) = param_2 + iVar10;
            *(int *)(iVar8 + 0x30c) = param_3;
            *(int *)(iVar8 + 0x360) = iVar2;
            *(int *)(iVar8 + 0x378) = param_2 + iVar11;
            *(int *)(iVar8 + 0x37c) = param_3;
            iVar8 = iVar8 + 0x380;
          } while (iVar9 < param_1 - 8);
        }
      }
      if (iVar9 < param_1) {
        iVar6 = iVar6 + iVar9 * 0x70;
        iVar8 = iVar9 * param_3;
        do {
          *(int *)(iVar6 + 0x50) = iVar2;
          *(int *)(iVar6 + 0x68) = param_2 + iVar8;
          iVar9 = iVar9 + 1;
          *(int *)(iVar6 + 0x6c) = param_3;
          iVar8 = iVar8 + param_3;
          iVar6 = iVar6 + 0x70;
        } while (iVar9 < param_1);
      }
    }
    lVar4 = FUN_003dd290(uVar5,param_1,param_5,param_4);
    if (lVar4 != 1) {
      (*DAT_008873ec)(*(unsigned int *)(iVar2 + 0x58));
      (*DAT_008873ec)(uVar5);
      uVar5 = 0;
    }
  }
  return uVar5;
}
