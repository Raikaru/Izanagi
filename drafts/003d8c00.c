/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d8c00 func_003d8c00 src/promoted/code1_003d.c - romwright draft, unreviewed */

long FUN_003d8c00(unsigned long long param_1)

{
  unsigned int uVar1;
  unsigned char bVar2;
  int iVar3;
  int iVar4;
  unsigned int uVar5;
  long lVar6;
  long lVar7;
  long lVar8;
  long lVar9;
  unsigned long long uVar10;
  int iVar11;
  unsigned char *puVar12;
  unsigned int *puVar13;
  unsigned char *puVar14;
  unsigned long long uVar15;
  unsigned long long uVar16;
  int iVar17;
  long lVar18;
  long lVar19;
  int iStack_460;
  short sStack_450;
  int iStack_440;
  int uStack_430;
  int uStack_42c;
  int uStack_428;
  int uStack_424;
  unsigned char auStack_420 [12];
  int uStack_414;
  int uStack_28;
  unsigned int uStack_24;
  int uStack_20;
  unsigned int uStack_1c;
  int uStack_18;
  unsigned int uStack_14;
  int uStack_10;
  unsigned int uStack_c;
  unsigned char bStack_8;
  unsigned char bStack_7;
  short asStack_4 [2];
  
  lVar6 = FUN_003e2f60(2,1,param_1);
  if (lVar6 != 0) {
    lVar7 = FUN_003e2910(lVar6,asStack_4,4);
    if (lVar7 == 4) {
      if (asStack_4[0] == 0x4d42) {
        lVar7 = FUN_003e2910(lVar6,(int)&uStack_430 + 2,0xe);
        iVar4 = uStack_424;
        iVar11 = uStack_428;
        if (lVar7 == 0xe) {
          iVar17 = uStack_424 - 4;
          iVar3 = FUN_003e2910(lVar6,&uStack_430,iVar17);
          if (iVar17 == iVar3) {
            iVar3 = iVar4 + 0xe;
            if (iVar4 == 0xc) {
              uStack_414 = 0;
              lVar18 = (long)uStack_430._2_2_;
              iStack_440 = 0;
              iStack_460 = (int)(short)uStack_430;
              sStack_450 = uStack_42c._2_2_;
            }
            else {
              iStack_440 = uStack_424;
              iStack_460 = uStack_430;
              sStack_450 = uStack_428._2_2_;
              lVar18 = (long)uStack_42c;
              if (uStack_424 == 2) {
                uStack_10 = 0x19d;
                uStack_c = FUN_003df590(0xffffffff80000009);
                FUN_003df4d0(&uStack_10);
                FUN_003e2e40(lVar6,0);
                return 0;
              }
            }
            if ((uStack_414 < 1) ||
               (iVar17 = uStack_414, 1 << ((int)sStack_450 & 0x1fU) < uStack_414)) {
              iVar17 = 1 << ((int)sStack_450 & 0x1fU);
            }
            if ((((sStack_450 != 0x20) && (sStack_450 != 0x18)) && (sStack_450 != 0x10)) &&
               (sStack_450 != 0xf)) {
              uVar15 = 8;
              if (sStack_450 != 8) {
                if ((sStack_450 == 4) || (sStack_450 == 1)) {
                  uVar15 = 4;
                }
                else {
                  uVar15 = 0x20;
                }
              }
            }
            else {
              uVar15 = 0x20;
            }
            lVar7 = FUN_003ea2c0(iStack_460,lVar18,uVar15);
            if (lVar7 != 0) {
              lVar8 = FUN_003ea3e0(lVar7);
              if (lVar8 != 0) {
                if (sStack_450 < 9) {
                  puVar14 = *(unsigned char **)((int)lVar7 + 0x18);
                  if (iVar4 != 0xc) {
                    if (iVar4 != 0x28) {
                      uStack_18 = 0x19d;
                      uStack_14 = FUN_003df590(0xffffffff80000009);
                      FUN_003df4d0(&uStack_18);
                      FUN_003e2e40(lVar6,0);
                      return 0;
                    }
                    iVar3 = iVar17 * 4;
                    iVar4 = FUN_003e2910(lVar6,&uStack_430,iVar3);
                    if (iVar3 != iVar4) {
                      FUN_003e2e40(lVar6,0);
                      return 0;
                    }
                    iVar3 = iVar3 + 0x36;
                    iVar4 = 0;
                    if (0 < iVar17) {
                      if (8 < iVar17) {
                        bVar2 = 0;
                        if ((-1 < iVar17) && (iVar17 < 0x7fffffff)) {
                          bVar2 = 1;
                        }
                        if (bVar2) {
                          puVar13 = &uStack_430;
                          puVar12 = puVar14;
                          do {
                            iVar4 = iVar4 + 8;
                            *puVar12 = *(unsigned char *)((int)puVar13 + 2);
                            puVar12[1] = *(unsigned char *)((int)puVar13 + 1);
                            puVar12[2] = *(unsigned char *)puVar13;
                            puVar12[3] = 0xff;
                            puVar12[4] = *(unsigned char *)((int)puVar13 + 6);
                            puVar12[5] = *(unsigned char *)((int)puVar13 + 5);
                            puVar12[6] = *(unsigned char *)(puVar13 + 1);
                            puVar12[7] = 0xff;
                            puVar12[8] = *(unsigned char *)((int)puVar13 + 10);
                            puVar12[9] = *(unsigned char *)((int)puVar13 + 9);
                            puVar12[10] = *(unsigned char *)(puVar13 + 2);
                            puVar12[0xb] = 0xff;
                            puVar12[0xc] = *(unsigned char *)((int)puVar13 + 0xe);
                            puVar12[0xd] = *(unsigned char *)((int)puVar13 + 0xd);
                            puVar12[0xe] = *(unsigned char *)(puVar13 + 3);
                            puVar12[0xf] = 0xff;
                            puVar12[0x10] = *(unsigned char *)((int)puVar13 + 0x12);
                            puVar12[0x11] = *(unsigned char *)((int)puVar13 + 0x11);
                            puVar12[0x12] = *(unsigned char *)(puVar13 + 4);
                            puVar12[0x13] = 0xff;
                            puVar12[0x14] = *(unsigned char *)((int)puVar13 + 0x16);
                            puVar12[0x15] = *(unsigned char *)((int)puVar13 + 0x15);
                            puVar12[0x16] = *(unsigned char *)(puVar13 + 5);
                            puVar12[0x17] = 0xff;
                            puVar12[0x18] = *(unsigned char *)((int)puVar13 + 0x1a);
                            puVar12[0x19] = *(unsigned char *)((int)puVar13 + 0x19);
                            puVar12[0x1a] = *(unsigned char *)(puVar13 + 6);
                            puVar12[0x1b] = 0xff;
                            puVar12[0x1c] = *(unsigned char *)((int)puVar13 + 0x1e);
                            puVar12[0x1d] = *(unsigned char *)((int)puVar13 + 0x1d);
                            puVar12[0x1e] = *(unsigned char *)(puVar13 + 7);
                            puVar13 = puVar13 + 8;
                            puVar12[0x1f] = 0xff;
                            puVar12 = puVar12 + 0x20;
                          } while (iVar4 < iVar17 - 8);
                        }
                      }
                      if (iVar4 < iVar17) {
                        puVar14 = puVar14 + iVar4 * 4;
                        puVar13 = &uStack_430 + iVar4;
                        do {
                          iVar4 = iVar4 + 1;
                          *puVar14 = *(unsigned char *)((int)puVar13 + 2);
                          puVar14[1] = *(unsigned char *)((int)puVar13 + 1);
                          puVar14[2] = *(unsigned char *)puVar13;
                          puVar13 = puVar13 + 1;
                          puVar14[3] = 0xff;
                          puVar14 = puVar14 + 4;
                        } while (iVar4 < iVar17);
                      }
                    }
                  }
                  else {
                    iVar3 = iVar17 * 3;
                    iVar4 = FUN_003e2910(lVar6,&uStack_430,iVar3);
                    if (iVar3 != iVar4) {
                      FUN_003e2e40(lVar6,0);
                      return 0;
                    }
                    iVar3 = iVar3 + 0x1a;
                    iVar4 = 0;
                    if (0 < iVar17) {
                      if (8 < iVar17) {
                        bVar2 = 0;
                        if ((-1 < iVar17) && (iVar17 < 0x7fffffff)) {
                          bVar2 = 1;
                        }
                        if (bVar2) {
                          puVar13 = &uStack_430;
                          puVar12 = puVar14;
                          do {
                            iVar4 = iVar4 + 8;
                            *puVar12 = *(unsigned char *)((int)puVar13 + 2);
                            puVar12[1] = *(unsigned char *)((int)puVar13 + 1);
                            puVar12[2] = *(unsigned char *)puVar13;
                            puVar12[3] = 0xff;
                            puVar12[4] = *(unsigned char *)((int)puVar13 + 5);
                            puVar12[5] = *(unsigned char *)(puVar13 + 1);
                            puVar12[6] = *(unsigned char *)((int)puVar13 + 3);
                            puVar12[7] = 0xff;
                            puVar12[8] = *(unsigned char *)(puVar13 + 2);
                            puVar12[9] = *(unsigned char *)((int)puVar13 + 7);
                            puVar12[10] = *(unsigned char *)((int)puVar13 + 6);
                            puVar12[0xb] = 0xff;
                            puVar12[0xc] = *(unsigned char *)((int)puVar13 + 0xb);
                            puVar12[0xd] = *(unsigned char *)((int)puVar13 + 10);
                            puVar12[0xe] = *(unsigned char *)((int)puVar13 + 9);
                            puVar12[0xf] = 0xff;
                            puVar12[0x10] = *(unsigned char *)((int)puVar13 + 0xe);
                            puVar12[0x11] = *(unsigned char *)((int)puVar13 + 0xd);
                            puVar12[0x12] = *(unsigned char *)(puVar13 + 3);
                            puVar12[0x13] = 0xff;
                            puVar12[0x14] = *(unsigned char *)((int)puVar13 + 0x11);
                            puVar12[0x15] = *(unsigned char *)(puVar13 + 4);
                            puVar12[0x16] = *(unsigned char *)((int)puVar13 + 0xf);
                            puVar12[0x17] = 0xff;
                            puVar12[0x18] = *(unsigned char *)(puVar13 + 5);
                            puVar12[0x19] = *(unsigned char *)((int)puVar13 + 0x13);
                            puVar12[0x1a] = *(unsigned char *)((int)puVar13 + 0x12);
                            puVar12[0x1b] = 0xff;
                            puVar12[0x1c] = *(unsigned char *)((int)puVar13 + 0x17);
                            puVar12[0x1d] = *(unsigned char *)((int)puVar13 + 0x16);
                            puVar12[0x1e] = *(unsigned char *)((int)puVar13 + 0x15);
                            puVar13 = puVar13 + 6;
                            puVar12[0x1f] = 0xff;
                            puVar12 = puVar12 + 0x20;
                          } while (iVar4 < iVar17 - 8);
                        }
                      }
                      if (iVar4 < iVar17) {
                        puVar14 = puVar14 + iVar4 * 4;
                        puVar12 = (unsigned char *)((int)&uStack_430 + iVar4 * 3);
                        do {
                          iVar4 = iVar4 + 1;
                          *puVar14 = puVar12[2];
                          puVar14[1] = puVar12[1];
                          puVar14[2] = *puVar12;
                          puVar12 = puVar12 + 3;
                          puVar14[3] = 0xff;
                          puVar14 = puVar14 + 4;
                        } while (iVar4 < iVar17);
                      }
                    }
                  }
                }
                lVar8 = FUN_003e2ce0(lVar6,iVar11 - iVar3);
                if (lVar8 != 0) {
                  iVar4 = (int)sStack_450;
                  iVar11 = iStack_460 * iVar4 + 7;
                  if (iVar11 < 0) {
                    iVar11 = iStack_460 * iVar4 + 0xe;
                  }
                  uVar1 = ((iVar11 >> 3) + 3U) & 0xfffffffc;
                  iVar11 = iVar4 + 7;
                  if (iVar11 < 0) {
                    iVar11 = iVar4 + 0xe;
                  }
                  iVar11 = ((iStack_460 + 7U) & 0xfffffff8) * (iVar11 >> 3);
                  lVar8 = (*DAT_008873e8)(iVar11,0x10000);
                  if (lVar8 != 0) {
                    iVar11 = (int)lVar18;
                    if (((iStack_440 != 0) && (sStack_450 != 0x18)) && (sStack_450 != 0x20)) {
                      if (iStack_440 != 1) {
                        uStack_28 = 0x19d;
                        uStack_24 = FUN_003df590(0xffffffff80000009);
                        FUN_003df4d0(&uStack_28);
                        (*DAT_008873ec)(lVar8);
                        FUN_003ea370(lVar7);
                        FUN_003e2e40(lVar6,0);
                        return 0;
                      }
                      lVar19 = 0;
                      if (0 < lVar18) {
                        do {
                          iVar4 = 0;
                          bVar2 = 0;
                          do {
                            lVar9 = FUN_003e2910(lVar6,&bStack_8,2);
                            if (lVar9 != 2) {
                              (*DAT_008873ec)(lVar8);
                              FUN_003ea370(lVar7);
                              FUN_003e2e40(lVar6,0);
                              return 0;
                            }
                            if (bStack_8 == 0) {
                              uVar16 = (unsigned long long)bStack_7;
                              if (uVar16 < 3) {
                                if (uVar16 != 0) {
                                  bVar2 = 1;
                                  if (uVar16 != 1) {
                                    (*DAT_008873ec)(lVar8);
                                    FUN_003ea370(lVar7);
                                    FUN_003e2e40(lVar6,0);
                                    return 0;
                                  }
                                  lVar19 = (long)(iVar11 - 1);
                                }
                                else {
                                  bVar2 = 1;
                                }
                              }
                              else {
                                uVar10 = FUN_003e2910(lVar6,(int)lVar8 + iVar4,uVar16);
                                if (uVar16 != uVar10) {
                                  (*DAT_008873ec)(lVar8);
                                  FUN_003ea370(lVar7);
                                  FUN_003e2e40(lVar6,0);
                                  return 0;
                                }
                                iVar4 = iVar4 + (unsigned int)bStack_7;
                                if (((bStack_7 & 1) != 0) &&
                                   (lVar9 = FUN_003e2910(lVar6,&bStack_8,1), lVar9 != 1)) {
                                  (*DAT_008873ec)(lVar8);
                                  FUN_003ea370(lVar7);
                                  FUN_003e2e40(lVar6,0);
                                  return 0;
                                }
                              }
                            }
                            else {
                              FUN_0043f9c8((int)lVar8 + iVar4,bStack_7);
                              iVar4 = iVar4 + (unsigned int)bStack_8;
                            }
                          } while (!bVar2);
                          FUN_003d8760(lVar7,(iVar11 - 1) - (int)lVar19,lVar8,sStack_450,1);
                          lVar19 = (long)((int)lVar19 + 1);
                        } while (lVar19 < lVar18);
                      }
                    }
                    else {
                      lVar19 = 0;
                      if (0 < lVar18) {
                        do {
                          uVar5 = FUN_003e2910(lVar6,lVar8,uVar1);
                          if (uVar1 != uVar5) {
                            (*DAT_008873ec)(lVar8);
                            FUN_003ea370(lVar7);
                            FUN_003e2e40(lVar6,0);
                            return 0;
                          }
                          FUN_003d8760(lVar7,(iVar11 - 1) - (int)lVar19,lVar8,sStack_450,0);
                          lVar19 = (long)((int)lVar19 + 1);
                        } while (lVar19 < lVar18);
                      }
                    }
                    (*DAT_008873ec)(lVar8);
                    FUN_003e2e40(lVar6,0);
                  }
                  else {
                    uStack_20 = 0x19d;
                    uStack_1c = FUN_003df590(0xffffffff80000013,iVar11);
                    FUN_003df4d0(&uStack_20);
                    FUN_003ea370(lVar7);
                    FUN_003e2e40(lVar6,0);
                    lVar7 = 0;
                  }
                }
                else {
                  FUN_003ea370(lVar7);
                  FUN_003e2e40(lVar6,0);
                  lVar7 = 0;
                }
              }
              else {
                FUN_003ea370(lVar7);
                FUN_003e2e40(lVar6,0);
                lVar7 = 0;
              }
            }
            else {
              FUN_003e2e40(lVar6,0);
              lVar7 = 0;
            }
          }
          else {
            FUN_003e2e40(lVar6,0);
            lVar7 = 0;
          }
        }
        else {
          FUN_003e2e40(lVar6,0);
          lVar7 = 0;
        }
      }
      else {
        FUN_003e2e40(lVar6,0);
        lVar7 = 0;
      }
    }
    else {
      FUN_003e2e40(lVar6,0);
      lVar7 = 0;
    }
  }
  else {
    lVar7 = 0;
  }
  return lVar7;
}
