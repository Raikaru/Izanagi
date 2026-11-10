/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f0518 func_004f0518 src/cri/sjx.c - romwright draft, unreviewed */

void FUN_004f0518(unsigned int param_1)

{
  if (DAT_00925e60 != (code *)0x0) {
    (*DAT_00925e60)(DAT_00925e64);
    if (DAT_00743980 == 0) {
      DAT_00743984 = param_1;
    }
    DAT_00743980 = DAT_00743980 + 1;
  }
  return;
}
