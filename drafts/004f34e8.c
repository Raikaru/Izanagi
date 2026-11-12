/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f34e8 func_004f34e8 src/promoted/code1_004f.c - romwright draft, unreviewed */

long FUN_004f34e8(long param_1,long param_2)

{
  int iVar1;
  unsigned int *puVar2;
  long lVar3;
  long lVar4;
  unsigned long long uVar5;
  unsigned int *puVar6;
  unsigned int *puVar7;
  int iVar8;
  unsigned int *puVar9;
  unsigned int *puVar10;
  int *piVar11;
  int iVar12;
  int iVar13;
  
  if ((param_1 != 0) && (param_2 != 0)) {
    piVar11 = (int *)param_1;
    lVar3 = -100;
    if ((((piVar11[3] != 0) && (0 < *piVar11)) && (0 < piVar11[1])) && (2 < piVar11[2])) {
      FUN_004f3780();
      puVar2 = (unsigned int *)FUN_004f69c8();
      lVar4 = (*(code *)*puVar2)();
      lVar3 = -0xcb;
      if (lVar4 == 0) {
        iVar8 = piVar11[2];
        iVar1 = piVar11[1];
        DAT_00743a50 = (unsigned int *)((piVar11[3] + 7U) & 0xfffffff8);
        iVar13 = *piVar11 + 1;
        iVar12 = (*piVar11 * 0x3c + (iVar8 * 0x30 + 0x4c) * iVar1 + 0x110c) -
                 ((int)DAT_00743a50 - piVar11[3]);
        FUN_0043f9c8(DAT_00743a50,0,iVar12);
        puVar7 = DAT_00743a50;
        puVar6 = DAT_00743a50 + iVar13 * 0xf + 0x423;
        puVar9 = DAT_00743a50 + 0x423;
        puVar10 = DAT_00743a50 + 0x432;
        DAT_00743a50[10] = puVar6;
        puVar7[0xc] = puVar6 + iVar1 * 0xd;
        puVar7[3] = iVar8;
        puVar7[4] = iVar12;
        puVar7[7] = 50000;
        puVar7[8] = puVar9;
        puVar7[9] = puVar10;
        *puVar7 = 0;
        puVar7[1] = iVar13;
        puVar7[2] = iVar1;
        puVar7[5] = 0;
        puVar7[6] = 0;
        puVar7[0xb] = 0;
        uVar5 = FUN_004f1538();
        lVar3 = FUN_004f3870(uVar5);
        puVar7 = DAT_00743a50;
        if (lVar3 == 0) {
          DAT_00743a50[0xe] = puVar2;
          puVar7[0xf] = (int)param_2;
          puVar7[0x11] = 0;
          puVar7[0x10] = 0;
          FUN_004f5bf0(0,0,0);
          FUN_004f5bf0(0,1,0);
          FUN_004f5bf0(0,2,1);
          puVar2 = DAT_00743a50;
          iVar8 = DAT_00743a50[1];
          puVar7 = DAT_00743a50 + 0x423;
          DAT_00743a50[0x12] = ((int)DAT_00743a50 + 0x8bU) & 0xffffffc0;
          if (0 < iVar8) {
            do {
              iVar8 = iVar8 - 1;
              *(unsigned short *)(puVar7 + 0xd) = 0;
              puVar7[2] = puVar2;
              puVar7 = puVar7 + 0xf;
            } while (iVar8 != 0);
          }
          FUN_004f1708(DAT_00743a50[8],0,0,0,0);
          FUN_004f1850(DAT_00743a50[8]);
          DAT_00743a58 = 0;
          FUN_004f9378();
          lVar3 = 0;
          *DAT_00743a50 = 1;
        }
      }
    }
  }
  else {
    lVar3 = -100;
  }
  return lVar3;
}
