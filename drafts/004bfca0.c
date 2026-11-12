/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bfca0 ACSSND_Start src/cri/acssnd.c - romwright draft, unreviewed */

void FUN_004bfca0(unsigned long long param_1,unsigned long long param_2)

{
  int *piVar1;
  unsigned char bVar2;
  long lVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  
  bVar2 = 0;
  iVar5 = (int)param_1;
  if (*(int *)(iVar5 + 0x50) == 0) {
    FUN_004bd6a0(0x7572c0);
    return;
  }
  FUN_004bd658();
  FUN_004c01c0(param_1,1);
  iVar4 = *(int *)(iVar5 + 0xb4);
  if (iVar4 < 0) {
    iVar7 = -iVar4;
    iVar4 = 0;
  }
  else {
    iVar7 = 0;
  }
  if (*(int *)(iVar5 + 0x5c) != 0) {
    iVar6 = iVar5 + 8;
    FUN_004c0308(param_1);
    (**(code **)(**(int **)(iVar5 + 8) + 0x1c))(*(int **)(iVar5 + 8),1);
    *(unsigned int *)(iVar5 + 100) = 1;
    lVar3 = (**(code **)(**(int **)(*(int *)(iVar5 + 0x14) * 0xc + iVar6 + 0x10) + 0x24))();
    if (lVar3 == 0) {
      FUN_004c0068(param_1,iVar5 + *(int *)(iVar5 + 0x10) * 0xc + 0x18,0,0);
      iVar4 = *(int *)(iVar5 + 0x14);
    }
    else {
      FUN_004c0068(param_1,iVar5 + *(int *)(iVar5 + 0x10) * 0xc + 0x18,iVar4,0);
      iVar4 = *(int *)(iVar5 + 0x14);
    }
    lVar3 = (**(code **)(**(int **)(iVar4 * 0xc + iVar6 + 0x10) + 0x24))();
    if (lVar3 != 0) {
      FUN_004c0118(param_1,iVar5 + *(int *)(iVar5 + 0x14) * 0xc + 0x18,iVar7,2);
      if (*(int *)(iVar5 + 0x60) != 1) {
        bVar2 = 1;
        FUN_004bd670();
        if (bVar2) {
          FUN_004c0518(param_1,iVar5 + *(int *)(iVar5 + 0x10) * 0xc + 0x18);
          FUN_004bff98(param_1,param_2);
        }
        return;
      }
      (**(code **)(**(int **)(*(int *)(iVar5 + 0x14) * 0xc + iVar6 + 0x10) + 0x18))();
      FUN_004bd760(*(unsigned int *)(*(int *)(iVar5 + 0x14) * 0xc + iVar5 + 0x20),0xfffffffffffffc40);
    }
    bVar2 = 1;
  }
  else {
    lVar3 = (**(code **)(**(int **)(iVar5 + 8) + 0x24))();
    if (lVar3 != 2) {
      piVar1 = *(int **)(iVar5 + 8);
    }
    else {
      if (*(int *)(iVar5 + 0x60) != 1) {
        FUN_004c0118(param_1,iVar5 + *(int *)(iVar5 + 0x10) * 0xc + 0x18,0,1);
        FUN_004bd670();
        if (bVar2) {
          FUN_004c0518(param_1,iVar5 + *(int *)(iVar5 + 0x10) * 0xc + 0x18);
          FUN_004bff98(param_1,param_2);
        }
        return;
      }
      piVar1 = *(int **)(iVar5 + 8);
    }
    bVar2 = 1;
    (**(code **)(*piVar1 + 0x1c))(piVar1,1);
    *(unsigned int *)(iVar5 + 100) = 1;
    FUN_004c0068(param_1,iVar5 + *(int *)(iVar5 + 0x10) * 0xc + 0x18,0,0);
  }
  FUN_004bd670();
  if (bVar2) {
    FUN_004c0518(param_1,iVar5 + *(int *)(iVar5 + 0x10) * 0xc + 0x18);
    FUN_004bff98(param_1,param_2);
  }
  return;
}
