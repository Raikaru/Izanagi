/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0042f200 func_0042f200 src/promoted/code1_0042.c - romwright draft, unreviewed */

unsigned int FUN_0042f200(unsigned int param_1,int param_2,unsigned long long param_3,unsigned char *param_4)

{
  unsigned char bVar1;
  long lVar2;
  int iVar3;
  
  if (((DAT_0070dcf8 & 1) == 0) && (lVar2 = FUN_0042e730(), lVar2 == 6)) {
    return 0;
  }
  lVar2 = FUN_0042e5d8(4);
  if (lVar2 != 0) {
    DAT_0070ddc8 = (unsigned int)param_3;
    DAT_0070ddcc = *param_4;
    DAT_0070ddcd = param_4[1];
    DAT_0070ddce = param_4[2];
    DAT_0070ddd0 = &LAB_0070edc0;
    DAT_0070ddd4 = &DAT_0070ee80;
    bVar1 = param_4[2];
    if (bVar1 != 1) {
      iVar3 = param_2 << 0xb;
      if ((1 < bVar1) && (bVar1 == 2)) {
        iVar3 = param_2 * 0x924;
      }
    }
    else {
      iVar3 = param_2 * 0x918;
    }
    DAT_0070ee80 = 0;
    DAT_0070ddc0 = param_1;
    DAT_0070ddc4 = param_2;
    if ((DAT_0070dcf8 & 2) == 0) {
      FUN_00424e78(param_3,iVar3);
    }
    FUN_00424e78(0x70edc0,0x90);
    FUN_00424e78(0x70ddc0,0x18);
    FUN_00424e78(0x70ee80,4);
    if (0 < DAT_0070dcd0) {
      FUN_004244c8(0x7546c8);
    }
    DAT_0070dd20 = 1;
    DAT_0070dcf4 = 1;
    lVar2 = FUN_004258b0(0x70ee90,1,1,0x70ddc0,0x18,0,0,0x42df28);
    if (-1 < lVar2) {
      if (DAT_0070dcd0 < 1) {
        return 1;
      }
      FUN_004244c8(0x7546e0);
      return 1;
    }
    DAT_0070dd20 = 0;
    DAT_0070dcf4 = 0;
    FUN_00421800(DAT_0070dce8);
  }
  return 0;
}
