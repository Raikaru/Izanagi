/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00426f80 func_00426f80 src/promoted/code1_0042.c - romwright draft, unreviewed */

int FUN_00426f80(void)

{
  int iVar1;
  long lVar2;
  unsigned long long uVar3;
  unsigned int *puVar4;
  unsigned char auStack_90 [4];
  unsigned int uStack_8c;
  unsigned int uStack_88;
  unsigned char *puStack_7c;
  int aiStack_70 [4];
  
  lVar2 = FUN_00426248();
  FUN_004266c8(1);
  if (((DAT_0070c67c == 0) || (lVar2 == 0)) || (puVar4 = (unsigned int *)lVar2, puVar4[1] == 0)) {
    FUN_004266f8();
    aiStack_70[0] = -9;
  }
  else {
    DAT_0088f78c = *puVar4;
    DAT_0088f790._0_4_ = (int)(puVar4 + -0x224320) >> 4;
    uStack_8c = 1;
    puStack_7c = &LAB_007541b8;
    uStack_88 = 0;
    uVar3 = FUN_004217e0(auStack_90);
    DAT_0088f784 = aiStack_70;
    DAT_0088f780 = (unsigned int)uVar3;
    DAT_0088f788 = 4;
    lVar2 = FUN_00426158(0x890e80,1,0,0x88f780,0x14,0x8903c0,4,0);
    if (lVar2 >= 0) {
      puVar4[1] = 0;
      iVar1 = iRam208903c0;
      FUN_004266f8();
      if (iVar1 == 0) {
        FUN_004217f0(uVar3);
        aiStack_70[0] = -0xb;
      }
      else {
        FUN_00421820(uVar3);
        FUN_004217f0(uVar3);
        if (-1 < aiStack_70[0]) {
          aiStack_70[0] = 0;
        }
      }
    }
    else {
      FUN_004217f0(uVar3);
      FUN_004266f8();
      aiStack_70[0] = -0xb;
    }
  }
  return aiStack_70[0];
}
