/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003de110 RwFexist src/promoted/code1_003d.c - romwright draft, unreviewed */

unsigned long long FUN_003de110(unsigned long long param_1)

{
  unsigned int uVar1;
  unsigned long long uVar2;
  long lVar3;
  unsigned int *puVar4;
  unsigned char *puVar5;
  unsigned int uVar6;
  unsigned char uStack_70;
  unsigned char auStack_6f [111];
  
  if (DAT_00887188 != 0) {
    uVar1 = (*DAT_008873d4)();
    uVar6 = 0;
    if (uVar1 != 0) {
      puVar5 = &uStack_70;
      do {
        if (*(char *)((int)param_1 + uVar6) == ':') {
          FUN_00442de8(&uStack_70,param_1,uVar6 + 1);
          puVar5[1] = 0;
          for (puVar4 = DAT_00887180; puVar4 != (unsigned int *)0x0; puVar4 = (unsigned int *)*puVar4) {
            lVar3 = (*DAT_008873d0)(&uStack_70,puVar4[3]);
            if (lVar3 == 0) goto LAB_003de240;
          }
          puVar4 = (unsigned int *)0x0;
LAB_003de240:
          if (puVar4 != (unsigned int *)0x0) goto LAB_003de15c;
        }
        uVar6 = uVar6 + 1;
        puVar5 = puVar5 + 1;
      } while (uVar6 < uVar1);
    }
    puVar4 = DAT_00764840;
    if (DAT_00764840 != (unsigned int *)0x0) goto LAB_003de15c;
  }
  if (DAT_00887194 != (code *)0x0) {
    (*DAT_00887194)(6);
  }
  puVar4 = (unsigned int *)0x0;
LAB_003de15c:
  if (puVar4 == (unsigned int *)0x0) {
    uVar2 = 0;
  }
  else {
    uVar2 = (*(code *)puVar4[0x13])(puVar4,param_1);
  }
  return uVar2;
}
