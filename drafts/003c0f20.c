/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c0f20 func_003c0f20 src/promoted/code1_003c.c - romwright draft, unreviewed */

long FUN_003c0f20(unsigned long long param_1)

{
  unsigned char bVar1;
  long lVar2;
  long lVar3;
  int *piVar4;
  long unaff_s0;
  int iVar5;
  long unaff_s1;
  int iVar6;
  long unaff_s2;
  int iVar7;
  int uStack_90;
  unsigned int uStack_8c;
  int uStack_88;
  unsigned int uStack_84;
  int uStack_80;
  unsigned int uStack_7c;
  int uStack_78;
  unsigned int uStack_74;
  int uStack_70;
  unsigned int uStack_6c;
  int uStack_68;
  unsigned int uStack_64;
  int uStack_60;
  unsigned int uStack_5c;
  int uStack_58;
  unsigned int uStack_54;
  int uStack_50;
  unsigned int uStack_4c;
  int uStack_48;
  unsigned int uStack_44;
  int iStack_40;
  int iStack_3c;
  unsigned int *apuStack_38 [2];
  int iStack_30;
  int iStack_2c;
  int iStack_28;
  int uStack_20;
  unsigned int uStack_1c;
  int iStack_14;
  int iStack_10;
  unsigned char auStack_c [4];
  unsigned int uStack_8;
  unsigned char auStack_4 [4];
  
  lVar2 = FUN_003df050(param_1,1,auStack_4,&uStack_8);
  if (lVar2 != 0) {
    if ((uStack_8 >= 0x35000) && (0x37002 >= uStack_8)) {
      lVar2 = FUN_003e2910(param_1,&iStack_30,0xc);
      if (lVar2 == 0xc) {
        lVar2 = FUN_003c0640();
        if (lVar2 != 0) {
          lVar3 = FUN_003df050(param_1,0xe,0,auStack_c);
          iVar7 = (int)lVar2;
          if (lVar3 == 0) {
            FUN_003e3c20(0x70af90,lVar2);
            FUN_003bff30(lVar2,0x3bf330,0);
            FUN_003c0050(lVar2,0x3bf200,0);
            FUN_003bffc0(lVar2,0x3bf240,0);
            if (*(int *)(iVar7 + 4) != 0) {
              FUN_003e95c0();
            }
            (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
            uStack_50 = 2;
            uStack_4c = FUN_003df590(0xffffffff8000001a);
            FUN_003df4d0(&uStack_50);
            lVar2 = 0;
          }
          else {
            lVar3 = FUN_003e5df0(param_1,apuStack_38);
            if (lVar3 == 0) {
              FUN_003e3c20(0x70af90,lVar2);
              FUN_003bff30(lVar2,0x3bf330,0);
              FUN_003c0050(lVar2,0x3bf200,0);
              FUN_003bffc0(lVar2,0x3bf240,0);
              if (*(int *)(iVar7 + 4) != 0) {
                FUN_003e95c0();
              }
              (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
              uStack_58 = 2;
              uStack_54 = FUN_003df590(0xffffffff8000001a);
              FUN_003df4d0(&uStack_58);
              lVar2 = 0;
            }
            else {
              *(unsigned int *)(iVar7 + 4) = *apuStack_38[0];
              lVar3 = FUN_003df050(param_1,0x1a,0,auStack_c);
              if (lVar3 == 0) {
                FUN_003e59e0(apuStack_38);
                FUN_003e3c20(0x70af90,lVar2);
                FUN_003bff30(lVar2,0x3bf330,0);
                FUN_003c0050(lVar2,0x3bf200,0);
                FUN_003bffc0(lVar2,0x3bf240,0);
                if (*(int *)(iVar7 + 4) != 0) {
                  FUN_003e95c0();
                }
                (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
                uStack_60 = 2;
                uStack_5c = FUN_003df590(0xffffffff8000001a);
                FUN_003df4d0(&uStack_60);
                lVar2 = 0;
              }
              else {
                lVar3 = FUN_003bee80(param_1,&iStack_40);
                if (lVar3 != 0) {
                  iVar6 = 0;
                  if (0 < iStack_30) {
                    do {
                      lVar3 = FUN_003df050(param_1,0x14,0,&uStack_8);
                      bVar1 = 0;
                      if (lVar3 != 0) {
                        unaff_s2 = FUN_003bf5f0(param_1,apuStack_38,&iStack_40);
                        bVar1 = unaff_s2 != 0;
                      }
                      if (!bVar1) {
                        iVar6 = 0;
                        if (0 < iStack_3c) {
                          iVar5 = 0;
                          do {
                            FUN_003c2a80(*(unsigned int *)(iStack_40 + iVar5));
                            iVar6 = iVar6 + 1;
                            iVar5 = iVar5 + 4;
                          } while (iVar6 < iStack_3c);
                        }
                        if (iStack_40 != 0) {
                          (*DAT_008873ec)();
                          iStack_40 = 0;
                        }
                        FUN_003e59e0(apuStack_38);
                        FUN_003e3c20(0x70af90,lVar2);
                        FUN_003bff30(lVar2,0x3bf330,0);
                        FUN_003c0050(lVar2,0x3bf200,0);
                        FUN_003bffc0(lVar2,0x3bf240,0);
                        if (*(int *)(iVar7 + 4) != 0) {
                          FUN_003e95c0();
                        }
                        (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
                        uStack_70 = 2;
                        uStack_6c = FUN_003df590(0xffffffff8000001a);
                        FUN_003df4d0(&uStack_70);
                        return 0;
                      }
                      iVar5 = (int)unaff_s2;
                      iVar6 = iVar6 + 1;
                      *(unsigned int *)(iVar5 + 0x40) = *(unsigned int *)(iVar7 + 8);
                      *(int *)(iVar5 + 0x44) = iVar7 + 8;
                      *(int *)(*(int *)(iVar7 + 8) + 4) = iVar5 + 0x40;
                      *(int *)(iVar7 + 8) = iVar5 + 0x40;
                      *(int *)(iVar5 + 0x3c) = iVar7;
                    } while (iVar6 < iStack_30);
                  }
                  iVar6 = 0;
                  if (0 < iStack_2c) {
                    do {
                      lVar3 = FUN_003df050(param_1,1,0,0);
                      bVar1 = 0;
                      if (lVar3 != 0) {
                        lVar3 = FUN_003df360(param_1,&iStack_10,4);
                        bVar1 = lVar3 != 0;
                      }
                      lVar3 = 0;
                      if (bVar1) {
                        lVar3 = FUN_003df050(param_1,0x12,0,0);
                      }
                      bVar1 = 0;
                      if (lVar3 != 0) {
                        unaff_s1 = FUN_003c3ae0(param_1);
                        bVar1 = unaff_s1 != 0;
                      }
                      if (!bVar1) {
                        iVar6 = 0;
                        if (0 < iStack_3c) {
                          iVar5 = 0;
                          do {
                            FUN_003c2a80(*(unsigned int *)(iStack_40 + iVar5));
                            iVar6 = iVar6 + 1;
                            iVar5 = iVar5 + 4;
                          } while (iVar6 < iStack_3c);
                        }
                        if (iStack_40 != 0) {
                          (*DAT_008873ec)();
                          iStack_40 = 0;
                        }
                        FUN_003e59e0(apuStack_38);
                        FUN_003e3c20(0x70af90,lVar2);
                        FUN_003bff30(lVar2,0x3bf330,0);
                        FUN_003c0050(lVar2,0x3bf200,0);
                        FUN_003bffc0(lVar2,0x3bf240,0);
                        if (*(int *)(iVar7 + 4) != 0) {
                          FUN_003e95c0();
                        }
                        (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
                        uStack_78 = 2;
                        uStack_74 = FUN_003df590(0xffffffff8000001a);
                        FUN_003df4d0(&uStack_78);
                        return 0;
                      }
                      FUN_003efd20(unaff_s1,apuStack_38[0][iStack_10]);
                      iVar6 = iVar6 + 1;
                      piVar4 = (int *)((int)unaff_s1 + DAT_007647a4);
                      piVar4[1] = *(int *)(iVar7 + 0x10);
                      piVar4[2] = iVar7 + 0x10;
                      *(int **)(*(int *)(iVar7 + 0x10) + 4) = piVar4 + 1;
                      *(int **)(iVar7 + 0x10) = piVar4 + 1;
                      *piVar4 = iVar7;
                    } while (iVar6 < iStack_2c);
                  }
                  iVar6 = 0;
                  if (0 < iStack_28) {
                    do {
                      lVar3 = FUN_003df050(param_1,1,0,0);
                      bVar1 = 0;
                      if (lVar3 != 0) {
                        lVar3 = FUN_003df360(param_1,&iStack_14,4);
                        bVar1 = lVar3 != 0;
                      }
                      lVar3 = 0;
                      if (bVar1) {
                        lVar3 = FUN_003df050(param_1,5,0,0);
                      }
                      bVar1 = 0;
                      if (lVar3 != 0) {
                        unaff_s0 = FUN_003e53b0(param_1);
                        bVar1 = unaff_s0 != 0;
                      }
                      if (!bVar1) {
                        iVar6 = 0;
                        if (0 < iStack_3c) {
                          iVar5 = 0;
                          do {
                            FUN_003c2a80(*(unsigned int *)(iStack_40 + iVar5));
                            iVar6 = iVar6 + 1;
                            iVar5 = iVar5 + 4;
                          } while (iVar6 < iStack_3c);
                        }
                        if (iStack_40 != 0) {
                          (*DAT_008873ec)();
                          iStack_40 = 0;
                        }
                        FUN_003e59e0(apuStack_38);
                        FUN_003e3c20(0x70af90,lVar2);
                        FUN_003bff30(lVar2,0x3bf330,0);
                        FUN_003c0050(lVar2,0x3bf200,0);
                        FUN_003bffc0(lVar2,0x3bf240,0);
                        if (*(int *)(iVar7 + 4) != 0) {
                          FUN_003e95c0();
                        }
                        (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
                        uStack_80 = 2;
                        uStack_7c = FUN_003df590(0xffffffff8000001a);
                        FUN_003df4d0(&uStack_80);
                        return 0;
                      }
                      FUN_003efd20(unaff_s0,apuStack_38[0][iStack_14]);
                      iVar6 = iVar6 + 1;
                      piVar4 = (int *)((int)unaff_s0 + DAT_007647a0);
                      piVar4[1] = *(int *)(iVar7 + 0x18);
                      piVar4[2] = iVar7 + 0x18;
                      *(int **)(*(int *)(iVar7 + 0x18) + 4) = piVar4 + 1;
                      *(int **)(iVar7 + 0x18) = piVar4 + 1;
                      *piVar4 = iVar7;
                    } while (iVar6 < iStack_28);
                  }
                  iVar6 = 0;
                  if (0 < iStack_3c) {
                    iVar5 = 0;
                    do {
                      FUN_003c2a80(*(unsigned int *)(iStack_40 + iVar5));
                      iVar6 = iVar6 + 1;
                      iVar5 = iVar5 + 4;
                    } while (iVar6 < iStack_3c);
                  }
                  if (iStack_40 != 0) {
                    (*DAT_008873ec)();
                    iStack_40 = 0;
                  }
                  FUN_003e59e0(apuStack_38);
                  lVar3 = FUN_003e3110(0x70af90,param_1,lVar2);
                  if (lVar3 == 0) {
                    FUN_003e3c20(0x70af90,lVar2);
                    FUN_003bff30(lVar2,0x3bf330,0);
                    FUN_003c0050(lVar2,0x3bf200,0);
                    FUN_003bffc0(lVar2,0x3bf240,0);
                    if (*(int *)(iVar7 + 4) != 0) {
                      FUN_003e95c0();
                    }
                    (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
                    uStack_88 = 2;
                    uStack_84 = FUN_003df590(0xffffffff8000001a);
                    FUN_003df4d0(&uStack_88);
                    lVar2 = 0;
                  }
                }
                else {
                  FUN_003e59e0(apuStack_38);
                  FUN_003e3c20(0x70af90,lVar2);
                  FUN_003bff30(lVar2,0x3bf330,0);
                  FUN_003c0050(lVar2,0x3bf200,0);
                  FUN_003bffc0(lVar2,0x3bf240,0);
                  if (*(int *)(iVar7 + 4) != 0) {
                    FUN_003e95c0();
                  }
                  (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),lVar2);
                  uStack_68 = 2;
                  uStack_64 = FUN_003df590(0xffffffff8000001a);
                  FUN_003df4d0(&uStack_68);
                  lVar2 = 0;
                }
              }
            }
          }
        }
        else {
          lVar2 = 0;
        }
      }
      else {
        uStack_48 = 2;
        uStack_44 = FUN_003df590(0xffffffff8000001a);
        FUN_003df4d0(&uStack_48);
        lVar2 = 0;
      }
    }
    else {
      uStack_90 = 2;
      uStack_8c = FUN_003df590(0xffffffff80000004);
      FUN_003df4d0(&uStack_90);
      lVar2 = 0;
    }
  }
  else {
    uStack_20 = 2;
    uStack_1c = FUN_003df590(0xffffffff8000001a);
    FUN_003df4d0(&uStack_20);
    lVar2 = 0;
  }
  return lVar2;
}
