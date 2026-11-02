/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003f25c0 func_003f25c0 src/rw/palquant.c - romwright draft, unreviewed */

void FUN_003f25c0(unsigned long long param_1,int param_2,long param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  
  if (param_2 != 0) {
    if (0 < param_3) {
      iVar4 = 0;
      iVar5 = param_2;
      do {
        iVar1 = *(int *)(iVar5 + 0x1c);
        if (iVar1 != 0) {
          if (0 < (int)param_3 - 1) {
            iVar3 = 0;
            iVar2 = iVar1;
            do {
              FUN_003f25c0(param_1,*(unsigned int *)(iVar2 + 0x1c),(int)param_3 - 2);
              iVar3 = iVar3 + 1;
              iVar2 = iVar2 + 4;
            } while (iVar3 < 0x10);
          }
          (*DAT_008873fc)(*(unsigned int *)((int)param_1 + 0x4004),iVar1);
        }
        iVar4 = iVar4 + 1;
        iVar5 = iVar5 + 4;
      } while (iVar4 < 0x10);
    }
    (*DAT_008873fc)(*(unsigned int *)((int)param_1 + 0x4004),param_2);
  }
  return;
}
