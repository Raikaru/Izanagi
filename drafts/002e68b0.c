/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x002e68b0 func_002e68b0 src/Yajima/y_list.c - romwright draft, unreviewed */

void FUN_002e68b0(char param_1)

{
  short sVar1;
  int iVar2;
  unsigned long long uVar3;
  long lVar4;
  int iVar5;
  int *piVar6;
  short sVar7;
  int iVar8;
  unsigned short auStack_10400 [256];
  short asStack_10200 [32504];
  unsigned short auStack_400 [256];
  short asStack_200 [256];
  
  piVar6 = &DAT_00882f70 + param_1;
  iVar8 = *piVar6;
  if (iVar8 != 0) {
    iVar2 = *(int *)(iVar8 + 0x38);
    for (sVar7 = 0; (long)sVar7 < (long)*(int *)(*(int *)(iVar8 + 0x38) + 8); sVar7 = sVar7 + 1) {
      asStack_200[sVar7] = sVar7;
    }
    FUN_00440bb8(asStack_200,*(unsigned short *)(*(int *)(iVar8 + 0x38) + 8),2,0x2e6630);
    sVar7 = 0;
    lVar4 = (long)*(int *)(*(int *)(*piVar6 + 0x38) + 8);
    if (0 < lVar4) {
      iVar8 = *(int *)((&DAT_00882f70)[param_1] + 0x38);
      for (; sVar7 < lVar4; sVar7 = sVar7 + 1) {
        sVar1 = asStack_200[sVar7];
        switch(*(unsigned int *)(iVar8 + 4)) {
        case 0:
        case 2:
        case 7:
        case 8:
          iVar5 = iVar8 + sVar1 * 0x30 + 0x14;
          break;
        case 1:
        case 5:
        case 6:
        case 10:
          iVar5 = iVar8 + sVar1 * 0x30 + 0xa4;
          break;
        default:
          iVar5 = iVar8 + sVar1 * 0x30 + 0x14;
        }
        auStack_400[sVar7] = *(unsigned short *)(iVar5 + 2);
      }
    }
    for (sVar7 = 0; (long)sVar7 < (long)*(int *)(*(int *)(*piVar6 + 0x38) + 8); sVar7 = sVar7 + 1) {
      iVar8 = iVar2 + sVar7 * 0x30 + 0x14;
      FUN_0043f9c8(iVar8,0,0x30);
      uVar3 = FUN_0010fcb0(auStack_400[sVar7]);
      FUN_0043f810(iVar8,uVar3,0x30);
    }
  }
  return;
}
