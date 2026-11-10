/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d0790 func_003d0790 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d0790_arg0 fn_003d0790_arg0;
struct fn_003d0790_arg0 {
    unsigned char pad_0x0[0xc];
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x1c];
    int field_0x2c; /* +0x2c */
    int field_0x30; /* +0x30 */
    int field_0x34; /* +0x34 */
    int field_0x38; /* +0x38 */
};


unsigned int FUN_003d0790(fn_003d0790_arg0 *param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1->field_0x2c + 4);
  if ((*(unsigned int *)param_1->field_0xc & 2) != 0) {
    iVar1 = (iVar1 - 2) * 3;
  }
  if (iVar1 != *(int *)(*(int *)param_1->field_0x30 + 0x24)) {
    param_1->field_0x34 = param_1->field_0x34 | 8;
  }
  if (((param_1->field_0x34 & 8U) == 0) &&
     (iVar1 = *(int *)param_1->field_0x30, param_1->field_0x38 = *(int *)param_1->field_0xc,
     param_1->field_0x38 != *(int *)(iVar1 + 0x2c))) {
    param_1->field_0x34 = param_1->field_0x34 | 8;
  }
  return 1;
}
