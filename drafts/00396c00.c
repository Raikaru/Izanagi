/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00396c00 func_00396c00 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_00396c00(unsigned long long param_1,unsigned long long param_2,unsigned int param_3)

{
  unsigned int uVar1;
  long lVar2;
  int iVar3;
  unsigned int *puVar4;
  int iVar5;
  int uStack_10;
  unsigned int uStack_c;
  int iStack_8;
  unsigned int uStack_4;
  
  iVar3 = param_3 + DAT_007646c8;
  lVar2 = FUN_003df360(param_1,&iStack_8,4);
  if (lVar2 != 0) {
    if (iStack_8 != 0x100) {
      param_1 = 0;
    }
    else {
      lVar2 = FUN_003df360(param_1,iVar3,4);
      if (lVar2 == 0) {
        param_1 = 0;
      }
      else {
        lVar2 = FUN_003df360(param_1,&uStack_4,4);
        if (lVar2 != 0) {
          if (0 < (int)uStack_4) {
            lVar2 = FUN_003df360(param_1,&uStack_c,4);
            if (lVar2 == 0) {
              param_1 = 0;
            }
            else {
              lVar2 = FUN_003df360(param_1,&uStack_10,4);
              if (lVar2 == 0) {
                param_1 = 0;
              }
              else {
                lVar2 = (*DAT_008873f8)(DAT_007646cc,0x3011e);
                FUN_0043f9c8(lVar2,0,0x24);
                puVar4 = (unsigned int *)lVar2;
                if (lVar2 != 0) {
                  uVar1 = FUN_003d5790(uStack_4,uStack_10);
                  puVar4[8] = uVar1;
                  *puVar4 = uStack_c;
                  puVar4[5] = param_3;
                  puVar4[1] = uStack_4;
                  puVar4[6] = (unsigned int)puVar4;
                  if ((*puVar4 & 2) != 0) {
                    puVar4[2] = 0;
                    puVar4[3] = 0;
                  }
                  else {
                    uVar1 = (*DAT_008873e8)(uStack_4 * 0x40 + 0xf,0x3011e);
                    puVar4[2] = (uVar1 + 0xf) & 0xfffffff0;
                    puVar4[3] = uVar1;
                  }
                  uVar1 = (*DAT_008873e8)(uStack_4 << 4,0x3011e);
                  puVar4[4] = uVar1;
                  uVar1 = puVar4[4];
                  iVar5 = 0;
                  if (0 < (int)puVar4[1]) {
                    do {
                      lVar2 = FUN_003df360(param_1,uVar1,4);
                      if (lVar2 == 0) {
                        return 0;
                      }
                      lVar2 = FUN_003df360(param_1,uVar1 + 4,4);
                      if (lVar2 == 0) {
                        return 0;
                      }
                      lVar2 = FUN_003df360(param_1,uVar1 + 8,4);
                      if (lVar2 == 0) {
                        return 0;
                      }
                      *(unsigned int *)(uVar1 + 0xc) = 0;
                      iVar5 = iVar5 + 1;
                      uVar1 = uVar1 + 0x10;
                    } while (iVar5 < (int)puVar4[1]);
                  }
                }
                *(unsigned int **)(iVar3 + 4) = puVar4;
              }
            }
          }
        }
        else {
          param_1 = 0;
        }
      }
    }
  }
  else {
    param_1 = 0;
  }
  return param_1;
}
