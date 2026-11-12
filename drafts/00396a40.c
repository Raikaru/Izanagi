/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00396a40 func_00396a40 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_00396a40(unsigned long long param_1,unsigned long long param_2,int param_3)

{
  unsigned int *puVar1;
  long lVar2;
  unsigned int uVar3;
  int iVar4;
  int uStack_8;
  int uStack_4;
  
  uStack_4 = 0x100;
  lVar2 = FUN_003df240(param_1,&uStack_4,4);
  if (lVar2 == 0) {
    param_1 = 0;
  }
  else {
    param_3 = param_3 + DAT_007646c8;
    lVar2 = FUN_003df240(param_1,param_3,4);
    if (lVar2 != 0) {
      puVar1 = *(unsigned int **)(param_3 + 4);
      if ((puVar1 != (unsigned int *)0x0) && ((*puVar1 & 1) == 0)) {
        lVar2 = FUN_003df240(param_1,puVar1 + 1,4);
        if (lVar2 == 0) {
          param_1 = 0;
        }
        else {
          lVar2 = FUN_003df240(param_1,puVar1,4);
          if (lVar2 == 0) {
            param_1 = 0;
          }
          else {
            lVar2 = FUN_003df240(param_1,puVar1[8] + 0x20,4);
            if (lVar2 != 0) {
              uVar3 = puVar1[4];
              iVar4 = 0;
              if (0 < (int)puVar1[1]) {
                do {
                  lVar2 = FUN_003df240(param_1,uVar3,4);
                  if (lVar2 == 0) {
                    return 0;
                  }
                  lVar2 = FUN_003df240(param_1,uVar3 + 4,4);
                  if (lVar2 == 0) {
                    return 0;
                  }
                  lVar2 = FUN_003df240(param_1,uVar3 + 8,4);
                  if (lVar2 == 0) {
                    return 0;
                  }
                  iVar4 = iVar4 + 1;
                  uVar3 = uVar3 + 0x10;
                } while (iVar4 < (int)puVar1[1]);
              }
            }
            else {
              param_1 = 0;
            }
          }
        }
      }
      else {
        uStack_8 = 0;
        lVar2 = FUN_003df240(param_1,&uStack_8,4);
        if (lVar2 == 0) {
          param_1 = 0;
        }
      }
    }
    else {
      param_1 = 0;
    }
  }
  return param_1;
}
