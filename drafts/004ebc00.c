/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004ebc00 AXRNA_SetOutPan src/cri/ps2_rna.c - romwright draft, unreviewed */

void FUN_004ebc00(void)

{
  long lVar1;
  int iStack_60;
  unsigned int uStack_5c;
  int iStack_58;
  
  if ((DAT_0073a748 != 1) || (0 < DAT_0073a740)) {
    if (DAT_0073a78c == 0) {
      FUN_004eff60(DAT_0073a748,DAT_00739798);
      FUN_004f01b8();
      FUN_004e5738();
      FUN_004ebf28();
      FUN_004eb3d8();
      FUN_0043f9c8(0x73d1e8,0,0x8d0);
      DAT_0073d1e4 = 0x880;
      DAT_0073a790 = &LAB_0073d200;
      if (DAT_0073a794 == 0) {
        lVar1 = FUN_00429ca0(0x8d0);
        DAT_0073a794 = (int)lVar1;
        if (lVar1 == 0) {
          FUN_004cd3f0(0x75c8d8,0x8d0);
          do {
                    /* WARNING: Do nothing block with infinite loop */
          } while( 1 );
        }
      }
      FUN_004cd3f0(0x75c7a0,0x8d0,DAT_0073a794);
      DAT_0073a798 = (DAT_0073a794 + 0x40U) & 0xffffffc0;
      lVar1 = FUN_004e53a8(1,DAT_0073a790,DAT_0073a798,DAT_0073d1e4);
      DAT_0073d1e0 = (unsigned int)lVar1;
      if (lVar1 == 0) {
        FUN_004cd3f0(0x75c920);
        do {
                    /* WARNING: Do nothing block with infinite loop */
        } while( 1 );
      }
      FUN_004e55a8(DAT_0073d1e0,0x4eb830,0);
      FUN_004e55b8(DAT_0073d1e0,0x4eb890,0);
      if (DAT_0073a75c == 0) {
        DAT_0073a764 = DAT_0073a740;
        DAT_0073a758 = DAT_0073a740 * 0x400 + 0x40;
        lVar1 = FUN_00429ca0(DAT_0073a758);
        DAT_0073a75c = (int)lVar1;
        if (lVar1 == 0) {
          FUN_004cd3f0(0x75c948,DAT_0073a758);
          do {
                    /* WARNING: Do nothing block with infinite loop */
          } while( 1 );
        }
        FUN_004cd3f0(0x75c7a0,DAT_0073a758,DAT_0073a75c);
      }
      DAT_0073a760 = (DAT_0073a75c + 0x3fU) & 0xffffffc0;
      iStack_60 = DAT_0073a764;
      iStack_58 = DAT_0073a758;
      uStack_5c = DAT_0073a760;
      FUN_004e58a0(10,&iStack_60,3,0,0);
    }
    DAT_0073a78c = DAT_0073a78c + 1;
  }
  return;
}
