/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039bb70 PDSRegistryGetPipe src/promoted/code1_0039.c - romwright draft, unreviewed */

int FUN_0039bb70(int *param_1,int param_2)

{
  int iVar1;
  unsigned int uVar2;
  
  uVar2 = 0;
  if (param_1[1] != 0) {
    iVar1 = *param_1;
    do {
      if (param_2 == *(int *)(iVar1 + 8)) {
        return *param_1 + uVar2 * 0x14;
      }
      uVar2 = uVar2 + 1;
      iVar1 = iVar1 + 0x14;
    } while (uVar2 < (unsigned int)param_1[1]);
  }
  return 0;
}
