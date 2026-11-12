/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004275a8 func_004275a8 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_004275a8(unsigned long long param_1,unsigned long long param_2,int param_3)

{
  unsigned int uVar1;
  int iVar2;
  unsigned char *puVar3;
  long lVar4;
  unsigned long long uVar5;
  int *piVar6;
  int iVar7;
  unsigned int *puVar8;
  unsigned int uVar9;
  int iVar10;
  unsigned int uVar11;
  unsigned char auStack_d0 [4];
  unsigned int uStack_cc;
  unsigned int uStack_c8;
  unsigned char *puStack_bc;
  unsigned int auStack_b0 [4];
  
  lVar4 = FUN_00426248();
  FUN_004266c8(3);
  if ((DAT_0070c67c != 0) && (lVar4 != 0)) {
    puVar8 = (unsigned int *)lVar4;
    uVar1 = puVar8[1];
    if (uVar1 != 0) {
      DAT_0088f78c = *puVar8;
      DAT_0088f7ac = (int)(puVar8 + -0x224320) >> 4;
      uStack_cc = 1;
      puStack_bc = &LAB_00754200;
      uVar9 = uVar1 & 0x8000;
      uVar11 = (unsigned int)param_2;
      uStack_c8 = 0;
      DAT_0088f790._0_4_ = uVar11;
      DAT_0088f794 = param_3;
      uVar5 = FUN_004217e0(auStack_d0);
      DAT_0088f784 = auStack_b0;
      DAT_0088f788 = 4;
      DAT_0088f780 = (int)uVar5;
      if (uVar9 != 0) {
        FUN_00421820(DAT_0070c68c);
        if (DAT_0070c5f8 == -1) {
          DAT_0070c5f8 = DAT_0088f780;
          DAT_0088f780 = -DAT_0088f780;
        }
        else {
          iVar10 = 1;
          do {
            if (0x1f < iVar10) goto LAB_00427704;
            piVar6 = &DAT_0070c5f8 + iVar10;
            iVar10 = iVar10 + 1;
          } while (*piVar6 != -1);
          *piVar6 = DAT_0088f780;
          DAT_0088f780 = -DAT_0088f780;
        }
LAB_00427704:
        FUN_00421800(DAT_0070c68c);
      }
      else {
        uVar9 = 0;
      }
      if ((param_2 & 0xf) == 0) {
        iVar10 = 0;
      }
      else {
        iVar10 = (uVar11 & 0xfffffff0) - (uVar11 - 0x10);
      }
      if (param_3 < iVar10) {
        iVar10 = param_3;
      }
      if ((uVar1 & 0x20000000) == 0) {
        FUN_00424e78(param_2,param_3);
      }
      iVar7 = 0;
      DAT_0088f798._0_4_ = iVar10;
      if (0 < iVar10) {
        do {
          puVar3 = (unsigned char *)((uVar11 | 0x20000000) + iVar7);
          iVar2 = iVar7 + 4;
          iVar7 = iVar7 + 1;
          *(unsigned char *)((int)&DAT_0088f798 + iVar2) = *puVar3;
        } while (iVar7 < iVar10);
      }
      lVar4 = FUN_00426158(0x890e80,3,0,0x88f780,0x30,0x8903c0,4,0);
      iVar10 = iRam208903c0;
      if (lVar4 < 0) {
        FUN_004217f0(uVar5);
        FUN_004266f8();
        return 0xfffffff5;
      }
      FUN_004266f8();
      if (iVar10 == 0) {
        FUN_004217f0(uVar5);
        return 0xfffffff5;
      }
      if (uVar9 != 0) {
        FUN_004217f0(uVar5);
        return 0;
      }
      FUN_00421820(uVar5);
      FUN_004217f0(uVar5);
      return auStack_b0[0];
    }
  }
  FUN_004266f8();
  return 0xfffffff7;
}
