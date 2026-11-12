/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dddf0 func_003dddf0 src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned long long FUN_003dddf0(unsigned long long param_1,unsigned long long param_2)

{
  unsigned char bVar1;
  long lVar2;
  long lVar3;
  unsigned long long uVar4;
  unsigned long long uVar5;
  unsigned long long uVar6;
  
  uVar5 = 0;
  lVar2 = FUN_00443010(param_2,0x763c2c);
  bVar1 = lVar2 != 0;
  lVar2 = FUN_00443010(param_2,0x763c30);
  lVar3 = FUN_00443010(param_2,0x763c34);
  if ((lVar3 != 0) && (uVar5 = 1, bVar1)) {
    uVar5 = 0xb;
  }
  lVar3 = FUN_00443010(param_2,0x763c38);
  uVar6 = uVar5;
  if ((lVar3 != 0) && (uVar6 = uVar5 | 6, bVar1)) {
    uVar6 = uVar5 | 7;
  }
  lVar3 = FUN_00443010(param_2,0x763c3c);
  if (lVar3 != 0) {
    uVar5 = uVar6 | 10;
    if (bVar1) {
      uVar5 = uVar6 | 0xb;
    }
    uVar6 = uVar5;
    if (lVar2 != 0) {
      uVar6 = uVar6 | 4;
    }
  }
  lVar2 = FUN_003ddca0(param_1);
  if (lVar2 != 0) {
    uVar4 = FUN_003dda50(lVar2,param_1,uVar6,0,0);
  }
  else {
    uVar4 = 0;
  }
  return uVar4;
}
