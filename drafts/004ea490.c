/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ea490 func_004ea490 src/cri/mfci.c - romwright draft, unreviewed */

void FUN_004ea490(long param_1)

{
  if (param_1 == 0) {
    FUN_004e9e68(0,0x75bfa8);
    return;
  }
  FUN_004e9e20();
  *(unsigned char *)((int)param_1 + 1) = 0;
  FUN_004e9e38();
  return;
}
