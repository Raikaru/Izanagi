/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003bb3a0 func_003bb3a0 src/promoted/code1_003b.c - romwright draft, unreviewed */
typedef struct fn_003bb3a0_arg0 fn_003bb3a0_arg0;
struct fn_003bb3a0_arg0 {
    unsigned char pad_0x0[0x8];
    int field_0x8; /* +0x8 */
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x4];
    int field_0x14; /* +0x14 */
};


unsigned long long FUN_003bb3a0(fn_003bb3a0_arg0 *param_1,int param_2,unsigned long long param_3)

{
  int iVar1;
  unsigned int *puVar2;
  unsigned int *puVar3;
  unsigned int uVar4;
  unsigned int uVar5;
  int uStack_10;
  unsigned int uStack_c;
  int uStack_8;
  unsigned int uStack_4;
  
  if (-1 < param_2) {
    if (param_1->field_0xc != 2) {
      if (param_1->field_0xc == 1) {
        iVar1 = param_1->field_0x8 - 2;
      }
      else {
        uStack_10 = 0x102;
        uStack_c = FUN_003df590();
        FUN_003df4d0(&uStack_10);
        iVar1 = 0;
      }
    }
    else {
      iVar1 = param_1->field_0x8 - 3;
    }
    if (param_2 < iVar1) {
      puVar2 = (unsigned int *)(param_1->field_0x14 + param_2 * 0xc);
      uVar4 = puVar2[1];
      uVar5 = puVar2[2];
      puVar3 = (unsigned int *)param_3;
      *puVar3 = *puVar2;
      puVar3[1] = uVar4;
      puVar3[2] = uVar5;
      return param_3;
    }
  }
  uStack_8 = 0x102;
  uStack_4 = FUN_003df590(0xffffffff80000003,0x70af50);
  FUN_003df4d0(&uStack_8);
  return 0;
}
