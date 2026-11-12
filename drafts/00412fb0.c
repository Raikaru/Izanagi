/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00412fb0 func_00412fb0 src/promoted/code1_0041.c - romwright draft, unreviewed */

/* WARNING: Removing unreachable block (ram,0x00413014) */
/* WARNING: Removing unreachable block (ram,0x00413098) */

long FUN_00412fb0(long param_1,long param_2,int param_3,unsigned long long param_4,unsigned long long param_5,
                 unsigned long long param_6,unsigned long long param_7,unsigned long long param_8)

{
  int iVar1;
  int *piVar2;
  long lVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  int *piVar7;
  int *piVar8;
  int iVar9;
  int uStack_78;
  unsigned int uStack_74;
  int uStack_70;
  unsigned int uStack_6c;
  int uStack_68;
  unsigned int uStack_64;
  unsigned long long uStack_28;
  unsigned long long uStack_20;
  unsigned long long uStack_18;
  unsigned long long uStack_10;
  unsigned long long uStack_8;
  
  uStack_28 = param_4;
  uStack_20 = param_5;
  uStack_18 = param_6;
  uStack_10 = param_7;
  uStack_8 = param_8;
  if ((param_1 != 0) && (piVar7 = (int *)param_1, *piVar7 != 0)) {
    iVar6 = 0;
    piVar2 = (int *)&uStack_28;
    iVar1 = param_3;
    while (iVar1 != 0) {
      iVar6 = iVar6 + 1;
      iVar1 = *piVar2;
      piVar2 = piVar2 + 2;
    }
    if (iVar6 != 0) {
      iVar1 = piVar7[1];
      if (*(unsigned int *)(DAT_00764aa8 + 0x887318) < (unsigned int)(iVar1 + iVar6)) {
        uStack_68 = 1;
        uStack_64 = FUN_003df590(0x2a);
        FUN_003df4d0(&uStack_68);
        return 0;
      }
      iVar9 = 0;
      piVar2 = (int *)&uStack_28;
      piVar5 = (int *)0x0;
      while (param_3 != 0) {
        piVar8 = (int *)(piVar7[2] + (iVar1 + iVar9) * 0x28);
        lVar3 = FUN_00411ce0(param_1,piVar8);
        if (lVar3 == 0) break;
        if (piVar5 != (int *)0x0) {
          if (((piVar5 != (int *)0x0) && (*piVar5 != 0)) && (piVar5[1] != 0)) {
            iVar4 = piVar5[2];
          }
          else {
            iVar4 = 0;
          }
          if ((piVar8 == (int *)0x0) || (piVar5 = piVar8, *piVar8 == 0)) {
            piVar5 = (int *)0x0;
          }
          lVar3 = FUN_00413290(param_1,iVar4,piVar5);
          if (lVar3 == 0) {
            FUN_00411130(piVar8,param_1);
            break;
          }
        }
        iVar9 = iVar9 + 1;
        param_3 = *piVar2;
        piVar2 = piVar2 + 2;
        piVar5 = piVar8;
      }
      if (iVar9 == iVar6) {
        if (param_2 == 0) {
          return param_1;
        }
        *(int *)param_2 = iVar1;
        return param_1;
      }
      while (iVar9 != 0) {
        iVar9 = iVar9 - 1;
        FUN_00411130(piVar7[2] + (iVar9 + iVar1) * 0x28,param_1);
      }
    }
  }
  else if (param_1 != 0) {
    uStack_78 = 1;
    uStack_74 = FUN_003df590(0x34);
    FUN_003df4d0(&uStack_78);
  }
  else {
    uStack_70 = 1;
    uStack_6c = FUN_003df590(0xffffffff80000016);
    FUN_003df4d0(&uStack_70);
  }
  return 0;
}
