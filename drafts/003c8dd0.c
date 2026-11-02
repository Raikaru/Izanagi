/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c8dd0 func_003c8dd0 src/promoted/code1_003c.c - romwright draft, unreviewed */

unsigned long long FUN_003c8dd0(unsigned long long param_1)

{
  unsigned int *puVar1;
  int iVar2;
  long lVar3;
  unsigned long long uVar4;
  
  lVar3 = FUN_003c8ce0();
  if (lVar3 != 0) {
    for (puVar1 = *(unsigned int **)((int)param_1 + 0x38);
        puVar1 != (unsigned int *)((int)param_1 + 0x38); puVar1 = (unsigned int *)*puVar1) {
      iVar2 = puVar1[2];
      if (((*(unsigned char *)(iVar2 + 2) & 4) != 0) && (*(short *)(iVar2 + 0x60) != DAT_008872e8)) {
        uVar4 = FUN_003bfae0(iVar2);
        lVar3 = FUN_003e8200(DAT_008872e0,uVar4);
        if (lVar3 != 0) {
          (**(code **)(iVar2 + 0x48))(iVar2);
        }
        *(short *)(iVar2 + 0x60) = DAT_008872e8;
      }
    }
  }
  return param_1;
}
