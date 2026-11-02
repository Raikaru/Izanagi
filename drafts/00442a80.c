/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x00442a80 func_00442a80 src/middleware/middleware_inferred_00442100.c - romwright draft, unreviewed */

unsigned long long FUN_00442a80(unsigned long long param_1,char *param_2,int param_3)

{
  unsigned char auVar1 [16];
  char cVar2;
  unsigned long long uVar3;
  unsigned char auVar4 [16];
  unsigned char auVar5 [16];
  unsigned char auVar6 [16];
  unsigned char (*pauVar7) [16];
  unsigned long long in_a0_udw;
  unsigned long long in_a3_udw;
  unsigned char auVar8 [16];
  unsigned char auVar9 [16];
  
  pauVar7 = (unsigned char (*) [16])param_1;
  if ((param_1 & 7) != 0) {
    cVar2 = (*pauVar7)[0];
  }
  else {
    if ((param_1 & 0xf) != 0) {
      uVar3 = (*(unsigned long long *)*pauVar7 + 0xfefefefefefefeff) & ~*(unsigned long long *)*pauVar7;
      while ((uVar3 & 0x8080808080808080) == 0) {
        pauVar7 = (unsigned char (*) [16])(*pauVar7 + 8);
        uVar3 = (*(unsigned long long *)*pauVar7 + 0xfefefefefefefeff) & ~*(unsigned long long *)*pauVar7;
      }
    }
    else {
      auVar8._8_8_ = in_a3_udw;
      auVar8._0_8_ = 0x101010101010101;
      auVar9._8_8_ = in_a3_udw;
      auVar9._0_8_ = 0x101010101010101;
      auVar8 = _pcpyld(auVar8,auVar9);
      auVar4 = _psubb(*pauVar7,auVar8);
      auVar5._8_8_ = in_a0_udw;
      auVar5._0_8_ = 0x8080808080808080;
      auVar6._8_8_ = in_a0_udw;
      auVar6._0_8_ = 0x8080808080808080;
      auVar9 = _pcpyld(auVar5,auVar6);
      auVar5 = _pand(auVar4,~*pauVar7);
      auVar6 = _pand(auVar5,auVar9);
      auVar4._8_8_ = in_a0_udw;
      auVar4._0_8_ = 0x8080808080808080;
      auVar5 = _pcpyud(auVar6,auVar4);
      if (auVar6._0_8_ == 0 && auVar5._0_8_ == 0) {
        do {
          pauVar7 = pauVar7 + 1;
          auVar5 = _psubb(*pauVar7,auVar8);
          auVar5 = _pand(auVar5,~*pauVar7);
          auVar5 = _pand(auVar5,auVar9);
          auVar1._8_8_ = in_a0_udw;
          auVar1._0_8_ = 0x8080808080808080;
          auVar6 = _pcpyud(auVar5,auVar1);
        } while (auVar5._0_8_ == 0 && auVar6._0_8_ == 0);
      }
    }
    cVar2 = (*pauVar7)[0];
  }
  param_3 = param_3 - 1;
  while (cVar2 != '\0') {
    pauVar7 = (unsigned char (*) [16])(*pauVar7 + 1);
    cVar2 = (*pauVar7)[0];
  }
  if (param_3 != -1) {
    cVar2 = *param_2;
    (*pauVar7)[0] = cVar2;
    while (cVar2 != '\0') {
      param_2 = param_2 + 1;
      pauVar7 = (unsigned char (*) [16])(*pauVar7 + 1);
      if (param_3 == 0) {
        (*pauVar7)[0] = 0;
      }
      param_3 = param_3 - 1;
      if (param_3 == -1) {
        return param_1;
      }
      cVar2 = *param_2;
      (*pauVar7)[0] = cVar2;
    }
  }
  return param_1;
}
