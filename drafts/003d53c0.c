/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d53c0 func_003d53c0 src/promoted/code1_003d.c - romwright draft, unreviewed */

long FUN_003d53c0(unsigned long long param_1)

{
  long lVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  int uStack_20;
  unsigned int uStack_1c;
  int iStack_14;
  int iStack_10;
  int iStack_c;
  int iStack_8;
  int iStack_4;
  
  lVar1 = FUN_003df360(param_1,&iStack_4,4);
  if (lVar1 != 0) {
    if (iStack_4 != 0x100) {
      lVar1 = 0;
    }
    else {
      lVar1 = FUN_003df360(param_1,&iStack_8,4);
      if (lVar1 != 0) {
        iVar4 = 0;
        if (0 < DAT_00764824) {
          piVar2 = (int *)0x886e50;
          do {
            if (iStack_8 == *piVar2) {
              iVar4 = iVar4 * 0x30 + 0x886e50;
              goto LAB_003d54a8;
            }
            iVar4 = iVar4 + 1;
            piVar2 = piVar2 + 0xc;
          } while (iVar4 < DAT_00764824);
        }
        uStack_20 = 0x1b7;
        uStack_1c = FUN_003df590(2);
        FUN_003df4d0(&uStack_20);
        iVar4 = 0;
LAB_003d54a8:
        if (iVar4 != 0) {
          lVar1 = FUN_003df360(param_1,&iStack_c,4);
          if (lVar1 == 0) {
            lVar1 = 0;
          }
          else {
            lVar1 = FUN_003df360(param_1,&iStack_10,4);
            if (lVar1 == 0) {
              lVar1 = 0;
            }
            else {
              lVar1 = FUN_003df300(param_1,&iStack_14,4);
              if (lVar1 == 0) {
                lVar1 = 0;
              }
              else {
                lVar1 = FUN_003d5130(iStack_8);
                if (lVar1 != 0) {
                  iVar3 = (int)lVar1;
                  lVar1 = (*DAT_008873e8)(*(int *)(iVar3 + 0x2c) +
                                          iStack_c * *(int *)(iVar3 + 8) + 0x18,0x3001b);
                  piVar2 = (int *)lVar1;
                  piVar2[1] = iStack_c;
                  piVar2[3] = iStack_14;
                  piVar2[2] = iStack_10;
                  *piVar2 = iVar3;
                  piVar2[4] = (int)(piVar2 + 6);
                  if (*(int *)(iVar3 + 0x2c) >= 1) {
                    piVar2[5] = piVar2[4] + iStack_c * *(int *)(iVar3 + 8);
                  }
                  else {
                    piVar2[5] = 0;
                  }
                }
                else {
                  lVar1 = 0;
                }
                if (lVar1 != 0) {
                  (**(code **)(iVar4 + 0x20))(param_1,lVar1);
                }
                else {
                  lVar1 = 0;
                }
              }
            }
          }
        }
        else {
          lVar1 = 0;
        }
      }
      else {
        lVar1 = 0;
      }
    }
  }
  else {
    lVar1 = 0;
  }
  return lVar1;
}
