/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004b55a0 func_004b55a0 src/Graphics/Model/mdlEffect_inferred_004b5340.c - romwright draft, unreviewed */

void FUN_004b55a0(unsigned long long param_1)

{
  code *pcVar1;
  long lVar2;
  int iVar3;
  
  lVar2 = (*DAT_00764c94)();
  if (lVar2 != 0) {
    iVar3 = (int)param_1;
    pcVar1 = *(code **)(DAT_00764c90 + 0x10 + *(int *)(iVar3 + 0x2c) * 0x1c);
    if (pcVar1 != (code *)0x0) {
      (*pcVar1)(param_1);
    }
    *(int *)(iVar3 + 0x28) = *(int *)(iVar3 + 0x28) + 1;
  }
  return;
}
