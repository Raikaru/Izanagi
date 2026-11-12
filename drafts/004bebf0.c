/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bebf0 ACSSND_Stop src/cri/acssnd.c - romwright draft, unreviewed */

void FUN_004bebf0(unsigned long long param_1)

{
  int iVar1;
  
  iVar1 = (int)param_1;
  if (*(int *)(iVar1 + 0x50) != 0) {
    FUN_004bd658();
    FUN_004c01c0(param_1,1);
    FUN_004c0118(param_1,iVar1 + *(int *)(iVar1 + 0x10) * 0xc + 0x18,0,2);
    if (*(int *)(iVar1 + 0x60) == 1) {
      (**(code **)(**(int **)(*(int *)(iVar1 + 0x10) * 0xc + iVar1 + 8 + 0x10) + 0x18))();
      FUN_004bd760(*(unsigned int *)(*(int *)(iVar1 + 0x10) * 0xc + iVar1 + 0x10 + 0x10),
                   0xfffffffffffffc40);
      if (*(int *)(iVar1 + 0x5c) != 0) {
        (**(code **)(**(int **)(*(int *)(iVar1 + 0x14) * 0xc + iVar1 + 8 + 0x10) + 0x18))();
        FUN_004bd760(*(unsigned int *)(*(int *)(iVar1 + 0x14) * 0xc + iVar1 + 0x10 + 0x10),
                     0xfffffffffffffc40);
      }
    }
    FUN_004bd670();
    return;
  }
  FUN_004bd6a0(0x756e50);
  return;
}
