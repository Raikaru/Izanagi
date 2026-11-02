/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d5600 func_003d5600 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned int FUN_003d5600(unsigned long long param_1,unsigned long long param_2)

{
  unsigned int uVar1;
  unsigned long long uVar2;
  long lVar3;
  int *piVar4;
  int uStack_4;
  
  uStack_4 = 0x100;
  uVar2 = FUN_003d5710();
  FUN_003deff0(param_2,0x1b,uVar2,0x37002,0x37);
  lVar3 = FUN_003df240(param_2,&uStack_4,4);
  if (lVar3 == 0) {
    uVar1 = 0;
  }
  else {
    piVar4 = (int *)param_1;
    lVar3 = FUN_003df240(param_2,*piVar4,4);
    if (lVar3 == 0) {
      uVar1 = 0;
    }
    else {
      lVar3 = FUN_003df240(param_2,piVar4 + 1,4);
      if (lVar3 == 0) {
        uVar1 = 0;
      }
      else {
        lVar3 = FUN_003df240(param_2,piVar4 + 2,4);
        if (lVar3 == 0) {
          uVar1 = 0;
        }
        else {
          lVar3 = FUN_003df1a0(param_2,piVar4 + 3,4);
          if (lVar3 == 0) {
            uVar1 = 0;
          }
          else {
            (**(code **)(*piVar4 + 0x24))(param_1,param_2);
            uVar1 = 1;
          }
        }
      }
    }
  }
  return uVar1;
}
