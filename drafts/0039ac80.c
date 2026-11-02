/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039ac80 func_0039ac80 src/promoted/code1_0039.c - romwright draft, unreviewed */

unsigned long long FUN_0039ac80(unsigned long long param_1,int param_2)

{
  int iVar1;
  int *piVar2;
  unsigned int *puVar3;
  int *piVar4;
  int iVar5;
  int iVar6;
  
  piVar2 = (int *)(param_2 + DAT_007646dc);
  iVar5 = (int)param_1;
  piVar4 = (int *)(iVar5 + DAT_007646dc);
  iVar1 = piVar2[2];
  puVar3 = (unsigned int *)(iVar5 + 0x4c);
  if (iVar1 != *(int *)(param_2 + 0x18)) {
    *piVar4 = 0;
    piVar4[1] = 0;
    piVar4[2] = 0;
  }
  else {
    iVar6 = piVar2[1];
    *piVar4 = *piVar2;
    piVar4[1] = iVar6;
    piVar4[2] = iVar1;
    if (*piVar4 != 0) {
      *(unsigned short *)(iVar5 + 0x50) = *(unsigned short *)(*piVar4 + 4);
      *puVar3 = *puVar3 | 3;
      if (((*puVar3 & 4) == 0) && (*(int *)(iVar5 + 4) != 0)) {
        FUN_003e9680();
      }
      *(unsigned short *)(iVar5 + 0x52) = *(unsigned short *)(*piVar4 + 6);
      *puVar3 = *puVar3 | 3;
      if (((*puVar3 & 4) == 0) && (*(int *)(iVar5 + 4) != 0)) {
        FUN_003e9680();
      }
      *(unsigned int *)(iVar5 + 0x54) = *(unsigned int *)(*piVar4 + 8);
      *(float *)(iVar5 + 0x58) = 1.0f / *(float *)(*piVar4 + 8);
      *puVar3 = *puVar3 | 3;
      if (((*puVar3 & 4) == 0) && (*(int *)(iVar5 + 4) != 0)) {
        FUN_003e9680();
      }
      *(int *)(iVar5 + 0x5c) = piVar4[1];
      *puVar3 = *puVar3 | 3;
      if (((*puVar3 & 4) == 0) && (*(int *)(iVar5 + 4) != 0)) {
        FUN_003e9680();
      }
    }
  }
  return param_1;
}
