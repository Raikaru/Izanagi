/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00426cf0 func_00426cf0 src/promoted/code1_0042.c - romwright draft, unreviewed */

int FUN_00426cf0(char *param_1,unsigned int param_2,unsigned int param_3)

{
  char cVar1;
  unsigned int uVar2;
  int iVar3;
  long lVar4;
  unsigned long long uVar5;
  int iVar6;
  int *piVar7;
  unsigned char auStack_140 [4];
  unsigned int uStack_13c;
  unsigned int uStack_138;
  unsigned char *puStack_12c;
  int aiStack_120 [4];
  
  FUN_004266c8(0);
  if (DAT_0070c67c == 0) {
    FUN_004267f0();
  }
  lVar4 = FUN_004269f8();
  if (lVar4 != 0) {
    FUN_004266f8();
    iVar6 = -0x10004;
  }
  else {
    lVar4 = FUN_004260d0();
    if (lVar4 != 0) {
      DAT_0088f794._0_1_ = *param_1;
      iVar6 = 0;
      if ((char)DAT_0088f794 != '\0') {
        for (iVar6 = 1;
            (iVar6 < 0x400 &&
            (cVar1 = param_1[iVar6], *(char *)((int)&DAT_0088f794 + iVar6) = cVar1, cVar1 != '\0'));
            iVar6 = iVar6 + 1) {
        }
      }
      if (iVar6 == 0x400) {
        DAT_0088fb93 = 0;
      }
      piVar7 = (int *)lVar4;
      iVar6 = (int)(piVar7 + -0x224320) >> 4;
      DAT_0088f78c = param_2 & 0x7fffffff;
      uStack_13c = 1;
      puStack_12c = &LAB_007541a0;
      uStack_138 = 0;
      DAT_0088f790._0_4_ = param_3;
      DAT_0088fb94 = iVar6;
      uVar5 = FUN_004217e0(auStack_140);
      DAT_0088f780 = (unsigned int)uVar5;
      DAT_0088f788 = 4;
      DAT_0088f784 = aiStack_120;
      lVar4 = FUN_00426158(0x890e80,0,0,0x88f780,0x418,0x8903c0,4,0);
      iVar3 = iRam208903c0;
      if (lVar4 < 0) {
        FUN_004217f0(uVar5);
        FUN_004266f8();
        iVar6 = -0xb;
      }
      else {
        FUN_004266f8();
        if (iVar3 == 0) {
          FUN_004217f0(uVar5);
          iVar6 = -0xb;
        }
        else {
          FUN_00421820(uVar5);
          FUN_004217f0(uVar5);
          if (aiStack_120[0] < 0) {
            FUN_00421820(DAT_0070c688);
            piVar7[1] = 0;
            FUN_00421800(DAT_0070c688);
            iVar6 = aiStack_120[0];
          }
          else {
            FUN_00421820(DAT_0070c688);
            uVar2 = DAT_0070c688;
            piVar7[1] = piVar7[1] | param_2;
            *piVar7 = aiStack_120[0];
            FUN_00421800(uVar2);
          }
        }
      }
    }
    else {
      FUN_004266f8();
      iVar6 = -0x13;
    }
  }
  return iVar6;
}
