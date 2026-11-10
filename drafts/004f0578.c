/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f0578 func_004f0578 src/cri/sjx.c - romwright draft, unreviewed */

void FUN_004f0578(int param_1)

{
  if (DAT_00925e68 != (code *)0x0) {
    DAT_00743980 = DAT_00743980 - 1;
    if (DAT_00743980 == 0) {
      if (DAT_00743984 != param_1) {
        FUN_004f0738(0x75d198,DAT_00743984);
      }
      DAT_00743984 = 0;
    }
    (*DAT_00925e68)(DAT_00925e6c);
  }
  return;
}
