/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039af40 func_0039af40 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_0039af40(unsigned long long param_1,unsigned long long param_2,int param_3)

{
  int iVar1;
  long lVar2;
  unsigned int *puVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  int uStack_20;
  int iStack_1c;
  int iStack_18;
  unsigned int uStack_14;
  int iStack_10;
  int iStack_4;
  
  piVar4 = (int *)(param_3 + DAT_007646e0);
  iStack_4 = *piVar4;
  lVar2 = FUN_003df240(param_1,&iStack_4,4);
  if (lVar2 != 0) {
    iVar6 = 0;
    if (0 < iStack_4) {
      iVar5 = 0;
      do {
        iVar1 = piVar4[2];
        puVar3 = (unsigned int *)(iVar1 + iVar5);
        uStack_20 = *puVar3;
        iStack_1c = (int)*(short *)(puVar3 + 1);
        iStack_18 = (int)*(short *)((int)puVar3 + 6);
        uStack_14 = puVar3[2];
        iStack_10 = (puVar3[4] - iVar1) / 0x14;
        lVar2 = FUN_003e2ab0(param_1,&uStack_20,0x14);
        if (lVar2 == 0) {
          return 0;
        }
        iVar6 = iVar6 + 1;
        iVar5 = iVar5 + 0x14;
      } while (iVar6 < iStack_4);
    }
  }
  else {
    param_1 = 0;
  }
  return param_1;
}
