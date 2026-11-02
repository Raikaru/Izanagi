/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039b720 func_0039b720 src/promoted/code1_0039.c - romwright draft, unreviewed */

void FUN_0039b720(int *param_1,int *param_2)

{
  int iVar1;
  unsigned int uVar2;
  
  if (param_1[3] == 2) {
    if (param_1[1] != 0) {
      uVar2 = 0;
      if (param_2[1] != 0) {
        iVar1 = *param_2;
        do {
          if (param_1[1] == *(int *)(iVar1 + 8)) {
            *(unsigned int *)(*param_1 + 0x10) = *(unsigned int *)(iVar1 + 0x10);
            break;
          }
          uVar2 = uVar2 + 1;
          iVar1 = iVar1 + 0x14;
        } while (uVar2 < (unsigned int)param_2[1]);
      }
    }
    else {
      *(unsigned int *)(*param_1 + 0x10) = 0;
    }
    iVar1 = FUN_0039bdf0();
    param_1[4] = iVar1;
    *(int *)(param_1[4] + 0x30) = param_1[2];
  }
  return;
}
