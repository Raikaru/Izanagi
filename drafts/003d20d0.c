/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d20d0 func_003d20d0 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d20d0_arg0 fn_003d20d0_arg0;
struct fn_003d20d0_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x4];
    undefined4 field_0x14; /* +0x14 */
    unsigned char pad_0x18[0x10];
    byte field_0x28; /* +0x28 */
    undefined1 field_0x29; /* +0x29 */
    undefined1 field_0x2a; /* +0x2a */
};


void FUN_003d20d0(fn_003d20d0_arg0 *param_1,int *param_2)

{
  unsigned int *puVar1;
  unsigned int uVar2;
  int iVar3;
  
  puVar1 = (unsigned int *)param_1->field_0x8;
  FUN_003d2010(puVar1,param_1);
  param_1->field_0x14 = 0x7fff8;
  if (puVar1[1] != 0) {
    FUN_003e05f0(*param_2);
  }
  else {
    *param_2 = DAT_008872e0 + 0x20;
  }
  iVar3 = 1;
  if ((*puVar1 & 4) != 0) {
    iVar3 = 2;
  }
  uVar2 = *(unsigned int *)param_1->field_0xc;
  if ((uVar2 & 1) != 1) {
    if (((uVar2 & 0xff) != 0) && ((uVar2 & 2) != 2)) {
      if ((uVar2 & 0x10) != 0x10) {
        if ((uVar2 & 8) == 8) {
          param_1->field_0x29 = 2;
          param_1->field_0x28 = DAT_00764a74 | 0x10;
        }
        else {
          param_1->field_0x29 = 1;
          param_1->field_0x28 = DAT_00764a74 | 0x14;
        }
      }
      else {
        param_1->field_0x29 = 4;
        param_1->field_0x28 = DAT_00764a74;
      }
    }
    else {
      param_1->field_0x29 = 3;
      param_1->field_0x28 = DAT_00764a74 | 4;
    }
  }
  else {
    param_1->field_0x29 = 4;
    param_1->field_0x28 = DAT_00764a74;
  }
  if (iVar3 != 2) {
    param_1->field_0x28 = param_1->field_0x28 | 2;
  }
  param_1->field_0x2a = 1;
  return;
}
