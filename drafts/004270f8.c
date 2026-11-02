/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004270f8 func_004270f8 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_004270f8(unsigned long long param_1,unsigned int param_2,unsigned int param_3)

{
  long lVar1;
  unsigned long long uVar2;
  int *piVar3;
  int iVar4;
  unsigned int *puVar5;
  unsigned int uVar6;
  unsigned char auStack_b0 [4];
  unsigned int uStack_ac;
  unsigned int uStack_a8;
  unsigned char *puStack_9c;
  unsigned int auStack_90 [4];
  
  lVar1 = FUN_00426248();
  FUN_004266c8(4);
  if ((DAT_0070c67c != 0) && (lVar1 != 0)) {
    puVar5 = (unsigned int *)lVar1;
    if (puVar5[1] != 0) {
      DAT_0088f78c = *puVar5;
      DAT_0088f798._0_4_ = (int)(puVar5 + -0x224320) >> 4;
      uStack_ac = 1;
      puStack_9c = &LAB_007541d0;
      uVar6 = puVar5[1] & 0x8000;
      uStack_a8 = 0;
      DAT_0088f790._0_4_ = param_2;
      DAT_0088f790._4_4_ = param_3;
      uVar2 = FUN_004217e0(auStack_b0);
      DAT_0088f784 = auStack_90;
      DAT_0088f788 = 4;
      DAT_0088f780 = (int)uVar2;
      if (uVar6 != 0) {
        FUN_00421820(DAT_0070c68c);
        if (DAT_0070c5f8 == -1) {
          DAT_0070c5f8 = DAT_0088f780;
          DAT_0088f780 = -DAT_0088f780;
        }
        else {
          iVar4 = 1;
          do {
            if (0x1f < iVar4) goto LAB_00427250;
            piVar3 = &DAT_0070c5f8 + iVar4;
            iVar4 = iVar4 + 1;
          } while (*piVar3 != -1);
          *piVar3 = DAT_0088f780;
          DAT_0088f780 = -DAT_0088f780;
        }
LAB_00427250:
        FUN_00421800(DAT_0070c68c);
      }
      else {
        uVar6 = 0;
      }
      lVar1 = FUN_00426158(0x890e80,4,0,0x88f780,0x1c,0x8903c0,4,0);
      iVar4 = iRam208903c0;
      if (lVar1 < 0) {
        FUN_004217f0(uVar2);
        FUN_004266f8();
        return 0xfffffff5;
      }
      FUN_004266f8();
      if (iVar4 == 0) {
        FUN_004217f0(uVar2);
        return 0xfffffff5;
      }
      if (uVar6 != 0) {
        FUN_004217f0(uVar2);
        return 0;
      }
      FUN_00421820(uVar2);
      FUN_004217f0(uVar2);
      return auStack_90[0];
    }
  }
  FUN_004266f8();
  return 0xfffffff7;
}
