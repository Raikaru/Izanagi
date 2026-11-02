/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003ce9e0 func_003ce9e0 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003ce9e0(unsigned long long param_1,unsigned long long param_2)

{
  unsigned int *puVar1;
  int iVar2;
  long lVar3;
  unsigned int uVar4;
  unsigned int uVar5;
  unsigned int *puVar6;
  int uStack_8;
  int iStack_4;
  
  iVar2 = FUN_003c9c20(param_2);
  if ((*(unsigned int *)(iVar2 + 8) & 0x1000000) != 0) {
    puVar1 = *(unsigned int **)((int)param_2 + DAT_00764818);
    uVar5 = *puVar1;
    uVar4 = 0;
    puVar6 = puVar1;
    if (uVar5 != 0) {
      do {
        FUN_003cc6e0(puVar6[1]);
        uVar4 = uVar4 + 1;
        puVar6 = puVar6 + 1;
      } while (uVar4 < uVar5);
    }
    iVar2 = FUN_003ceeb0(param_2);
    lVar3 = FUN_003deff0(param_1,1,iVar2 - 0xc,0x37002,0x37);
    if (lVar3 == 0) {
      param_1 = 0;
    }
    else {
      lVar3 = FUN_003e2ab0(param_1,0x763bac,4);
      if (lVar3 != 0) {
        uVar5 = 0;
        puVar6 = puVar1;
        if (*puVar1 != 0) {
          do {
            uVar4 = puVar6[1];
            iStack_4 = *(int *)(uVar4 + 8) - (*(int *)(uVar4 + 0x20) - uVar4);
            lVar3 = FUN_003e2ab0(param_1,&iStack_4,4);
            if (lVar3 == 0) {
              return 0;
            }
            uStack_8 = FUN_003ce170(uVar4 + 0x18);
            lVar3 = FUN_003e2ab0(param_1,&uStack_8,4);
            if (lVar3 == 0) {
              return 0;
            }
            lVar3 = FUN_003e2ab0(param_1,*(unsigned int *)(uVar4 + 0x20),iStack_4);
            if (lVar3 == 0) {
              return 0;
            }
            FUN_003ce230(uVar4 + 0x18);
            FUN_004220d8(uVar4,uVar4 + *(int *)(uVar4 + 8) + 0x18);
            uVar5 = uVar5 + 1;
            puVar6 = puVar6 + 1;
          } while (uVar5 < *puVar1);
        }
      }
      else {
        param_1 = 0;
      }
    }
  }
  return param_1;
}
