/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f98a8 func_004f98a8 src/promoted/code1_004f.c - romwright draft, unreviewed */

void FUN_004f98a8(char *param_1,char *param_2,char *param_3)

{
  *param_3 = param_1[3] + *param_2;
  param_3[1] = param_1[7];
  param_3[2] = param_1[2] ^ param_2[1];
  param_3[3] = param_1[6];
  param_3[4] = param_1[1];
  param_3[5] = param_1[5] ^ param_2[2];
  param_3[6] = *param_1;
  param_3[7] = param_1[4] + param_2[3];
  return;
}
