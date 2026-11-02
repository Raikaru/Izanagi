/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f4248 func_004f4248 src/sce/rofs_mai.c - romwright draft, unreviewed */

int FUN_004f4248(unsigned long long param_1,long param_2,long param_3)

{
  long lVar1;
  int iVar2;
  unsigned long long uVar3;
  unsigned long long uVar4;
  unsigned long long uVar5;
  int iVar6;
  
  if ((param_3 < 0) || (param_2 < 0)) {
    uVar4 = 0x75d820;
    uVar3 = 0x52b;
    uVar5 = 0xffffffffffffff9c;
  }
  else {
    lVar1 = FUN_004f2e80(param_1,1);
    if (lVar1 == 0) {
      uVar4 = 0x75d798;
      uVar3 = 0x532;
      uVar5 = 0xffffffffffffff97;
    }
    else {
      iVar2 = 0;
      iVar6 = *(int *)(DAT_00743a50 + 0x24);
      if (0 < *(int *)(DAT_00743a50 + 4) - 1) {
        do {
          if (*(short *)(iVar6 + 0x34) == 0) {
            FUN_004f16b0(iVar6,0,param_2,(int)param_3 << 0xb);
            if (iVar6 != 0) {
              return iVar6;
            }
            FUN_004f53c0(0x75d6c8,0x53d,0x75d800,0,0xffffffffffffff9a);
            return 0;
          }
          iVar2 = iVar2 + 1;
          iVar6 = iVar6 + 0x3c;
        } while (iVar2 < *(int *)(DAT_00743a50 + 4) - 1);
      }
      uVar4 = 0x75d820;
      uVar3 = 0x544;
      uVar5 = 0xffffffffffffff9a;
    }
  }
  FUN_004f53c0(0x75d6c8,uVar3,uVar4,0,uVar5);
  return 0;
}
