/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c4fa0 func_003c4fa0 src/promoted/code1_003c.c - romwright draft, unreviewed */

long FUN_003c4fa0(unsigned long long param_1,long param_2)

{
  unsigned int *puVar1;
  int *piVar2;
  long lVar3;
  long lVar4;
  int *piVar5;
  int *piVar6;
  unsigned int *puVar7;
  int iVar8;
  int iVar9;
  int uStack_28;
  unsigned int uStack_24;
  int uStack_20;
  unsigned int uStack_1c;
  int uStack_18;
  unsigned int uStack_14;
  unsigned int uStack_c;
  unsigned char auStack_8 [4];
  int iStack_4;
  
  lVar3 = FUN_003df050(param_1,1,auStack_8,&uStack_c);
  if (lVar3 != 0) {
    if ((uStack_c >= 0x35000) && (0x37002 >= uStack_c)) {
      lVar3 = FUN_003df360(param_1,&iStack_4,4);
      iVar9 = iStack_4;
      if (lVar3 != 0) {
        piVar5 = (int *)param_2;
        piVar5[2] = 0;
        *piVar5 = 0;
        piVar5[1] = 0;
        if (iStack_4 != 0) {
          lVar3 = param_2;
          if (piVar5[2] < iStack_4) {
            iVar8 = iStack_4 << 2;
            if (*piVar5 != 0) {
              lVar4 = (*DAT_008873f0)(*piVar5,iVar8,0x1030008);
            }
            else {
              lVar4 = (*DAT_008873e8)(iVar8,0x1030008);
            }
            if (lVar4 != 0) {
              *piVar5 = (int)lVar4;
              piVar5[2] = iVar9;
            }
            else {
              uStack_28 = 2;
              uStack_24 = FUN_003df590(0xffffffff80000013,iVar8);
              FUN_003df4d0(&uStack_28);
              lVar3 = 0;
            }
          }
          if (lVar3 != 0) {
            piVar2 = (int *)(*DAT_008873e8)(iStack_4 << 2,0x10501);
            lVar3 = FUN_003df360(param_1,piVar2,iStack_4 << 2);
            if (lVar3 != 0) {
              iVar9 = 0;
              piVar6 = piVar2;
              if (0 < iStack_4) {
                do {
                  if (*piVar6 >= 0) {
                    iVar8 = *(int *)(*piVar5 + *piVar6 * 4);
                    *(short *)(iVar8 + 0x18) = *(short *)(iVar8 + 0x18) + 1;
                  }
                  else {
                    lVar3 = FUN_003df050(param_1,7,0,&uStack_c);
                    if (lVar3 == 0) {
                      (*DAT_008873ec)(piVar2);
                      puVar1 = (unsigned int *)*piVar5;
                      if (puVar1 != (unsigned int *)0x0) {
                        iVar9 = piVar5[1];
                        iVar8 = 0;
                        puVar7 = puVar1;
                        if (0 < iVar9) {
                          do {
                            FUN_003c4220(*puVar7);
                            iVar8 = iVar8 + 1;
                            *puVar7 = 0;
                            puVar7 = puVar7 + 1;
                          } while (iVar8 < iVar9);
                        }
                        (*DAT_008873ec)(puVar1);
                        *piVar5 = 0;
                      }
                      piVar5[1] = 0;
                      piVar5[2] = 0;
                      return 0;
                    }
                    if ((uStack_c < 0x35000) || (0x37002 < uStack_c)) {
                      uStack_18 = 2;
                      uStack_14 = FUN_003df590(0xffffffff80000004);
                      FUN_003df4d0(&uStack_18);
                      (*DAT_008873ec)(piVar2);
                      puVar1 = (unsigned int *)*piVar5;
                      if (puVar1 != (unsigned int *)0x0) {
                        iVar9 = piVar5[1];
                        iVar8 = 0;
                        puVar7 = puVar1;
                        if (0 < iVar9) {
                          do {
                            FUN_003c4220(*puVar7);
                            iVar8 = iVar8 + 1;
                            *puVar7 = 0;
                            puVar7 = puVar7 + 1;
                          } while (iVar8 < iVar9);
                        }
                        (*DAT_008873ec)(puVar1);
                        *piVar5 = 0;
                      }
                      piVar5[1] = 0;
                      piVar5[2] = 0;
                      return 0;
                    }
                    iVar8 = FUN_003c4390(param_1);
                    if (iVar8 == 0) {
                      (*DAT_008873ec)(piVar2);
                      puVar1 = (unsigned int *)*piVar5;
                      if (puVar1 != (unsigned int *)0x0) {
                        iVar9 = piVar5[1];
                        iVar8 = 0;
                        puVar7 = puVar1;
                        if (0 < iVar9) {
                          do {
                            FUN_003c4220(*puVar7);
                            iVar8 = iVar8 + 1;
                            *puVar7 = 0;
                            puVar7 = puVar7 + 1;
                          } while (iVar8 < iVar9);
                        }
                        (*DAT_008873ec)(puVar1);
                        *piVar5 = 0;
                      }
                      piVar5[1] = 0;
                      piVar5[2] = 0;
                      return 0;
                    }
                  }
                  FUN_003c4a80(param_2,iVar8);
                  FUN_003c4220(iVar8);
                  iVar9 = iVar9 + 1;
                  piVar6 = piVar6 + 1;
                } while (iVar9 < iStack_4);
              }
              (*DAT_008873ec)(piVar2);
            }
            else {
              (*DAT_008873ec)(piVar2);
              puVar1 = (unsigned int *)*piVar5;
              if (puVar1 != (unsigned int *)0x0) {
                iVar9 = piVar5[1];
                iVar8 = 0;
                puVar7 = puVar1;
                if (0 < iVar9) {
                  do {
                    FUN_003c4220(*puVar7);
                    iVar8 = iVar8 + 1;
                    *puVar7 = 0;
                    puVar7 = puVar7 + 1;
                  } while (iVar8 < iVar9);
                }
                (*DAT_008873ec)(puVar1);
                *piVar5 = 0;
              }
              piVar5[1] = 0;
              param_2 = 0;
              piVar5[2] = 0;
            }
          }
          else {
            puVar1 = (unsigned int *)*piVar5;
            if (puVar1 != (unsigned int *)0x0) {
              iVar9 = piVar5[1];
              iVar8 = 0;
              puVar7 = puVar1;
              if (0 < iVar9) {
                do {
                  FUN_003c4220(*puVar7);
                  iVar8 = iVar8 + 1;
                  *puVar7 = 0;
                  puVar7 = puVar7 + 1;
                } while (iVar8 < iVar9);
              }
              (*DAT_008873ec)(puVar1);
              *piVar5 = 0;
            }
            piVar5[1] = 0;
            param_2 = 0;
            piVar5[2] = 0;
          }
        }
      }
      else {
        param_2 = 0;
      }
    }
    else {
      uStack_20 = 2;
      uStack_1c = FUN_003df590(0xffffffff80000004);
      FUN_003df4d0(&uStack_20);
      param_2 = 0;
    }
  }
  else {
    param_2 = 0;
  }
  return param_2;
}
