/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004bf138 ACSSND_SetLpFlg src/cri/acssnd.c - romwright draft, unreviewed */

void FUN_004bf138(int param_1,unsigned long long param_2)

{
  int *piVar1;
  unsigned int *puVar2;
  int iVar3;
  
  if (*(int *)(param_1 + 0x50) == 0) {
    FUN_004bd6a0(0x756fb0);
    return;
  }
  iVar3 = 0;
  if (0 < *(int *)(param_1 + 0xa0)) {
    puVar2 = (unsigned int *)(param_1 + 0x18);
    piVar1 = (int *)*puVar2;
    while( 1 ) {
      puVar2 = puVar2 + 3;
      iVar3 = iVar3 + 1;
      (**(code **)(*piVar1 + 0x38))(piVar1,param_2);
      if (*(int *)(param_1 + 0xa0) <= iVar3) break;
      piVar1 = (int *)*puVar2;
    }
  }
  return;
}
