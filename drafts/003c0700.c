/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003c0700 func_003c0700 src/promoted/code1_003c.c - romwright draft, unreviewed */
typedef struct fn_003c0700_arg0 fn_003c0700_arg0;
struct fn_003c0700_arg0 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x4];
    int field_0x10; /* +0x10 */
    unsigned char pad_0x14[0x4];
    int field_0x18; /* +0x18 */
};


unsigned int FUN_003c0700(fn_003c0700_arg0 *param_1)

{
  unsigned int *puVar1;
  unsigned int *puVar2;
  int iVar3;
  
  FUN_003e3c20(0x70af90,param_1);
  puVar1 = (unsigned int *)param_1->field_0x8;
  do {
    if (puVar1 == &param_1->field_0x8) break;
    puVar2 = puVar1 + -0x10;
    puVar1 = (unsigned int *)*puVar1;
    FUN_003c02e0(puVar2);
  } while (puVar2 != (unsigned int *)0x0);
  puVar1 = (unsigned int *)param_1->field_0x10;
  do {
    if (puVar1 == &param_1->field_0x10) break;
    puVar2 = (unsigned int *)*puVar1;
    iVar3 = (int)puVar1 + (-4 - DAT_007647a4);
    FUN_003c0880(*(unsigned int *)(iVar3 + DAT_007647a4),iVar3);
    FUN_003c3cc0(iVar3);
    puVar1 = puVar2;
  } while (iVar3 != 0);
  puVar1 = (unsigned int *)param_1->field_0x18;
  do {
    if (puVar1 == &param_1->field_0x18) break;
    puVar2 = (unsigned int *)*puVar1;
    iVar3 = (int)puVar1 + (-4 - DAT_007647a0);
    FUN_003c08c0(*(unsigned int *)(iVar3 + DAT_007647a0),iVar3);
    FUN_003e8440(iVar3);
    puVar1 = puVar2;
  } while (iVar3 != 0);
  if (param_1->field_0x4 != 0) {
    FUN_003e95c0();
  }
  (*DAT_008873fc)(*(unsigned int *)((int)&DAT_008872e4 + DAT_007647b0),param_1);
  return 1;
}
