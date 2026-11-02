/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a2770 func_003a2770 src/promoted/code1_003a.c - romwright draft, unreviewed */

unsigned int FUN_003a2770(int param_1,int *param_2,unsigned int param_3,unsigned int param_4)

{
  int iVar1;
  int iVar2;
  
  iVar1 = *(int *)(param_1 + DAT_00764700);
  if ((*(unsigned int *)(iVar1 + 0xa8) & param_3) == 0) {
    *param_2 = 0;
    param_2[1] = 0;
    return 0;
  }
  if (param_3 != 0x100) {
    if (param_3 != 0x80) {
      if (param_3 != 0x40) {
        if (param_3 != 0x20) {
          if (param_3 != 0x10) {
            if (param_3 != 8) {
              if (param_3 != 4) {
                if (param_3 != 2) {
                  if (param_3 != 1) {
                    iVar1 = *(int *)(iVar1 + 0xa4);
                    if (iVar1 != 0) {
                      param_2[1] = iVar1;
                    }
                    if ((*param_2 != 0) && ((param_4 & 0x40000000) == 0x40000000)) {
                      *(unsigned int *)(*(int *)(param_1 + DAT_00764700) + 0x3c) =
                           *(unsigned int *)(*(int *)(param_1 + DAT_00764700) + 0x3c) | param_4 | param_3;
                    }
                    return 1;
                  }
                  iVar2 = *(int *)(iVar1 + 0x58);
                  *param_2 = *(int *)(iVar1 + 0x54);
                  param_2[1] = iVar2;
                }
                else {
                  iVar2 = *(int *)(iVar1 + 0x78);
                  *param_2 = *(int *)(iVar1 + 0x74);
                  param_2[1] = iVar2;
                }
              }
              else {
                iVar2 = *(int *)(iVar1 + 0x70);
                *param_2 = *(int *)(iVar1 + 0x6c);
                param_2[1] = iVar2;
              }
            }
            else {
              iVar2 = *(int *)(iVar1 + 0x60);
              *param_2 = *(int *)(iVar1 + 0x5c);
              param_2[1] = iVar2;
            }
          }
          else {
            iVar2 = *(int *)(iVar1 + 0x68);
            *param_2 = *(int *)(iVar1 + 100);
            param_2[1] = iVar2;
          }
        }
        else {
          iVar2 = *(int *)(iVar1 + 0x88);
          *param_2 = *(int *)(iVar1 + 0x84);
          param_2[1] = iVar2;
        }
      }
      else {
        iVar2 = *(int *)(iVar1 + 0x80);
        *param_2 = *(int *)(iVar1 + 0x7c);
        param_2[1] = iVar2;
      }
    }
    else {
      iVar2 = *(int *)(iVar1 + 0x90);
      *param_2 = *(int *)(iVar1 + 0x8c);
      param_2[1] = iVar2;
    }
  }
  else {
    iVar2 = *(int *)(iVar1 + 0x98);
    *param_2 = *(int *)(iVar1 + 0x94);
    param_2[1] = iVar2;
  }
  iVar1 = *(int *)(iVar1 + 0xa4);
  if (iVar1 != 0) {
    param_2[1] = iVar1;
  }
  if ((*param_2 != 0) && ((param_4 & 0x40000000) == 0x40000000)) {
    *(unsigned int *)(*(int *)(param_1 + DAT_00764700) + 0x3c) =
         *(unsigned int *)(*(int *)(param_1 + DAT_00764700) + 0x3c) | param_4 | param_3;
  }
  return 1;
}
