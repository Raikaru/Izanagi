/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e6120 dvCiReqRd src/cri/dvci.c - romwright draft, unreviewed */

int FUN_004e6120(long param_1,int param_2,long param_3)

{
  unsigned char uVar1;
  long lVar2;
  int iVar3;
  unsigned long long uVar4;
  int iVar5;
  int iVar6;
  
  if (param_1 == 0) {
    uVar4 = 0x75b1c8;
  }
  else if (param_2 < 0) {
    uVar4 = 0x75b1e0;
  }
  else {
    if (param_3 != 0) {
      iVar6 = (int)param_1;
      if (*(char *)(iVar6 + 2) == '\x02') {
        return 0;
      }
      if (param_2 == 0) {
        *(unsigned char *)(iVar6 + 2) = 1;
        return 0;
      }
      lVar2 = FUN_004e36e8();
      *(int *)(iVar6 + 0x1c) = (int)lVar2;
      if (lVar2 == 0) {
        return 0;
      }
      iVar3 = *(int *)(iVar6 + 8) - *(int *)(iVar6 + 0xc);
      *(unsigned char *)(iVar6 + 0x44) = DAT_00733dc0;
      iVar5 = param_2;
      if (iVar3 <= param_2) {
        iVar5 = iVar3;
      }
      *(unsigned char *)(iVar6 + 0x45) = DAT_00733dc1;
      uVar1 = DAT_00733dc2;
      *(int *)(iVar6 + 0x10) = iVar5;
      *(unsigned char *)(iVar6 + 0x46) = uVar1;
      *(int *)(iVar6 + 0x18) = (int)param_3;
      FUN_00422218(param_3,((int)param_3 + iVar5 * 0x800) - 1);
      lVar2 = FUN_004e3810(*(unsigned int *)(iVar6 + 0x1c),
                           *(int *)(iVar6 + 0x20) + *(int *)(iVar6 + 0xc),
                           *(unsigned int *)(iVar6 + 0x10),*(unsigned int *)(iVar6 + 0x18),iVar6 + 0x44)
      ;
      if (lVar2 != 0) {
        if (DAT_00733dc4 != 1) {
          *(unsigned char *)(iVar6 + 2) = 2;
          return *(int *)(iVar6 + 0x10);
        }
        FUN_0042e7c8(0);
        FUN_004e42a0();
        FUN_004e4190();
        FUN_004e3ae0(*(unsigned int *)(iVar6 + 0x1c));
        FUN_004e3770(*(unsigned int *)(iVar6 + 0x1c));
        *(unsigned int *)(iVar6 + 0x1c) = 0;
        *(int *)(iVar6 + 0x14) = param_2;
        *(unsigned char *)(iVar6 + 2) = 1;
        *(int *)(iVar6 + 0xc) = *(int *)(iVar6 + 0xc) + *(int *)(iVar6 + 0x10);
        return *(int *)(iVar6 + 0x10);
      }
      FUN_004e3770(*(unsigned int *)(iVar6 + 0x1c));
      return 0;
    }
    uVar4 = 0x75b200;
  }
  FUN_004e5a38(param_1,uVar4);
  return 0;
}
