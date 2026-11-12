/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a3e90 func_003a3e90 src/promoted/code1_003a.c - romwright draft, unreviewed */

long FUN_003a3e90(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3,unsigned long long param_4,
                 unsigned long long param_5)

{
  unsigned char bVar1;
  int iVar2;
  long lVar3;
  long lVar4;
  unsigned long long uVar5;
  int iVar6;
  unsigned long long uVar7;
  int iVar8;
  
  iVar6 = (int)param_2;
  iVar2 = *(int *)(iVar6 + 0x4c);
  bVar1 = (param_5 & 4) != 4;
  uVar7 = param_5 & 0x10;
  if (!bVar1) {
    uVar7 = 0;
  }
  iVar8 = (int)param_1;
  if (uVar7 == 0) {
    if (*(int *)(iVar2 + 0xd8) == 0) {
      if (!(bVar1 && (param_4 & 0xffffffffcff7ff78) == 0)) {
        if (bVar1 && (param_4 & 0xffffffffcee17e10) == 0) {
          lVar3 = FUN_003a8340(param_1,param_3,param_4);
          if (lVar3 == 1) {
            FUN_003aa0b0(param_1);
            iVar6 = *(int *)(iVar8 + DAT_00764700);
            *(code **)(iVar6 + 0x24) = FUN_003a72a0;
            *(unsigned int *)(iVar6 + 0x38) = 0;
            FUN_003a3050(param_1,iVar2 + 400);
          }
        }
        else {
          FUN_003a3050(param_1,iVar2 + 400);
          lVar3 = FUN_003b4470(param_1,param_3,param_4);
          if (lVar3 != 0) {
            *(unsigned int *)(iVar8 + 0x6c) =
                 *(unsigned int *)(*(int *)((int)&DAT_008872e0 + DAT_00764704) + 0x14);
            *(code **)(*(int *)(iVar8 + DAT_00764700) + 0x20) = FUN_003b6cc0;
          }
        }
      }
      else {
        lVar3 = FUN_003a6fb0(param_1,param_3,param_4);
        if (lVar3 == 1) {
          iVar8 = *(int *)(iVar8 + DAT_00764700);
          *(code **)(iVar8 + 0x24) = FUN_003a5280;
          *(code **)(iVar8 + 0x28) = FUN_003a5740;
          if ((*(unsigned int *)(iVar6 + 0x58) & 4) == 4) {
            *(code **)(iVar8 + 0x34) = FUN_003a63b0;
          }
          else {
            *(code **)(iVar8 + 0x34) = FUN_003a6230;
          }
          if ((*(unsigned int *)(iVar6 + 0x58) & 0x80) != 0x80) {
            if ((*(unsigned int *)(iVar6 + 0x58) & 0x80000) == 0x80000) {
              *(code **)(iVar8 + 0x2c) = FUN_003a5dc0;
            }
            else {
              *(unsigned int *)(iVar8 + 0x2c) = 0;
            }
          }
          else {
            *(code **)(iVar8 + 0x2c) = FUN_003a5fe0;
          }
          if ((*(unsigned int *)(iVar6 + 0x58) & 2) == 2) {
            *(code **)(iVar8 + 0x30) = FUN_003a5b50;
          }
          else {
            *(code **)(iVar8 + 0x30) = FUN_003a5940;
          }
          *(unsigned int *)(iVar8 + 0x38) = 0;
          FUN_003a3050(param_1,iVar2 + 400);
        }
      }
    }
    else {
      lVar3 = FUN_003c2630(0,0,0);
      if (lVar3 != 0) {
        iVar6 = *(int *)((int)lVar3 + 0x5c);
        *(unsigned int *)(iVar6 + 4) = 0;
        *(unsigned int *)(iVar6 + 8) = 0;
        *(unsigned int *)(iVar6 + 0xc) = 0;
        *(unsigned int *)(iVar6 + 0x10) = 0x3f800000;
        lVar4 = FUN_003c22f0(lVar3);
        if (lVar4 == 0) {
          lVar3 = 0;
        }
        else {
          FUN_003c0210(param_1,lVar3,0);
          FUN_003c2a80(lVar3);
          uVar5 = FUN_003c4140();
          FUN_003c4a80((int)lVar3 + 0x20,uVar5);
          FUN_003c4220(uVar5);
          iVar6 = *(int *)(iVar8 + DAT_00764700);
          *(code **)(iVar6 + 0x1c) = FUN_003a8640;
          *(unsigned int *)(iVar6 + 0x24) = 0;
          *(unsigned int *)(iVar6 + 0x28) = 0;
          *(unsigned int *)(iVar6 + 0x2c) = 0;
          *(unsigned int *)(iVar6 + 0x30) = 0;
          *(unsigned int *)(iVar6 + 0x34) = 0;
          *(unsigned int *)(iVar6 + 0x38) = 0;
          *(unsigned int *)(iVar8 + 0x6c) = **(unsigned int **)((int)&DAT_008872e0 + DAT_00764704);
          if ((*(unsigned int *)(iVar6 + 0x44) & 8) == 0) {
            FUN_003a3050(param_1,iVar2 + 400);
          }
          lVar3 = 1;
        }
      }
      else {
        lVar3 = 0;
      }
    }
  }
  else {
    FUN_0043f810(*(int *)(iVar8 + DAT_00764700) + 0x14,0x7084e0,0x10);
    lVar3 = (*(code *)PTR_FUN_007084e4)(param_1,param_2,param_3,param_4,param_5);
  }
  return lVar3;
}
