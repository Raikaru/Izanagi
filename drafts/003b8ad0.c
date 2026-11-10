/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b8ad0 func_003b8ad0 src/promoted/code1_003b.c - romwright draft, unreviewed */

long FUN_003b8ad0(long param_1,unsigned long long param_2)

{
  unsigned int uVar1;
  unsigned int uVar2;
  int iVar3;
  long lVar4;
  unsigned long long uVar5;
  unsigned int *puVar6;
  int iVar7;
  int uStack_18;
  unsigned int uStack_14;
  int iStack_10;
  unsigned int uStack_c;
  int iStack_8;
  unsigned int uStack_4;
  
  lVar4 = FUN_003df050(param_1,1,&iStack_8,&uStack_4);
  if (lVar4 != 0) {
    if ((uStack_4 < 0x35000) || (0x37002 < uStack_4)) {
      uStack_18 = 0x116;
      uStack_14 = FUN_003df590(0xffffffff80000004);
      FUN_003df4d0(&uStack_18);
      param_1 = 0;
    }
    else {
      lVar4 = FUN_003df360(param_1,&iStack_10,4);
      if (lVar4 != 0) {
        if (iStack_10 != 4) {
          param_1 = 0;
        }
        else {
          uVar5 = (*DAT_008873f8)(DAT_008864a8,0x30116);
          FUN_0043f9c8(uVar5,0,0x48);
          lVar4 = FUN_003df360(param_1,&uStack_c,4);
          if (lVar4 == 0) {
            param_1 = 0;
          }
          else {
            puVar6 = (unsigned int *)uVar5;
            *puVar6 = uStack_c & 0xff;
            puVar6[1] = (uStack_c >> 8) & 0xff;
            puVar6[4] = (uStack_c >> 0x10) & 0xff;
            uVar1 = puVar6[1];
            iVar7 = *puVar6 * 0x40;
            iStack_8 = iVar7 + uVar1;
            uVar2 = (*DAT_008873e8)(iStack_8 + 0xf,0x30116);
            puVar6[0x11] = uVar2;
            puVar6[3] = (puVar6[0x11] + 0xf) & 0xfffffff0;
            puVar6[2] = puVar6[3] + *puVar6 * 0x40;
            uVar2 = FUN_003e2910(param_1,puVar6[2],uVar1);
            if (uVar1 != uVar2) {
              param_1 = 0;
            }
            else {
              iVar3 = FUN_003e2910(param_1,puVar6[3],iVar7);
              if (iVar7 != iVar3) {
                param_1 = 0;
              }
              else {
                lVar4 = FUN_003df360(param_1,puVar6 + 7,0x10);
                if (lVar4 != param_1) {
                  param_1 = 0;
                }
                else {
                  lVar4 = FUN_003b7290(param_1,uVar5);
                  if (lVar4 == 0) {
                    param_1 = 0;
                  }
                  else {
                    FUN_003b8410(param_2,uVar5);
                  }
                }
              }
            }
          }
        }
      }
      else {
        param_1 = 0;
      }
    }
  }
  else {
    param_1 = 0;
  }
  return param_1;
}
