/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00428f08 func_00428f08 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_00428f08(char *param_1,unsigned int param_2)

{
  char cVar1;
  unsigned long long uVar2;
  long lVar3;
  int iVar4;
  unsigned char auStack_b0 [4];
  unsigned int uStack_ac;
  unsigned int uStack_a8;
  unsigned char *puStack_9c;
  unsigned int auStack_90 [4];
  
  FUN_004266c8(0x13);
  if (DAT_0070c67c != 0) {
    DAT_0088f794._0_1_ = *param_1;
  }
  else {
    FUN_004267f0();
    DAT_0088f794._0_1_ = *param_1;
  }
  iVar4 = 0;
  if ((char)DAT_0088f794 != '\0') {
    for (iVar4 = 1;
        (iVar4 < 0x400 &&
        (cVar1 = param_1[iVar4], *(char *)((int)&DAT_0088f794 + iVar4) = cVar1, cVar1 != '\0'));
        iVar4 = iVar4 + 1) {
    }
  }
  if (iVar4 == 0x400) {
    DAT_0088fb93 = 0;
  }
  uStack_ac = 1;
  puStack_9c = &LAB_00754320;
  uStack_a8 = 0;
  DAT_0088f790._0_4_ = param_2;
  uVar2 = FUN_004217e0(auStack_b0);
  DAT_0088f780 = (unsigned int)uVar2;
  DAT_0088f788 = 4;
  DAT_0088f784 = auStack_90;
  lVar3 = FUN_00426158(0x890e80,0x13,0,0x88f780,0x414,0x8903c0,4,0);
  iVar4 = iRam208903c0;
  if (lVar3 < 0) {
    FUN_004217f0(uVar2);
    FUN_004266f8();
    auStack_90[0] = 0xfffffff5;
  }
  else {
    FUN_004266f8();
    if (iVar4 == 0) {
      FUN_004217f0(uVar2);
      auStack_90[0] = 0xfffffff5;
    }
    else {
      FUN_00421820(uVar2);
      FUN_004217f0(uVar2);
    }
  }
  return auStack_90[0];
}
