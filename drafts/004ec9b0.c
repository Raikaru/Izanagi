/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ec9b0 AXRNA_SetPlaySw src/cri/ps2_rna.c - romwright draft, unreviewed */

void FUN_004ec9b0(int param_1,unsigned char param_2)

{
  if (DAT_0073a748 != 1) {
    *(unsigned char *)(param_1 + 0x50) = param_2;
  }
  else if (0 < DAT_0073a740) {
    *(unsigned char *)(param_1 + 0x50) = param_2;
  }
  return;
}
