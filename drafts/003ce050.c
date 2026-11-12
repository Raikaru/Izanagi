/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003ce050 _rpObjectGetMeshCacheResize src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003ce050(unsigned long long param_1,int param_2)

{
  unsigned char bVar1;
  int *piVar2;
  int *piVar3;
  int *piVar4;
  int *piVar5;
  int iVar6;
  
  piVar4 = (int *)param_1;
  piVar2 = (int *)*piVar4;
  if (piVar2 != (int *)0x0) {
    iVar6 = *piVar2 - 1;
    if (*piVar2 != 0) {
      piVar5 = piVar2 + iVar6;
      do {
        piVar3 = (int *)piVar5[1];
        if (piVar3 != (int *)0x0) {
          if ((*piVar3 != 0) || (piVar3[1] != 0)) {
            piVar3[4] = 0;
          }
          else {
            FUN_003cc6e0(piVar3);
            FUN_003e1ea0(piVar3);
          }
        }
        piVar5 = piVar5 + -1;
        bVar1 = iVar6 != 0;
        iVar6 = iVar6 - 1;
      } while (bVar1);
    }
    (*DAT_008873ec)(piVar2);
    *piVar4 = 0;
  }
  iVar6 = (*DAT_008873e8)((param_2 - 1) * 4 + 8,0x3050d);
  *piVar4 = iVar6;
  if ((int *)*piVar4 != (int *)0x0) {
    *(int *)*piVar4 = param_2;
    FUN_0043f9c8(*piVar4 + 4,0,param_2 << 2);
  }
  return param_1;
}
