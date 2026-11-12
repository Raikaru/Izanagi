/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004b5620 func_004b5620 src/Graphics/Model/mdlEffect_inferred_004b5340.c - romwright draft, unreviewed */

void FUN_004b5620(unsigned long long param_1)

{
  code *pcVar1;
  long lVar2;
  
  lVar2 = (*DAT_00764c94)();
  if ((lVar2 != 0) &&
     (pcVar1 = *(code **)(DAT_00764c90 + 0x14 + *(int *)((int)param_1 + 0x2c) * 0x1c),
     pcVar1 != (code *)0x0)) {
    (*pcVar1)(param_1);
  }
  return;
}
