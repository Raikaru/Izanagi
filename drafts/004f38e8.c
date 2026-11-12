/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f38e8 func_004f38e8 src/sce/rofs_mai.c - romwright draft, unreviewed */

unsigned long long FUN_004f38e8(long param_1,long param_2,long param_3)

{
  unsigned int uVar1;
  unsigned long long *puVar2;
  unsigned long long uVar3;
  long lVar4;
  long lVar5;
  long lVar6;
  unsigned int uVar7;
  unsigned int uVar8;
  int iVar9;
  unsigned char auStack_ff0 [8];
  unsigned long long uStack_fe8;
  unsigned int uStack_f84;
  unsigned char auStack_880 [48];
  unsigned char auStack_850 [132];
  unsigned int uStack_7cc;
  unsigned int uStack_7c4;
  int aiStack_50 [10];
  
  if (((param_1 == 0) || (param_2 == 0)) || (param_3 == 0)) {
    uVar3 = FUN_004f53c0(0x75d6c8,0x3c3,0x75d6f8,0,0xffffffffffffff9c);
    return uVar3;
  }
  lVar4 = FUN_004426e8(param_1,0x75d6c0);
  if (lVar4 == 0) {
    uVar3 = FUN_004f53c0(0x75d6c8,0x3c7,0x75d6f8);
    return uVar3;
  }
  lVar4 = FUN_004f2e80(param_1,0,param_3,0,0xffffffffffffff99);
  if (lVar4 != 0) {
    uVar3 = FUN_004f53c0(0x75d6c8,0x3de,0x75d708,0,0xffffffffffffff98);
    return uVar3;
  }
  lVar4 = FUN_004f2f58(param_1,param_2);
  if (lVar4 == 0) {
    uVar8 = 0x75d718;
    uVar7 = 0x3e5;
    uVar3 = FUN_004f53c0(0x75d6c8,uVar7,uVar8,0,0xffffffffffffff97);
  }
  else {
    lVar5 = FUN_004f1708(*(unsigned int *)(DAT_00743a50 + 0x20),0,0,0,0);
    if (lVar5 == 0) {
      FUN_004f30c0(lVar4);
      uVar3 = FUN_004f53c0(0x75d6c8,0x3f1,0x75d728,0,0xffffffffffffff32);
      return uVar3;
    }
    lVar6 = FUN_004f17a8(lVar5,param_2);
    if (lVar6 == 1) {
      FUN_004f30c0(lVar4);
      uVar3 = FUN_004f53c0(0x75d6c8,0x3f7,0x75d708,0,0xffffffffffffff38);
      return uVar3;
    }
    lVar6 = FUN_004f6820();
    if (lVar6 != 0) {
      FUN_004f30c0(lVar4);
      uVar8 = 0x75d738;
      uVar7 = 0x400;
    }
    else {
      lVar6 = FUN_004f3108(param_2);
      if (lVar6 == 1) {
        lVar6 = FUN_004f3258(auStack_ff0,auStack_880);
      }
      else {
        lVar6 = FUN_004f3150(lVar4,auStack_ff0,auStack_880,param_3);
      }
      if (lVar6 == 0) {
        iVar9 = (int)lVar4;
        *(unsigned int *)(iVar9 + 4) = uStack_f84;
        uVar1 = (iVar9 + 0x33U) & 7;
        puVar2 = (unsigned long long *)((iVar9 + 0x33U) - uVar1);
        *puVar2 = (*puVar2 & (-1L << (uVar1 + 1) * 8)) | (uStack_fe8 >> (7 - uVar1) * 8);
        uVar1 = (iVar9 + 0x2cU) & 7;
        puVar2 = (unsigned long long *)((iVar9 + 0x2cU) - uVar1);
        *puVar2 = (uStack_fe8 << uVar1 * 8) | (*puVar2 & (0xffffffffffffffffU >> (8 - uVar1) * 8));
        lVar6 = FUN_004f3340(lVar4,auStack_880);
        if (lVar6 == 0) {
          FUN_004f3d28(lVar5,lVar4);
          uVar3 = FUN_004f53c0(0x75d6c8,0x427,0x75d760,0,0xffffffffffffff31);
          return uVar3;
        }
        FUN_004f3d28(lVar5,0);
        lVar5 = FUN_004f2bc0(lVar6);
        if (lVar5 == 1) {
          FUN_004f5d38(auStack_850,lVar6);
          *(unsigned int *)(iVar9 + 8) = uStack_7c4;
          *(unsigned int *)(iVar9 + 0xc) = uStack_7cc;
          FUN_004f5cd0(0,0,aiStack_50);
          if (aiStack_50[0] == 1) {
            return 0;
          }
          lVar5 = FUN_004f33e0(lVar4,auStack_850,*(unsigned int *)(iVar9 + 0x10));
          if (lVar5 != -0x6d) {
            if (lVar5 != 0) {
              FUN_004f30c0(lVar4);
              uVar3 = FUN_004f53c0(0x75d6c8,0x45f,0x75d778,0,lVar5);
              return uVar3;
            }
            return 0;
          }
          FUN_004f30c0(lVar4);
          uVar3 = FUN_004f53c0(0x75d6c8,0x45b,0x75d778,0,0xffffffffffffff93);
          return uVar3;
        }
        FUN_004f30c0(lVar4);
        uVar8 = 0x75d768;
        uVar7 = 0x434;
        uVar3 = FUN_004f53c0(0x75d6c8,uVar7,uVar8,0,0xffffffffffffff97);
        return uVar3;
      }
      FUN_004f3d28(lVar5,lVar4);
      uVar8 = 0x75d750;
      uVar7 = 0x414;
    }
    uVar3 = FUN_004f53c0(0x75d6c8,uVar7,uVar8,0,lVar6);
  }
  return uVar3;
}
