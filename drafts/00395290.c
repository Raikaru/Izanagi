/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00395290 func_00395290 src/promoted/code1_0039.c - romwright draft, unreviewed */

long FUN_00395290(unsigned long long param_1)

{
  long lVar1;
  long lVar2;
  int iVar3;
  unsigned int uVar4;
  int iVar5;
  unsigned int uVar6;
  int uStack_28;
  unsigned int uStack_24;
  unsigned int uStack_1c;
  unsigned char auStack_18 [4];
  unsigned int uStack_14;
  unsigned char auStack_10 [4];
  unsigned int uStack_c;
  unsigned char auStack_8 [4];
  unsigned char auStack_4 [4];
  
  lVar1 = FUN_003df050(param_1,1,auStack_4,auStack_8);
  if (lVar1 == 0) {
    lVar1 = 0;
  }
  else {
    lVar1 = FUN_003df360(param_1,&uStack_c,4);
    if ((((lVar1 == 0) || (lVar1 = FUN_003df360(param_1,auStack_10,4), lVar1 == 0)) ||
        (lVar1 = FUN_003df360(param_1,&uStack_14,4), lVar1 == 0)) ||
       (lVar1 = FUN_003df360(param_1,auStack_18,4), lVar1 == 0)) {
      lVar1 = 0;
    }
    else if (uStack_c < 0x101) {
      lVar1 = FUN_00394fd0(uStack_14);
      if (lVar1 != 0) {
        uVar6 = 0;
        if (uStack_14 != 0) {
          iVar3 = 0;
          do {
            lVar2 = FUN_003df360(param_1,&uStack_1c,4);
            if (lVar2 == 0) {
              return 0;
            }
            if (uStack_1c != 0) {
              lVar2 = FUN_00395100(lVar1,uVar6);
              if (lVar2 == 0) {
                return 0;
              }
              iVar5 = *(int *)(*(int *)((int)lVar1 + 4) + iVar3 + 4);
              uVar4 = 0;
              if (uStack_1c != 0) {
                do {
                  lVar2 = FUN_003df300(param_1,iVar5,0x10);
                  if ((lVar2 == 0) || (lVar2 = FUN_003df360(param_1,iVar5 + 0x10,4), lVar2 == 0)) {
                    return 0;
                  }
                  uVar4 = uVar4 + 1;
                  iVar5 = iVar5 + 0x14;
                } while (uVar4 < uStack_1c);
              }
            }
            uVar6 = uVar6 + 1;
            iVar3 = iVar3 + 8;
          } while (uVar6 < uStack_14);
        }
      }
      else {
        lVar1 = 0;
      }
    }
    else {
      uStack_28 = 0x122;
      uStack_24 = FUN_003df590(0xffffffff80000004);
      FUN_003df4d0(&uStack_28);
      lVar1 = 0;
    }
  }
  return lVar1;
}
