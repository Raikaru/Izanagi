/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039b080 func_0039b080 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_0039b080(unsigned long long param_1,unsigned long long param_2,unsigned long long param_3)

{
  long lVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  unsigned int auStack_20 [3];
  float fStack_14;
  int iStack_10;
  int iStack_4;
  
  iVar2 = (int)param_3 + DAT_007646e0;
  lVar1 = FUN_003df360(param_1,&iStack_4,4);
  if (lVar1 != 0) {
    if (0 < iStack_4) {
      lVar1 = FUN_0039b2c0(param_3);
      if (lVar1 != 0) {
        iVar4 = 0;
        if (0 < iStack_4) {
          iVar3 = 0;
          do {
            lVar1 = FUN_003e2910(param_1,auStack_20,0x14);
            if (lVar1 != 0x14) {
              return 0;
            }
            iVar4 = iVar4 + 1;
            *(unsigned int *)(*(int *)(iVar2 + 8) + iVar3) = auStack_20[0];
            *(short *)(*(int *)(iVar2 + 8) + iVar3 + 4) = (short)auStack_20[1];
            *(short *)(*(int *)(iVar2 + 8) + iVar3 + 6) = (short)auStack_20[2];
            *(float *)(*(int *)(iVar2 + 8) + iVar3 + 8) = fStack_14;
            *(float *)(*(int *)(iVar2 + 8) + iVar3 + 0xc) = 1.0f / fStack_14;
            *(int *)(*(int *)(iVar2 + 8) + iVar3 + 0x10) = *(int *)(iVar2 + 8) + iStack_10 * 0x14;
            iVar3 = iVar3 + 0x14;
          } while (iVar4 < iStack_4);
        }
      }
      else {
        param_1 = 0;
      }
    }
  }
  else {
    param_1 = 0;
  }
  return param_1;
}
