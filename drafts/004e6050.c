/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004e6050 func_004e6050 src/cri/dvci.c - romwright draft, unreviewed */
typedef struct fn_004e6050_arg0 fn_004e6050_arg0;
struct fn_004e6050_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x4];
    undefined4 field_0x14; /* +0x14 */
};


int FUN_004e6050(fn_004e6050_arg0 *param_1,int param_2,long param_3)

{
  int iVar1;
  int iVar2;
  
  if (param_1 == (fn_004e6050_arg0 *)0x0) {
    FUN_004e5a38(0,0x75b1c8);
    iVar1 = 0;
  }
  else {
    if (param_3 == 0) {
      param_1->field_0xc = param_2;
      iVar2 = param_1->field_0x8;
    }
    else if (param_3 == 2) {
      iVar2 = param_1->field_0x8;
      param_1->field_0xc = iVar2 + param_2;
    }
    else {
      iVar2 = param_1->field_0x8;
      if (param_3 == 1) {
        param_1->field_0xc = param_1->field_0xc + param_2;
      }
    }
    param_1->field_0x14 = 0;
    iVar1 = param_1->field_0xc;
    if (iVar2 <= param_1->field_0xc) {
      iVar1 = iVar2;
    }
    if (iVar1 < 0) {
      iVar1 = 0;
    }
    param_1->field_0xc = iVar1;
  }
  return iVar1;
}
