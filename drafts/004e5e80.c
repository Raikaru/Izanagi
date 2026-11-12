/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e5e80 dvCiOpen src/cri/dvci.c - romwright draft, unreviewed */

long FUN_004e5e80(long param_1,unsigned long long param_2,long param_3)

{
  long lVar1;
  long lVar2;
  long lVar3;
  int iVar4;
  unsigned long long uVar5;
  unsigned char *puVar6;
  
  if (param_1 == 0) {
    uVar5 = 0x75b0f0;
  }
  else if (param_3 != 0) {
    uVar5 = 0x75b118;
  }
  else {
    lVar1 = FUN_004e5e28();
    puVar6 = (unsigned char *)lVar1;
    if (lVar1 != 0) {
      lVar2 = FUN_004e6eb0(param_1,puVar6 + 0x20);
      iVar4 = *(int *)(puVar6 + 0x24);
      if (iVar4 == 0) {
        FUN_004e6640(param_1,0x733c80);
        FUN_004cd3f0(0x75b170,0x733c80);
        FUN_004e4618(0x8300);
        FUN_0042e7c8(0);
        FUN_004e4618(0x8301);
        lVar3 = FUN_004e5a68(puVar6 + 0x20,0x733c80);
        if (lVar3 == 0) {
          FUN_004e5a38(0,0x75b198);
          FUN_004e5e60(lVar1);
          return 0;
        }
        puVar6[1] = 0;
        iVar4 = *(int *)(puVar6 + 0x24);
      }
      else {
        puVar6[1] = 1;
      }
      *(unsigned int *)(puVar6 + 8) = (iVar4 + 0x7ffU) >> 0xb;
      *puVar6 = 1;
      *(int *)(puVar6 + 4) = iVar4;
      *(unsigned int *)(puVar6 + 0xc) = 0;
      *(unsigned int *)(puVar6 + 0x18) = 0;
      *(unsigned int *)(puVar6 + 0x10) = 0;
      *(unsigned int *)(puVar6 + 0x14) = 0;
      puVar6[2] = 0;
      *(unsigned int *)(puVar6 + 0x1c) = 0;
      if (lVar2 != 1) {
        return lVar1;
      }
      *(unsigned int *)(puVar6 + 8) = 0x7fffffff;
      return lVar1;
    }
    uVar5 = 0x75b140;
  }
  FUN_004e5a38(0,uVar5);
  return 0;
}
