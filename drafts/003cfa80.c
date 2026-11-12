/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003cfa80 func_003cfa80 src/promoted/code1_003c.c - romwright draft, unreviewed */

void FUN_003cfa80(float param_1,float param_2,int param_3,long param_4)

{
  unsigned char bVar1;
  int iVar2;
  unsigned int *puVar3;
  unsigned int *puVar4;
  unsigned int uVar5;
  unsigned int uVar6;
  unsigned int uVar7;
  unsigned int uVar8;
  unsigned int uVar9;
  unsigned int uVar10;
  unsigned int uVar11;
  unsigned int uVar12;
  unsigned int uVar13;
  unsigned int uVar14;
  unsigned int uVar15;
  
  bVar1 = *(unsigned char *)(param_3 + 1);
  iVar2 = *(int *)(param_3 + 4);
  if (DAT_00764810 < 0x7c) {
    if (DAT_00764810 == 0) {
      if (param_4 != 0) {
        puVar3 = (unsigned int *)param_4;
        uVar15 = puVar3[1];
        uVar13 = puVar3[2];
        uVar14 = puVar3[4];
        uVar11 = puVar3[5];
        uVar12 = puVar3[6];
        *DAT_00764814 = *puVar3;
        uVar9 = puVar3[8];
        uVar10 = puVar3[9];
        uVar7 = puVar3[10];
        uVar8 = puVar3[0xc];
        uVar5 = puVar3[0xd];
        uVar6 = puVar3[0xe];
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar15;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar13;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_1;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar14;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar11;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar12;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_1;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar9;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar10;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar7;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_1;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar8;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar5;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar6;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_2;
      }
      else {
        *DAT_00764814 = 0x3f800000;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_1;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0x3f800000;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_1;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0x3f800000;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_1;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = 0;
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = param_2;
      }
      DAT_00764814 = DAT_00764814 + 1;
      DAT_00764810 = DAT_00764810 + 4;
    }
    if (iVar2 != 0) {
      iVar2 = FUN_003e9700();
      puVar3 = (unsigned int *)(iVar2 + 0x20);
      puVar4 = (unsigned int *)(iVar2 + 0x30);
    }
    else {
      puVar4 = (unsigned int *)0x0;
      puVar3 = (unsigned int *)0x0;
    }
    uVar5 = *(unsigned int *)(param_3 + 0x1c);
    uVar6 = *(unsigned int *)(param_3 + 0x20);
    *DAT_00764814 = *(unsigned int *)(param_3 + 0x18);
    DAT_00764814[1] = uVar5;
    DAT_00764814[2] = uVar6;
    DAT_00764814[3] = (unsigned int)bVar1;
    if (bVar1 != 0) {
      if ((bVar1 != 0x82) && (bVar1 != 0x81)) {
        if (bVar1 != 0x80) {
          if (bVar1 != 2) {
            if (bVar1 == 1) {
              DAT_00764814 = DAT_00764814 + 4;
              *DAT_00764814 = *puVar3;
              DAT_00764814 = DAT_00764814 + 1;
              *DAT_00764814 = puVar3[1];
              DAT_00764814 = DAT_00764814 + 1;
              *DAT_00764814 = puVar3[2];
              puVar3 = DAT_00764814 + 1;
              DAT_00764814 = DAT_00764814 + 1;
              *puVar3 = 0;
              DAT_00764814 = DAT_00764814 + 1;
              DAT_00764810 = DAT_00764810 + 2;
            }
          }
          else {
            DAT_00764814 = DAT_00764814 + 4;
            DAT_00764810 = DAT_00764810 + 1;
          }
        }
        else {
          uVar5 = *(unsigned int *)(param_3 + 0x14);
          DAT_00764814 = DAT_00764814 + 4;
          *DAT_00764814 = *puVar4;
          DAT_00764814 = DAT_00764814 + 1;
          *DAT_00764814 = puVar4[1];
          DAT_00764814 = DAT_00764814 + 1;
          *DAT_00764814 = puVar4[2];
          puVar3 = DAT_00764814 + 1;
          DAT_00764814 = DAT_00764814 + 1;
          *puVar3 = uVar5;
          DAT_00764814 = DAT_00764814 + 1;
          DAT_00764810 = DAT_00764810 + 2;
        }
      }
      else {
        uVar5 = *(unsigned int *)(param_3 + 0x14);
        uVar6 = *(unsigned int *)(param_3 + 0x28);
        DAT_00764814 = DAT_00764814 + 4;
        *DAT_00764814 = *puVar4;
        DAT_00764814 = DAT_00764814 + 1;
        *DAT_00764814 = puVar4[1];
        DAT_00764814 = DAT_00764814 + 1;
        *DAT_00764814 = puVar4[2];
        puVar4 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar4 = uVar5;
        DAT_00764814 = DAT_00764814 + 1;
        *DAT_00764814 = *puVar3;
        DAT_00764814 = DAT_00764814 + 1;
        *DAT_00764814 = puVar3[1];
        DAT_00764814 = DAT_00764814 + 1;
        *DAT_00764814 = puVar3[2];
        puVar3 = DAT_00764814 + 1;
        DAT_00764814 = DAT_00764814 + 1;
        *puVar3 = uVar6;
        DAT_00764814 = DAT_00764814 + 1;
        DAT_00764810 = DAT_00764810 + 3;
      }
    }
  }
  return;
}
