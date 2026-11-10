/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00427338 func_00427338 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_00427338(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3)

{
  unsigned int uVar1;
  long lVar2;
  unsigned long long uVar3;
  int *piVar4;
  int iVar5;
  unsigned int *puVar6;
  unsigned int uVar7;
  unsigned char auStack_d0 [4];
  unsigned int uStack_cc;
  unsigned int uStack_c8;
  unsigned char *puStack_bc;
  unsigned int auStack_b0 [4];
  
  lVar2 = FUN_00426248();
  FUN_004266c8(2);
  if ((DAT_0070c67c != 0) && (lVar2 != 0)) {
    puVar6 = (unsigned int *)lVar2;
    uVar1 = puVar6[1];
    if (uVar1 != 0) {
      DAT_0088f78c = *puVar6;
      DAT_0088f79c = (int)(puVar6 + -0x224320) >> 4;
      uStack_cc = 1;
      puStack_bc = &LAB_007541e8;
      uVar7 = uVar1 & 0x8000;
      DAT_0088f790._0_4_ = (unsigned int)param_2;
      DAT_0088f790._4_4_ = (unsigned int)param_3;
      uStack_c8 = 0;
      uVar3 = FUN_004217e0(auStack_d0);
      DAT_0088f784 = auStack_b0;
      DAT_0088f788 = 4;
      DAT_0088f780 = (int)uVar3;
      if (uVar7 != 0) {
        FUN_00421820(DAT_0070c68c);
        if (DAT_0070c5f8 == -1) {
          DAT_0070c5f8 = DAT_0088f780;
          DAT_0088f780 = -DAT_0088f780;
        }
        else {
          iVar5 = 1;
          do {
            if (0x1f < iVar5) goto LAB_00427494;
            piVar4 = &DAT_0070c5f8 + iVar5;
            iVar5 = iVar5 + 1;
          } while (*piVar4 != -1);
          *piVar4 = DAT_0088f780;
          DAT_0088f780 = -DAT_0088f780;
        }
LAB_00427494:
        FUN_00421800(DAT_0070c68c);
      }
      else {
        uVar7 = 0;
      }
      if ((uVar1 & 0x20000000) == 0) {
        FUN_00424e78(param_2,param_3);
      }
      FUN_00424e78(0x88f780,0x20);
      lVar2 = FUN_00426158(0x890e80,2,0,0x88f780,0x20,0x8903c0,4,0);
      iVar5 = iRam208903c0;
      if (lVar2 < 0) {
        FUN_004217f0(uVar3);
        FUN_004266f8();
        return 0xfffffff5;
      }
      FUN_004266f8();
      if (iVar5 == 0) {
        FUN_004217f0(uVar3);
        return 0xfffffff5;
      }
      if (uVar7 != 0) {
        FUN_004217f0(uVar3);
        return 0;
      }
      FUN_00421820(uVar3);
      FUN_004217f0(uVar3);
      return auStack_b0[0];
    }
  }
  FUN_004266f8();
  return 0xfffffff7;
}
