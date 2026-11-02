/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00447d00 func_00447d00 src/middleware/middleware_inferred_00446108.c - romwright draft, unreviewed */

unsigned long long FUN_00447d00(unsigned long long param_1,unsigned long long param_2)

{
  unsigned long long uVar1;
  unsigned long long uVar2;
  unsigned long long uVar3;
  unsigned long long uVar4;
  long lVar5;
  unsigned long long uVar6;
  unsigned int uVar7;
  unsigned int uVar8;
  int iVar9;
  unsigned long long uVar10;
  unsigned long long uVar11;
  unsigned long long uVar12;
  unsigned long long uVar13;
  unsigned long long uVar14;
  int iVar15;
  unsigned long long uStack_80;
  int iStack_68;
  int iStack_64;
  int iStack_60;
  
  uVar7 = (unsigned int)param_2;
  uVar12 = (long)param_2 >> 0x20;
  uVar10 = uVar12 & 0x7fffffff;
  uVar13 = (long)param_1 >> 0x20;
  iVar15 = (int)param_1;
  uVar14 = uVar13 & 0x7fffffff;
  if (uVar10 == 0 && uVar7 == 0) {
    return 0x3ff0000000000000;
  }
  if (((0x7ff00000 < uVar14) || (((uVar14 == 0x7ff00000 && (iVar15 != 0)) || (0x7ff00000 < uVar10)))
      ) || ((uVar10 == 0x7ff00000 && (uVar7 != 0)))) {
    uVar10 = FUN_0044e0a8(param_1,param_2);
    return uVar10;
  }
  iStack_64 = 0;
  if ((long)uVar13 < 0) {
    if (0x433fffff < uVar10) {
      iStack_64 = 2;
    }
    else if (uVar10 >= 0x3ff00000) {
      iVar9 = ((int)uVar10 >> 0x14) - 0x3ff;
      if (0x14 < iVar9) {
        uVar8 = uVar7 >> ((0x34U - iVar9) & 0x1f);
        if (uVar8 << ((0x34U - iVar9) & 0x1f) == uVar7) {
          iStack_64 = 2 - (uVar8 & 1);
        }
      }
      else {
        if (uVar7 != 0) goto LAB_00447f08;
        uVar8 = (int)uVar10 >> ((0x14U - iVar9) & 0x1f);
        if ((long)(int)(uVar8 << ((0x14U - iVar9) & 0x1f)) == uVar10) {
          iStack_64 = 2 - (uVar8 & 1);
        }
      }
    }
  }
  if (uVar7 == 0) {
    if (uVar10 == 0x7ff00000) {
      uVar10 = param_2;
      if ((int)uVar14 != 0x3ff00000 || iVar15 != 0) {
        if (0x3fefffff < uVar14) {
          if ((long)uVar12 >= 0) {
            return param_2;
          }
          return 0;
        }
        if (-1 < (long)uVar12) {
          return 0;
        }
        uVar10 = 0;
      }
      uVar10 = FUN_0044e100(uVar10,param_2);
      return uVar10;
    }
    if (uVar10 == 0x3ff00000) {
      if (-1 < (long)uVar12) {
        return param_1;
      }
      uVar10 = 0x3ff0000000000000;
      uVar10 = FUN_0044e3b8(uVar10,param_1);
      return uVar10;
    }
    uVar1 = param_1;
    if (uVar12 == 0x40000000) {
      uVar10 = FUN_0044e168(param_1,uVar1);
      return uVar10;
    }
    if ((uVar12 == 0x3fe00000) && (-1 < (long)uVar13)) {
      uVar10 = FUN_00448a00(param_1);
      return uVar10;
    }
  }
LAB_00447f08:
  uVar1 = FUN_0044b310(param_1);
  if ((iVar15 == 0) && (((uVar14 == 0x7ff00000 || (uVar14 == 0)) || (uVar14 == 0x3ff00000)))) {
    if ((long)uVar12 < 0) {
      uVar1 = FUN_0044e3b8(0x3ff0000000000000,uVar1);
    }
    if (-1 < (long)uVar13) {
      return uVar1;
    }
    if ((int)uVar14 == 0x3ff00000 && iStack_64 == 0) {
      uVar2 = FUN_0044e100(uVar1,uVar1);
      uVar10 = FUN_0044e3b8(uVar2,uVar2);
      return uVar10;
    }
    if (iStack_64 != 1) {
      return uVar1;
    }
    uVar10 = FUN_0044e100(0,uVar1);
    return uVar10;
  }
  if ((long)param_1 < 0 && iStack_64 == 0) {
    uVar10 = FUN_0044e100(param_1,param_1);
    param_1 = uVar10;
    uVar10 = FUN_0044e3b8(uVar10,param_1);
    return uVar10;
  }
  if (uVar10 >= 0x41e00001) {
    if (0x43f00000 < uVar10) {
      if (0x3fefffff >= uVar14) {
        if (-1 >= (long)uVar12) {
          return DAT_00755a98;
        }
        return 0;
      }
      if ((long)uVar12 >= 1) {
        return DAT_00755a98;
      }
      return 0;
    }
    if (uVar14 < 0x3fefffff) {
      if (-1 >= (long)uVar12) {
        return DAT_00755a98;
      }
      return 0;
    }
    if (0x3ff00000 < uVar14) {
      if ((long)uVar12 >= 1) {
        return DAT_00755a98;
      }
      return 0;
    }
    uVar2 = FUN_0044e100(param_1,0x3ff0000000000000);
    uVar3 = FUN_0044e168(uVar2,uVar2);
    uVar4 = FUN_0044e168(uVar2,0x3fd0000000000000);
    uVar4 = FUN_0044e100(DAT_00755aa0,uVar4);
    uVar4 = FUN_0044e168(uVar2,uVar4);
    uVar4 = FUN_0044e100(0x3fe0000000000000,uVar4);
    uVar3 = FUN_0044e168(uVar3,uVar4);
    uVar4 = FUN_0044e168(uVar2,DAT_00755aa8);
    uVar2 = FUN_0044e168(uVar2,DAT_00755ab0);
    uVar3 = FUN_0044e168(uVar3,DAT_00755ab8);
    uVar2 = FUN_0044e100(uVar2,uVar3);
    uVar10 = FUN_0044e0a8(uVar4,uVar2);
    uVar10 = uVar10 & 0xffffffff00000000;
    uVar3 = FUN_0044e100(uVar10,uVar4);
  }
  else {
    iStack_60 = 0;
    if (uVar14 < 0x100000) {
      iStack_60 = -0x35;
      uVar1 = FUN_0044e168(uVar1,0x4340000000000000);
      uVar14 = (long)uVar1 >> 0x20;
    }
    iStack_68 = 0;
    iVar15 = iStack_60 + ((int)uVar14 >> 0x14);
    uVar14 = uVar14 & 0xfffff;
    iStack_60 = iVar15 - 0x3ff;
    uVar10 = uVar14 | 0x3ff00000;
    if ((0x3988e < uVar14) && (iStack_68 = 1, 0xbb679 < uVar14)) {
      iStack_60 = iVar15 - 0x3fe;
      iStack_68 = 0;
      uVar10 = (unsigned long long)((int)uVar10 - 0x100000);
    }
    iVar15 = iStack_68 * 8;
    uVar13 = (uVar1 & 0xffffffff) | (uVar10 << 0x20);
    uVar4 = *(unsigned long long *)(iVar15 + 0x755990);
    uVar2 = FUN_0044e100(uVar13,uVar4);
    uVar3 = FUN_0044e0a8(uVar13,uVar4);
    uVar3 = FUN_0044e3b8(0x3ff0000000000000,uVar3);
    uVar12 = FUN_0044e168(uVar2,uVar3);
    uVar14 = uVar12 & 0xffffffff00000000;
    lVar5 = (long)(int)((((int)uVar10 >> 1) | 0x20000000U) + iStack_68 * 0x40000 + 0x80000) << 0x20;
    uVar4 = FUN_0044e100(lVar5,uVar4);
    uVar4 = FUN_0044e100(uVar13,uVar4);
    uVar6 = FUN_0044e168(uVar14,lVar5);
    uVar2 = FUN_0044e100(uVar2,uVar6);
    uVar4 = FUN_0044e168(uVar14,uVar4);
    uVar2 = FUN_0044e100(uVar2,uVar4);
    uVar2 = FUN_0044e168(uVar3,uVar2);
    uVar3 = FUN_0044e168(uVar12,uVar12);
    uVar4 = FUN_0044e168(uVar3,uVar3);
    uVar6 = FUN_0044e168(uVar3,DAT_00755ac0);
    uVar6 = FUN_0044e0a8(uVar6,DAT_00755ac8);
    uVar6 = FUN_0044e168(uVar3,uVar6);
    uVar6 = FUN_0044e0a8(uVar6,DAT_00755ad0);
    uVar6 = FUN_0044e168(uVar3,uVar6);
    uVar6 = FUN_0044e0a8(uVar6,DAT_00755ad8);
    uVar6 = FUN_0044e168(uVar3,uVar6);
    uVar6 = FUN_0044e0a8(uVar6,DAT_00755ae0);
    uVar3 = FUN_0044e168(uVar3,uVar6);
    uVar3 = FUN_0044e0a8(uVar3,DAT_00755ae8);
    uVar3 = FUN_0044e168(uVar4,uVar3);
    uVar4 = FUN_0044e0a8(uVar14,uVar12);
    uVar4 = FUN_0044e168(uVar2,uVar4);
    uVar3 = FUN_0044e0a8(uVar3,uVar4);
    uVar4 = FUN_0044e168(uVar14,uVar14);
    uVar6 = FUN_0044e0a8(uVar4,0x4008000000000000);
    uVar10 = FUN_0044e0a8(uVar6,uVar3);
    uVar10 = uVar10 & 0xffffffff00000000;
    uVar6 = FUN_0044e100(uVar10,0x4008000000000000);
    uVar4 = FUN_0044e100(uVar6,uVar4);
    uVar3 = FUN_0044e100(uVar3,uVar4);
    uVar4 = FUN_0044e168(uVar14,uVar10);
    uVar2 = FUN_0044e168(uVar2,uVar10);
    uVar3 = FUN_0044e168(uVar3,uVar12);
    uVar2 = FUN_0044e0a8(uVar2,uVar3);
    uVar10 = FUN_0044e0a8(uVar4,uVar2);
    uVar10 = uVar10 & 0xffffffff00000000;
    uVar3 = FUN_0044e100(uVar10,uVar4);
    uVar2 = FUN_0044e100(uVar2,uVar3);
    uVar3 = FUN_0044e168(uVar10,DAT_00755af0);
    uVar4 = FUN_0044e168(uVar10,DAT_00755af8);
    uVar2 = FUN_0044e168(uVar2,DAT_00755b00);
    uVar2 = FUN_0044e0a8(uVar4,uVar2);
    uVar2 = FUN_0044e0a8(uVar2,*(unsigned long long *)(iVar15 + 0x7559b0));
    uVar4 = FUN_0044e658(iStack_60);
    uVar6 = FUN_0044e0a8(uVar3,uVar2);
    uVar11 = *(unsigned long long *)(iVar15 + 0x7559a0);
    uVar6 = FUN_0044e0a8(uVar6,uVar11);
    uVar10 = FUN_0044e0a8(uVar6,uVar4);
    uVar10 = uVar10 & 0xffffffff00000000;
    uVar4 = FUN_0044e100(uVar10,uVar4);
    uVar4 = FUN_0044e100(uVar4,uVar11);
    uVar3 = FUN_0044e100(uVar4,uVar3);
  }
  uVar2 = FUN_0044e100(uVar2,uVar3);
  uStack_80 = 0x3ff0000000000000;
  if ((long)param_1 < 0 && iStack_64 == 1) {
    uStack_80 = 0xbff0000000000000;
  }
  uVar3 = FUN_0044e100(param_2,param_2 & 0xffffffff00000000);
  uVar3 = FUN_0044e168(uVar3,uVar10);
  uVar2 = FUN_0044e168(param_2,uVar2);
  uVar2 = FUN_0044e0a8(uVar3,uVar2);
  uVar3 = FUN_0044e168(param_2 & 0xffffffff00000000,uVar10);
  lVar5 = FUN_0044e0a8(uVar2,uVar3);
  uVar10 = lVar5 >> 0x20;
  iVar15 = (int)((unsigned long long)lVar5 >> 0x20);
  if ((long)uVar10 >= 0x40900000) {
    uVar1 = DAT_00755b08;
    if (iVar15 == 0x40900000 && (int)lVar5 == 0) {
      uVar4 = FUN_0044e0a8(uVar2,DAT_00755b10);
      uVar6 = FUN_0044e100(lVar5,uVar3);
      lVar5 = FUN_0044e608(uVar6,uVar4);
      uVar1 = DAT_00755b08;
      if (-1 < lVar5) goto LAB_00448268;
    }
  }
  else {
    if ((uVar10 & 0x7fffffff) < 0x4090cc00) {
LAB_00448268:
      iStack_60 = 0;
      if (0x3fe00000 < (uVar10 & 0x7fffffff)) {
        uVar7 = iVar15 + (0x100000 >> ((((int)(uVar10 & 0x7fffffff) >> 0x14) - 0x3feU) & 0x1f));
        uVar8 = ((int)(uVar7 & 0x7fffffff) >> 0x14) - 0x3ff;
        iStack_60 = (int)((uVar7 & 0xfffff) | 0x100000) >> ((0x14 - uVar8) & 0x1f);
        if ((long)uVar10 < 0) {
          iStack_60 = -iStack_60;
        }
        uVar3 = FUN_0044e100(uVar3,((long)(int)uVar7 & ~(long)(0xfffff >> (uVar8 & 0x1f))) << 0x20);
      }
      uVar10 = FUN_0044e0a8(uVar2,uVar3);
      uVar10 = uVar10 & 0xffffffff00000000;
      uVar4 = FUN_0044e168(uVar10,0x3fe62e4300000000);
      uVar3 = FUN_0044e100(uVar10,uVar3);
      uVar2 = FUN_0044e100(uVar2,uVar3);
      uVar2 = FUN_0044e168(uVar2,DAT_00755b20);
      uVar3 = FUN_0044e168(uVar10,DAT_00755b28);
      uVar2 = FUN_0044e0a8(uVar2,uVar3);
      uVar3 = FUN_0044e0a8(uVar4,uVar2);
      uVar4 = FUN_0044e100(uVar3,uVar4);
      uVar2 = FUN_0044e100(uVar2,uVar4);
      uVar4 = FUN_0044e168(uVar3,uVar3);
      uVar6 = FUN_0044e168(uVar4,DAT_00755b30);
      uVar6 = FUN_0044e0a8(uVar6,DAT_00755b38);
      uVar6 = FUN_0044e168(uVar4,uVar6);
      uVar6 = FUN_0044e0a8(uVar6,DAT_00755b40);
      uVar6 = FUN_0044e168(uVar4,uVar6);
      uVar6 = FUN_0044e0a8(uVar6,DAT_00755b48);
      uVar6 = FUN_0044e168(uVar4,uVar6);
      uVar6 = FUN_0044e0a8(uVar6,DAT_00755b50);
      uVar4 = FUN_0044e168(uVar4,uVar6);
      uVar4 = FUN_0044e100(uVar3,uVar4);
      uVar6 = FUN_0044e168(uVar3,uVar4);
      uVar4 = FUN_0044e100(uVar4,0x4000000000000000);
      uVar4 = FUN_0044e3b8(uVar6,uVar4);
      uVar6 = FUN_0044e168(uVar3,uVar2);
      uVar2 = FUN_0044e0a8(uVar2,uVar6);
      uVar2 = FUN_0044e100(uVar4,uVar2);
      uVar2 = FUN_0044e100(uVar2,uVar3);
      uVar10 = FUN_0044e100(0x3ff0000000000000,uVar2);
      uVar7 = (int)(uVar10 >> 0x20) + iStack_60 * 0x100000;
      param_1 = uStack_80;
      if ((int)uVar7 >> 0x14 < 1) {
        uVar1 = FUN_00441d70(uVar10,iStack_60);
      }
      else {
        uVar1 = (uVar10 & 0xffffffff) | ((unsigned long long)uVar7 << 0x20);
      }
      uVar10 = FUN_0044e168(param_1,uVar1);
      return uVar10;
    }
    uVar1 = DAT_00755b18;
    if (iVar15 == -0x3f6f3400 && (int)lVar5 == 0) {
      uVar4 = FUN_0044e100(lVar5,uVar3);
      lVar5 = FUN_0044e608(uVar2,uVar4);
      uVar1 = DAT_00755b18;
      if (0 < lVar5) goto LAB_00448268;
    }
  }
  param_1 = FUN_0044e168(uStack_80,uVar1);
  uVar10 = FUN_0044e168(param_1,uVar1);
  return uVar10;
}
