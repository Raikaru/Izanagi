/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004f8848 func_004f8848 src/sce/rofs_if.c - romwright draft, unreviewed */

int FUN_004f8848(long param_1,long param_2,unsigned long long param_3,unsigned long long param_4)

{
  int iVar1;
  int *piVar2;
  
  if (param_2 != 100) {
    if (param_2 < 0x65) {
      piVar2 = (int *)param_1;
      if (param_2 != 3) {
        if (param_2 < 4) {
          if (param_2 != 1) {
            iVar1 = -1;
            if (((param_2 == 2) && (param_1 != 0)) && ((*piVar2 != 0 && (piVar2[1] != 0)))) {
              iVar1 = FUN_004f8740(piVar2[1],*piVar2,piVar2[2]);
              return iVar1;
            }
          }
          else {
            iVar1 = -1;
          }
        }
        else if (param_2 != 5) {
          if (param_2 >= 5) {
            iVar1 = -1;
            if (((param_2 == 6) && (param_1 != 0)) && (piVar2[1] != 0)) {
              iVar1 = FUN_004f2188();
              return iVar1;
            }
          }
          else {
            iVar1 = -1;
            if ((param_1 != 0) && (piVar2[1] != 0)) {
              iVar1 = FUN_004f29b8();
              return iVar1;
            }
          }
        }
        else {
          iVar1 = -1;
          if ((param_1 != 0) && (piVar2[1] != 0)) {
            iVar1 = FUN_004f2668(piVar2[1],piVar2[2]);
            return iVar1;
          }
        }
      }
      else {
        iVar1 = -1;
        if ((param_1 != 0) && (piVar2[1] != 0)) {
          iVar1 = FUN_004f87e8();
          return iVar1;
        }
      }
    }
    else if (param_2 != 0xcb) {
      if (param_2 < 0xcc) {
        if (param_2 != 0xc9) {
          if (param_2 < 0xca) {
            iVar1 = -1;
            if ((param_2 == 200) && (param_1 != 0)) {
              iVar1 = FUN_004f1d80();
              iVar1 = iVar1 >> 0x20;
            }
          }
          else {
            iVar1 = FUN_004f24d8(param_3);
            iVar1 = iVar1 >> 0x20;
          }
        }
        else {
          iVar1 = -1;
          if (param_1 != 0) {
            iVar1 = FUN_004f1d80();
          }
        }
      }
      else if (param_2 != 0xcd) {
        if (param_2 >= 0xcd) {
          iVar1 = 1;
          if (param_2 != 299) {
            iVar1 = -1;
          }
        }
        else {
          iVar1 = FUN_004f25c8(param_3,param_4);
          iVar1 = iVar1 >> 0x20;
        }
      }
      else {
        iVar1 = FUN_004f25c8(param_3,param_4);
      }
    }
    else {
      iVar1 = FUN_004f24d8(param_3);
    }
  }
  else {
    iVar1 = 1;
  }
  return iVar1;
}
