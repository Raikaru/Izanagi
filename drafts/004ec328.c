/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ec328 AXRNA_SetOutVol src/cri/ps2_rna.c - romwright draft, unreviewed */

void FUN_004ec328(void)

{
  if (((DAT_0073a748 != 1) || (0 < DAT_0073a740)) &&
     (DAT_0073a78c = DAT_0073a78c - 1, DAT_0073a78c == 0)) {
    FUN_00429e08(DAT_0073a75c);
    DAT_0073a75c = 0;
    FUN_004e5540(DAT_0073d1e0);
    DAT_0073d1e0 = 0;
    FUN_00429e08(DAT_0073a794);
    DAT_0073a794 = 0;
    FUN_004eb6d0();
    FUN_004ec008();
    FUN_004e5828();
    FUN_004f0350();
    FUN_004effb0();
  }
  return;
}
