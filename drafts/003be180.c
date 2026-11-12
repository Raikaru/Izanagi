/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003be180 func_003be180 src/promoted/code1_003b.c - romwright draft, unreviewed */

long FUN_003be180(unsigned long long param_1)

{
  int iVar1;
  long lVar2;
  long lVar3;
  unsigned char *puVar4;
  unsigned int uVar5;
  int iVar6;
  int iStack_80;
  float fStack_7c;
  float fStack_78;
  float fStack_74;
  int iStack_70;
  int iStack_6c;
  int iStack_68;
  int iStack_64;
  int iStack_60;
  unsigned int uStack_5c;
  unsigned int uStack_58;
  unsigned int uStack_54;
  unsigned int uStack_50;
  unsigned int uStack_4c;
  unsigned int uStack_48;
  unsigned int uStack_44;
  int uStack_40;
  unsigned int uStack_3c;
  int uStack_38;
  unsigned int uStack_34;
  int uStack_30;
  unsigned int uStack_2c;
  int uStack_28;
  unsigned int uStack_24;
  int uStack_20;
  unsigned int uStack_1c;
  int uStack_18;
  unsigned int uStack_14;
  unsigned int uStack_10;
  unsigned int uStack_c;
  int iStack_8;
  unsigned char *puStack_4;
  
  lVar2 = FUN_003df050(param_1,1,&iStack_8,&uStack_c);
  if (lVar2 != 0) {
    if ((uStack_c < 0x35000) || (0x37002 < uStack_c)) {
      uStack_18 = 2;
      uStack_14 = FUN_003df590(0xffffffff80000004);
      FUN_003df4d0(&uStack_18);
      lVar2 = 0;
    }
    else {
      FUN_0043f9c8(&iStack_80,0,0x40);
      iVar1 = FUN_003e2910(param_1,&iStack_80,iStack_8);
      if (iStack_8 == iVar1) {
        if (iStack_60 < 1) {
          iVar1 = DAT_0070b060 + iStack_68 * 0x18 + iStack_64 * DAT_0070b040;
          if ((uStack_5c & 0xff0000) != 0) {
            uVar5 = (int)(uStack_5c & 0xff0000) >> 0x10;
          }
          else if ((uStack_5c & 0x80) != 0) {
            uVar5 = 2;
          }
          else {
            uVar5 = (unsigned int)((uStack_5c & 4) != 0);
          }
          if ((uStack_5c & 0x1000000) == 0) {
            iVar1 = iVar1 + iStack_6c * 0xc;
            if ((uStack_5c & 0x10) != 0) {
              iVar1 = iVar1 + iStack_6c * 4;
            }
            if ((uStack_5c & 8) != 0) {
              iVar1 = iVar1 + iStack_6c * 4;
            }
            if (uVar5 != 0) {
              iVar1 = iVar1 + uVar5 * iStack_6c * 8;
            }
            iVar1 = iVar1 + iStack_70 * 8;
          }
          lVar2 = (*DAT_008873e8)(iVar1,0x3000b);
          if (lVar2 != 0) {
            FUN_0043f9c8(lVar2,0,iVar1);
            puVar4 = (unsigned char *)lVar2;
            puStack_4 = puVar4 + DAT_0070b060;
            *puVar4 = 7;
            puVar4[1] = 0;
            puVar4[2] = 0;
            puVar4[3] = 0;
            *(unsigned int *)(puVar4 + 4) = 0;
            puVar4[3] = 1;
            FUN_003c9940(lVar2,iVar1);
            *(unsigned int *)(puVar4 + 8) = uStack_5c;
            *(unsigned int *)(puVar4 + 0xc) = 2;
            *(float *)(puVar4 + 0x44) = fStack_7c * -1.0f;
            *(float *)(puVar4 + 0x48) = fStack_78 * -1.0f;
            *(float *)(puVar4 + 0x4c) = fStack_74 * -1.0f;
            *(unsigned int *)(puVar4 + 0x20) = uVar5;
            *(unsigned int *)(puVar4 + 0x1c) = 0;
            *(unsigned int *)(puVar4 + 0x6c) = 0;
            *(unsigned char **)(puVar4 + 0x2c) = puVar4 + 0x2c;
            *(unsigned char **)(puVar4 + 0x30) = puVar4 + 0x2c;
            *(unsigned char **)(puVar4 + 0x34) = puVar4 + 0x34;
            *(unsigned char **)(puVar4 + 0x38) = puVar4 + 0x34;
            *(unsigned char **)(puVar4 + 0x3c) = puVar4 + 0x3c;
            *(unsigned char **)(puVar4 + 0x40) = puVar4 + 0x3c;
            lVar3 = FUN_003df050(param_1,8,0,&uStack_10);
            if (lVar3 != 0) {
              if ((uStack_10 < 0x35000) || (0x37002 < uStack_10)) {
                uStack_30 = 2;
                uStack_2c = FUN_003df590(0xffffffff80000004);
                FUN_003df4d0(&uStack_30);
                FUN_003c9d40(lVar2);
                lVar2 = 0;
              }
              else {
                lVar3 = FUN_003c4fa0(param_1,puVar4 + 0x10);
                if (lVar3 != 0) {
                  if (iStack_80 != 0) {
                    lVar3 = FUN_003df050(param_1,9,0,&uStack_10);
                    if (lVar3 == 0) {
                      FUN_003c9d40(lVar2);
                      return 0;
                    }
                    if ((uStack_10 < 0x35000) || (0x37002 < uStack_10)) {
                      uStack_38 = 2;
                      uStack_34 = FUN_003df590(0xffffffff80000004);
                      FUN_003df4d0(&uStack_38);
                      FUN_003c9d40(lVar2);
                      return 0;
                    }
                    lVar3 = FUN_003bd6b0(param_1,&puStack_4,lVar2,uStack_5c);
                    *(int *)(puVar4 + 0x1c) = (int)lVar3;
                    if (lVar3 == 0) {
                      FUN_003c9d40(lVar2);
                      return 0;
                    }
                  }
                  else {
                    lVar3 = FUN_003df050(param_1,10,0,&uStack_10);
                    if (lVar3 == 0) {
                      FUN_003c9d40(lVar2);
                      return 0;
                    }
                    if ((uStack_10 < 0x35000) || (0x37002 < uStack_10)) {
                      uStack_40 = 2;
                      uStack_3c = FUN_003df590(0xffffffff80000004);
                      FUN_003df4d0(&uStack_40);
                      FUN_003c9d40(lVar2);
                      return 0;
                    }
                    lVar3 = FUN_003bdd00(param_1,&puStack_4,lVar2,uStack_5c);
                    *(int *)(puVar4 + 0x1c) = (int)lVar3;
                    if (lVar3 == 0) {
                      FUN_003c9d40(lVar2);
                      return 0;
                    }
                  }
                  *(unsigned int *)(puVar4 + 0x24) = 0;
                  *(unsigned char **)(puVar4 + 0x28) = puVar4 + 0x2c;
                  *(unsigned int *)(puVar4 + 0x50) = uStack_58;
                  *(unsigned int *)(puVar4 + 0x54) = uStack_54;
                  *(unsigned int *)(puVar4 + 0x58) = uStack_50;
                  *(unsigned int *)(puVar4 + 0x5c) = uStack_4c;
                  *(unsigned int *)(puVar4 + 0x60) = uStack_48;
                  *(unsigned int *)(puVar4 + 100) = uStack_44;
                  FUN_003c97e0(lVar2);
                  FUN_003c9e80(lVar2,0);
                  iVar1 = 0;
                  if (0 < *(int *)(puVar4 + 0x14)) {
                    iVar6 = 0;
                    do {
                      if (**(int **)(*(int *)(puVar4 + 0x10) + iVar6) != 0) {
                        FUN_003c42b0();
                      }
                      iVar1 = iVar1 + 1;
                      iVar6 = iVar6 + 4;
                    } while (iVar1 < *(int *)(puVar4 + 0x14));
                  }
                  FUN_003e3b70(0x70b060,lVar2);
                  DAT_00764794 = 0;
                  DAT_00764790 = 0;
                  lVar3 = FUN_003e3110(0x70b060,param_1,lVar2);
                  if (lVar3 != 0) {
                    if (DAT_00764794 != 0) {
                      FUN_003e32f0(0x70b060,DAT_00764794,lVar2,DAT_00764790);
                    }
                    lVar3 = FUN_003c9b30(lVar2);
                    if (lVar3 == 0) {
                      FUN_003c9d40(lVar2);
                      lVar2 = 0;
                    }
                  }
                  else {
                    FUN_003c9d40(lVar2);
                    lVar2 = 0;
                  }
                }
                else {
                  FUN_003c9d40(lVar2);
                  lVar2 = 0;
                }
              }
            }
            else {
              FUN_003c9d40(lVar2);
              lVar2 = 0;
            }
          }
          else {
            uStack_28 = 2;
            uStack_24 = FUN_003df590(0xffffffff80000013,iVar1);
            FUN_003df4d0(&uStack_28);
            lVar2 = 0;
          }
        }
        else {
          uStack_20 = 2;
          uStack_1c = FUN_003df590(0xffffffff80000004);
          FUN_003df4d0(&uStack_20);
          lVar2 = 0;
        }
      }
      else {
        lVar2 = 0;
      }
    }
  }
  else {
    lVar2 = 0;
  }
  return lVar2;
}
