/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003b7290 func_003b7290 src/promoted/code1_003b.c - romwright draft, unreviewed */

unsigned long long FUN_003b7290(unsigned long long param_1,long param_2)

{
  int iVar1;
  int iVar2;
  long lVar3;
  int iVar4;
  int *piVar5;
  int iStack_c;
  int iStack_8;
  int iStack_4;
  
  piVar5 = (int *)param_2;
  lVar3 = FUN_003df360(param_1,&iStack_c,4);
  if (lVar3 == 0) {
    param_1 = 0;
  }
  else {
    lVar3 = FUN_003df360(param_1,&iStack_4,4);
    if (lVar3 == 0) {
      param_1 = 0;
    }
    else {
      lVar3 = FUN_003df360(param_1,&iStack_8,4);
      if (lVar3 != 0) {
        if (0 < iStack_4) {
          iVar1 = *piVar5;
          FUN_003b7150(param_2);
          iVar4 = iVar1 + iStack_4 * 2 + iStack_8 * 2;
          iVar2 = (*DAT_008873e8)(iVar4,0x30116);
          piVar5[0xe] = iVar2;
          if (piVar5[0xe] != 0) {
            FUN_0043f9c8(piVar5[0xe],0,iVar4);
            piVar5[0xb] = iStack_c;
            piVar5[0xc] = iStack_4;
            piVar5[0xd] = iStack_8;
            piVar5[0xf] = piVar5[0xe] + iVar1;
            piVar5[0x10] = piVar5[0xf] + iStack_4 * 2;
          }
          else {
            param_2 = 0;
          }
          if (param_2 != 0) {
            lVar3 = FUN_003e2910(param_1,piVar5[0xe],*piVar5 + piVar5[0xc] * 2 + piVar5[0xd] * 2);
            if (lVar3 == 0) {
              (*DAT_008873ec)(piVar5 + 0xb);
              param_1 = 0;
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
    }
  }
  return param_1;
}
