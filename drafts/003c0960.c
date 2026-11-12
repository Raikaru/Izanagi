/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c0960 func_003c0960 src/promoted/code1_003c.c - romwright draft, unreviewed */
typedef struct fn_003c0960_arg0 fn_003c0960_arg0;
struct fn_003c0960_arg0 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x4];
    int field_0x10; /* +0x10 */
    unsigned char pad_0x14[0x4];
    int field_0x18; /* +0x18 */
};


long FUN_003c0960(fn_003c0960_arg0 *param_1,unsigned long long param_2)

{
  unsigned int *puVar1;
  unsigned int *puVar2;
  int iVar3;
  long lVar4;
  long lVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int uStack_50;
  unsigned char auStack_4c [8];
  int iStack_44;
  int iStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  unsigned char auStack_30 [8];
  int uStack_28;
  int iStack_24;
  int iStack_20;
  unsigned char auStack_18 [4];
  unsigned char auStack_14 [4];
  int iStack_10;
  int iStack_c;
  int uStack_8;
  int iStack_4;
  
  lVar5 = (long)(int)param_1;
  iStack_4 = 0x18;
  lVar4 = FUN_003e5830(auStack_30,param_1->field_0x4);
  if (lVar4 == 0) {
    iVar8 = 0;
  }
  else {
    iVar8 = FUN_003e5a30(auStack_30);
    iStack_4 = iStack_4 + iVar8 + 0xc;
    FUN_003e59e0(auStack_30);
    iStack_34 = 0;
    lVar4 = FUN_003bebb0(&iStack_38,lVar5);
    if (lVar4 == 0) {
      iVar8 = 0;
    }
    else {
      iVar7 = 0x10;
      iVar8 = 0;
      if (0 < iStack_34) {
        iVar6 = 0;
        do {
          iVar3 = FUN_003c2c90(*(unsigned int *)(iStack_38 + iVar6));
          iVar8 = iVar8 + 1;
          iVar7 = iVar7 + iVar3 + 0xc;
          iVar6 = iVar6 + 4;
        } while (iVar8 < iStack_34);
      }
      iStack_4 = iStack_4 + iVar7 + 0xc;
      FUN_003beb10(&iStack_38);
      FUN_003bff30(lVar5,0x3bf360,&iStack_4);
      FUN_003c0050(lVar5,0x3bf280,&iStack_4);
      FUN_003bffc0(lVar5,0x3bf2d0,&iStack_4);
      iVar8 = FUN_003e3370(0x70af90,lVar5);
      iVar8 = iStack_4 + iVar8 + 0xc;
      iStack_4 = iVar8;
    }
  }
  lVar4 = FUN_003deff0(param_2,0x10,iVar8,0x37002,0x37);
  if (lVar4 == 0) {
    lVar5 = 0;
  }
  else {
    lVar4 = FUN_003deff0(param_2,1,0xc,0x37002,0x37);
    if (lVar4 == 0) {
      lVar5 = 0;
    }
    else {
      uStack_8 = 0;
      FUN_003bff30(lVar5,0x3be940,&uStack_8);
      iStack_c = 0;
      uStack_28 = uStack_8;
      FUN_003c0050(lVar5,0x3be960,&iStack_c);
      iStack_10 = 0;
      iStack_24 = iStack_c;
      FUN_003bffc0(lVar5,0x3be980,&iStack_10);
      iStack_20 = iStack_10;
      lVar4 = FUN_003e2ab0(param_2,&uStack_28,0xc);
      if (lVar4 == 0) {
        lVar5 = 0;
      }
      else {
        lVar4 = FUN_003e5830(auStack_4c,param_1->field_0x4);
        if (lVar4 == 0) {
          lVar5 = 0;
        }
        else {
          lVar4 = FUN_003e5ae0(auStack_4c,param_2);
          if (lVar4 != 0) {
            iStack_40 = 0;
            lVar4 = FUN_003bebb0(&iStack_44,lVar5);
            if (lVar4 == 0) {
              lVar5 = 0;
            }
            else {
              lVar4 = FUN_003bed10(&iStack_44,param_2);
              if (lVar4 == 0) {
                iVar8 = 0;
                if (0 < iStack_40) {
                  iVar7 = 0;
                  do {
                    FUN_003c2a80(*(unsigned int *)(iStack_44 + iVar7));
                    iVar8 = iVar8 + 1;
                    iVar7 = iVar7 + 4;
                  } while (iVar8 < iStack_40);
                }
                if (iStack_44 != 0) {
                  (*DAT_008873ec)();
                }
                lVar5 = 0;
              }
              else {
                uStack_50 = (unsigned int)param_2;
                iStack_3c = 1;
                puVar2 = (unsigned int *)param_1->field_0x8;
                do {
                  if (puVar2 == &param_1->field_0x8) break;
                  puVar1 = (unsigned int *)*puVar2;
                  lVar4 = FUN_003bf3d0(puVar2 + -0x10,&uStack_50);
                  puVar2 = puVar1;
                } while (lVar4 != 0);
                if ((0 < iStack_24) && (iStack_3c != 0)) {
                  puVar2 = (unsigned int *)param_1->field_0x10;
                  do {
                    if (puVar2 == &param_1->field_0x10) break;
                    puVar1 = (unsigned int *)*puVar2;
                    iVar8 = (int)puVar2 + (-4 - DAT_007647a4);
                    lVar4 = FUN_003e5990(auStack_4c,*(unsigned int *)(iVar8 + 4),auStack_14);
                    if ((((lVar4 == 0) ||
                         (lVar4 = FUN_003deff0(uStack_50,1,4,0x37002,0x37), lVar4 == 0)) ||
                        (lVar4 = FUN_003df240(uStack_50,auStack_14,4), lVar4 == 0)) ||
                       (lVar4 = FUN_003c39c0(iVar8,uStack_50), lVar4 == 0)) {
                      iStack_3c = 0;
                      iVar8 = 0;
                    }
                    puVar2 = puVar1;
                  } while (iVar8 != 0);
                }
                if ((0 < iStack_20) && (iStack_3c != 0)) {
                  puVar2 = (unsigned int *)param_1->field_0x18;
                  do {
                    if (puVar2 == &param_1->field_0x18) break;
                    puVar1 = (unsigned int *)*puVar2;
                    iVar8 = (int)puVar2 + (-4 - DAT_007647a0);
                    lVar4 = FUN_003e5990(auStack_4c,*(unsigned int *)(iVar8 + 4),auStack_18);
                    if (((lVar4 == 0) ||
                        (lVar4 = FUN_003deff0(uStack_50,1,4,0x37002,0x37), lVar4 == 0)) ||
                       ((lVar4 = FUN_003df240(uStack_50,auStack_18,4), lVar4 == 0 ||
                        (lVar4 = FUN_003e5290(iVar8,uStack_50), lVar4 == 0)))) {
                      iStack_3c = 0;
                      iVar8 = 0;
                    }
                    puVar2 = puVar1;
                  } while (iVar8 != 0);
                }
                iVar8 = 0;
                if (0 < iStack_40) {
                  iVar7 = 0;
                  do {
                    FUN_003c2a80(*(unsigned int *)(iStack_44 + iVar7));
                    iVar8 = iVar8 + 1;
                    iVar7 = iVar7 + 4;
                  } while (iVar8 < iStack_40);
                }
                if (iStack_44 != 0) {
                  (*DAT_008873ec)();
                  iStack_44 = 0;
                }
                FUN_003e59e0(auStack_4c);
                if (iStack_3c == 0) {
                  lVar5 = 0;
                }
                else {
                  lVar4 = FUN_003e33f0(0x70af90,param_2,lVar5);
                  if (lVar4 == 0) {
                    lVar5 = 0;
                  }
                }
              }
            }
          }
          else {
            FUN_003e59e0(auStack_4c);
            lVar5 = 0;
          }
        }
      }
    }
  }
  return lVar5;
}
