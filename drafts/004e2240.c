/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e2240 func_004e2240 src/cri/cri_cvfs.c - romwright draft, unreviewed */

void FUN_004e2240(long param_1)

{
  code *pcVar1;
  
  if (param_1 == 0) {
    FUN_004e15a8(0x75a070);
    return;
  }
  pcVar1 = *(code **)(*(int *)param_1 + 0x28);
  if (pcVar1 == (code *)0x0) {
    FUN_004e15a8(0x75a090);
    return;
  }
  (*pcVar1)(((int *)param_1)[1]);
  return;
}
