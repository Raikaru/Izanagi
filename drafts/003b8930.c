/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b8930 func_003b8930 src/promoted/code1_003b.c - romwright draft, unreviewed */

/* WARNING: Type propagation algorithm not settling */

unsigned long long FUN_003b8930(unsigned long long param_1,int param_2)

{
  int iVar1;
  int iVar2;
  unsigned char *pbVar3;
  int iVar4;
  long lVar5;
  unsigned int auStack_8 [2];
  
  auStack_8[1] = 4;
  iVar1 = **(int **)(param_2 + DAT_00886498);
  iVar2 = (*(int **)(param_2 + DAT_00886498))[1];
  iVar4 = FUN_003b7450();
  lVar5 = FUN_003deff0(param_1,1,iVar1 * 0x40 + iVar2 + iVar4 + 0x18,0x37002,0x37);
  if (lVar5 == 0) {
    param_1 = 0;
  }
  else {
    lVar5 = FUN_003df240(param_1,auStack_8 + 1,4);
    if (lVar5 == 0) {
      param_1 = 0;
    }
    else {
      pbVar3 = *(unsigned char **)(param_2 + DAT_00886498);
      auStack_8[0] = (unsigned int)*pbVar3 |
                     ((*(unsigned int *)(pbVar3 + 0x10) & 0xff) << 0x10) |
                     ((*(unsigned int *)(pbVar3 + 4) & 0xff) << 8);
      lVar5 = FUN_003df240(param_1,auStack_8,4);
      if (lVar5 == 0) {
        param_1 = 0;
      }
      else {
        lVar5 = FUN_003e2ab0(param_1,*(unsigned int *)(pbVar3 + 8),*(unsigned int *)(pbVar3 + 4));
        if (lVar5 == 0) {
          param_1 = 0;
        }
        else {
          lVar5 = FUN_003e2ab0(param_1,*(unsigned int *)(pbVar3 + 0xc),*(int *)pbVar3 << 6);
          if (lVar5 == 0) {
            param_1 = 0;
          }
          else {
            lVar5 = FUN_003df240(param_1,pbVar3 + 0x1c,0x10);
            if (lVar5 == 0) {
              param_1 = 0;
            }
            else {
              lVar5 = FUN_003b71b0(param_1,pbVar3);
              if (lVar5 == 0) {
                param_1 = 0;
              }
            }
          }
        }
      }
    }
  }
  return param_1;
}
