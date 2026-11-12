/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c2cf0 func_003c2cf0 src/promoted/code1_003c.c - romwright draft, unreviewed */

int FUN_003c2cf0(int param_1,unsigned long long param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  long lVar4;
  unsigned long long uVar5;
  unsigned short *puVar6;
  int uStack_40;
  unsigned int uStack_3c;
  unsigned int uStack_38;
  unsigned int uStack_34;
  unsigned int uStack_30;
  unsigned int uStack_2c;
  unsigned int uStack_20;
  unsigned int uStack_1c;
  unsigned int uStack_18;
  unsigned int uStack_14;
  int uStack_8;
  unsigned int uStack_4;
  
  iVar1 = FUN_003c2bd0();
  iVar2 = FUN_003c4c00(param_1 + 0x20);
  iVar3 = FUN_003e3370(0x70afb0,param_1);
  lVar4 = FUN_003deff0(param_2,0xf,iVar1 + iVar2 + iVar3 + 0x24,0x37002,0x37);
  if (lVar4 == 0) {
    param_1 = 0;
  }
  else {
    uVar5 = FUN_003c2bd0(param_1);
    lVar4 = FUN_003deff0(param_2,1,uVar5,0x37002,0x37);
    if (lVar4 == 0) {
      param_1 = 0;
    }
    else {
      uStack_1c = *(unsigned int *)(param_1 + 0x10);
      uStack_18 = *(unsigned int *)(param_1 + 0x14);
      uStack_14 = *(unsigned int *)(param_1 + 0x18);
      uStack_20 = *(unsigned int *)(param_1 + 8) | ((unsigned int)*(unsigned char *)(param_1 + 0x1c) << 0x10);
      lVar4 = FUN_003e2ab0(param_2,&uStack_20,0x10);
      if (lVar4 != 0) {
        if (((*(unsigned int *)(param_1 + 8) & 0x1000000) == 0) && (*(int *)(param_1 + 0x14) != 0)) {
          if (((*(unsigned int *)(param_1 + 8) & 8) != 0) &&
             (lVar4 = FUN_003e2ab0(param_2,*(unsigned int *)(param_1 + 0x30),
                                   *(int *)(param_1 + 0x14) << 2), lVar4 == 0)) {
            return 0;
          }
          if (0 < *(int *)(param_1 + 0x1c)) {
            iVar1 = *(int *)(param_1 + 0x14);
            iVar3 = 0;
            iVar2 = param_1;
            if (0 < *(int *)(param_1 + 0x1c)) {
              do {
                lVar4 = FUN_003df1a0(param_2,*(unsigned int *)(iVar2 + 0x34),iVar1 << 3);
                if (lVar4 == 0) {
                  return 0;
                }
                iVar3 = iVar3 + 1;
                iVar2 = iVar2 + 4;
              } while (iVar3 < *(int *)(param_1 + 0x1c));
            }
          }
          iVar1 = *(int *)(param_1 + 0x10);
          if (iVar1 != 0) {
            puVar6 = *(unsigned short **)(param_1 + 0x2c);
            while (iVar1 != 0) {
              iVar1 = iVar1 - 1;
              uStack_8 = CONCAT22(*puVar6,puVar6[1]);
              uStack_4 = CONCAT22(puVar6[2],puVar6[3]);
              puVar6 = puVar6 + 4;
              lVar4 = FUN_003e2ab0(param_2,&uStack_8,8);
              if (lVar4 == 0) {
                return 0;
              }
            }
          }
        }
        iVar1 = 0;
        if (0 < *(int *)(param_1 + 0x18)) {
          iVar2 = 0;
          do {
            iVar3 = *(int *)(param_1 + 0x5c) + iVar2;
            uStack_40 = *(unsigned int *)(iVar3 + 4);
            uStack_3c = *(unsigned int *)(iVar3 + 8);
            uStack_38 = *(unsigned int *)(iVar3 + 0xc);
            uStack_34 = *(unsigned int *)(iVar3 + 0x10);
            if ((*(unsigned int *)(param_1 + 8) & 0x1000000) == 0) {
              uStack_30 = (unsigned int)(*(int *)(iVar3 + 0x14) != 0);
              uStack_2c = (unsigned int)(*(int *)(iVar3 + 0x18) != 0);
            }
            else {
              uStack_30 = 0;
              uStack_2c = 0;
            }
            lVar4 = FUN_003e2ab0(param_2,&uStack_40,0x18);
            if (lVar4 == 0) {
              return 0;
            }
            if ((uStack_30 != 0) &&
               (lVar4 = FUN_003df1a0(param_2,*(unsigned int *)
                                              (*(int *)(param_1 + 0x5c) + iVar2 + 0x14),
                                     *(int *)(param_1 + 0x14) * 0xc), lVar4 == 0)) {
              return 0;
            }
            if ((uStack_2c != 0) &&
               (lVar4 = FUN_003df1a0(param_2,*(unsigned int *)
                                              (*(int *)(param_1 + 0x5c) + iVar2 + 0x18),
                                     *(int *)(param_1 + 0x14) * 0xc), lVar4 == 0)) {
              return 0;
            }
            iVar1 = iVar1 + 1;
            iVar2 = iVar2 + 0x1c;
          } while (iVar1 < *(int *)(param_1 + 0x18));
        }
        lVar4 = FUN_003c4cf0(param_1 + 0x20,param_2);
        if (lVar4 == 0) {
          param_1 = 0;
        }
        else {
          lVar4 = FUN_003e33f0(0x70afb0,param_2,param_1);
          if (lVar4 == 0) {
            param_1 = 0;
          }
        }
      }
      else {
        param_1 = 0;
      }
    }
  }
  return param_1;
}
