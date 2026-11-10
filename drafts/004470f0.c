/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004470f0 func_004470f0 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

unsigned long long FUN_004470f0(long param_1)

{
  unsigned long long uVar1;
  unsigned long long uVar2;
  unsigned long long uVar3;
  unsigned long long uVar4;
  unsigned long long uVar5;
  unsigned long long uVar6;
  
  uVar5 = param_1 >> 0x20;
  uVar6 = uVar5 & 0x7fffffff;
  if (uVar6 >= 0x3ff00000) {
    if ((int)uVar6 == 0x3ff00000 && (int)param_1 == 0) {
      uVar1 = 0;
      if ((long)uVar5 < 1) {
        uVar1 = DAT_00755830;
      }
    }
    else {
      uVar1 = FUN_0044e100(param_1,param_1);
      uVar2 = FUN_0044e100(param_1,param_1);
      uVar1 = FUN_0044e3b8(uVar1,uVar2);
    }
  }
  else {
    if (uVar6 < 0x3fe00000) {
      if (uVar6 < 0x3c600001) {
        return DAT_00755838;
      }
      uVar1 = FUN_0044e168(param_1,param_1);
      uVar2 = FUN_0044e168(uVar1,DAT_00755840);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755848);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755850);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755858);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755860);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755868);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar3 = FUN_0044e168(uVar1,DAT_00755870);
      uVar3 = FUN_0044e0a8(uVar3,DAT_00755878);
      uVar3 = FUN_0044e168(uVar1,uVar3);
      uVar3 = FUN_0044e0a8(uVar3,DAT_00755880);
      uVar3 = FUN_0044e168(uVar1,uVar3);
      uVar3 = FUN_0044e0a8(uVar3,DAT_00755888);
      uVar1 = FUN_0044e168(uVar1,uVar3);
      uVar1 = FUN_0044e0a8(uVar1,0x3ff0000000000000);
      uVar1 = FUN_0044e3b8(uVar2,uVar1);
      uVar1 = FUN_0044e168(param_1,uVar1);
      uVar1 = FUN_0044e100(DAT_00755890,uVar1);
      uVar2 = FUN_0044e100(param_1,uVar1);
      uVar1 = DAT_00755838;
    }
    else {
      if (-1 < (long)uVar5) {
        uVar1 = FUN_0044e100(0x3ff0000000000000,param_1);
        uVar1 = FUN_0044e168(uVar1,0x3fe0000000000000);
        uVar5 = FUN_00448a00(uVar1);
        uVar6 = uVar5 & 0xffffffff00000000;
        uVar2 = FUN_0044e168(uVar6,uVar6);
        uVar2 = FUN_0044e100(uVar1,uVar2);
        uVar3 = FUN_0044e0a8(uVar5,uVar6);
        uVar2 = FUN_0044e3b8(uVar2,uVar3);
        uVar3 = FUN_0044e168(uVar1,DAT_00755840);
        uVar3 = FUN_0044e0a8(uVar3,DAT_00755848);
        uVar3 = FUN_0044e168(uVar1,uVar3);
        uVar3 = FUN_0044e0a8(uVar3,DAT_00755850);
        uVar3 = FUN_0044e168(uVar1,uVar3);
        uVar3 = FUN_0044e0a8(uVar3,DAT_00755858);
        uVar3 = FUN_0044e168(uVar1,uVar3);
        uVar3 = FUN_0044e0a8(uVar3,DAT_00755860);
        uVar3 = FUN_0044e168(uVar1,uVar3);
        uVar3 = FUN_0044e0a8(uVar3,DAT_00755868);
        uVar3 = FUN_0044e168(uVar1,uVar3);
        uVar4 = FUN_0044e168(uVar1,DAT_00755870);
        uVar4 = FUN_0044e0a8(uVar4,DAT_00755878);
        uVar4 = FUN_0044e168(uVar1,uVar4);
        uVar4 = FUN_0044e0a8(uVar4,DAT_00755880);
        uVar4 = FUN_0044e168(uVar1,uVar4);
        uVar4 = FUN_0044e0a8(uVar4,DAT_00755888);
        uVar1 = FUN_0044e168(uVar1,uVar4);
        uVar1 = FUN_0044e0a8(uVar1,0x3ff0000000000000);
        uVar1 = FUN_0044e3b8(uVar3,uVar1);
        uVar1 = FUN_0044e168(uVar1,uVar5);
        uVar1 = FUN_0044e0a8(uVar1,uVar2);
        uVar1 = FUN_0044e0a8(uVar6,uVar1);
        uVar1 = FUN_0044e0a8(uVar1,uVar1);
        return uVar1;
      }
      uVar1 = FUN_0044e0a8(param_1,0x3ff0000000000000);
      uVar1 = FUN_0044e168(uVar1,0x3fe0000000000000);
      uVar2 = FUN_0044e168(uVar1,DAT_00755840);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755848);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755850);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755858);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755860);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,DAT_00755868);
      uVar2 = FUN_0044e168(uVar1,uVar2);
      uVar3 = FUN_0044e168(uVar1,DAT_00755870);
      uVar3 = FUN_0044e0a8(uVar3,DAT_00755878);
      uVar3 = FUN_0044e168(uVar1,uVar3);
      uVar3 = FUN_0044e0a8(uVar3,DAT_00755880);
      uVar3 = FUN_0044e168(uVar1,uVar3);
      uVar3 = FUN_0044e0a8(uVar3,DAT_00755888);
      uVar3 = FUN_0044e168(uVar1,uVar3);
      uVar3 = FUN_0044e0a8(uVar3,0x3ff0000000000000);
      uVar1 = FUN_00448a00(uVar1);
      uVar2 = FUN_0044e3b8(uVar2,uVar3);
      uVar2 = FUN_0044e168(uVar2,uVar1);
      uVar2 = FUN_0044e100(uVar2,DAT_00755890);
      uVar1 = FUN_0044e0a8(uVar1,uVar2);
      uVar2 = FUN_0044e0a8(uVar1,uVar1);
      uVar1 = DAT_00755830;
    }
    uVar1 = FUN_0044e100(uVar1,uVar2);
  }
  return uVar1;
}
