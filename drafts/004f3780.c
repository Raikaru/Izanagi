/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f3780 func_004f3780 src/promoted/code1_004f.c - romwright draft, unreviewed */

void FUN_004f3780(void)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  
  if ((DAT_00743a50 != (int *)0x0) && (*DAT_00743a50 == 1)) {
    iVar2 = 0;
    FUN_004f93d0();
    piVar1 = DAT_00743a50 + 0x423;
    if (0 < DAT_00743a50[1]) {
      do {
        FUN_004f1850(piVar1);
        iVar2 = iVar2 + 1;
        piVar1 = piVar1 + 0xf;
      } while (iVar2 < DAT_00743a50[1]);
    }
    iVar3 = 0;
    iVar2 = DAT_00743a50[10];
    if (0 < DAT_00743a50[2]) {
      do {
        FUN_004f30c0(iVar2);
        iVar3 = iVar3 + 1;
        iVar2 = iVar2 + 0x34;
      } while (iVar3 < DAT_00743a50[2]);
    }
    (**(code **)(DAT_00743a50[0xe] + 4))();
    piVar1 = DAT_00743a50;
    *DAT_00743a50 = 0;
    FUN_0043f9c8(piVar1,0,piVar1[4]);
    DAT_00743a50 = (int *)0x0;
  }
  return;
}
