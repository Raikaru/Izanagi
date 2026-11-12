/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00504b68 func_00504b68 src/cri/mpv_deli.c - romwright draft, unreviewed */

void FUN_00504b68(unsigned int *param_1,long param_2)

{
  param_1[2] = (int)param_2;
  if ((param_2 != 0) && ((code *)*param_1 != (code *)0x0)) {
    (*(code *)*param_1)(param_1[1]);
  }
  return;
}
