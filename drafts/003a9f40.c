/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003a9f40 func_003a9f40 src/promoted/code1_003a.c - romwright draft, unreviewed */
typedef struct fn_003a9f40_arg1 fn_003a9f40_arg1;
struct fn_003a9f40_arg1 {
    unsigned char pad_0x0[0xc];
    int field_0xc; /* +0xc */
    unsigned char pad_0x10[0x14];
    int field_0x24; /* +0x24 */
    unsigned char pad_0x28[0x30];
    int field_0x58; /* +0x58 */
    int field_0x5c; /* +0x5c */
    int field_0x60; /* +0x60 */
    int field_0x64; /* +0x64 */
};


unsigned long long FUN_003a9f40(int param_1,fn_003a9f40_arg1 *param_2,unsigned long long param_3)

{
  unsigned int *puVar1;
  unsigned int uVar2;
  int iVar3;
  int uStack_8;
  int uStack_4;
  
  puVar1 = *(unsigned int **)(*(int *)(param_1 + DAT_00764700) + 0x9c);
  if (param_2->field_0x64 == 0) {
    uVar2 = param_2->field_0x58;
  }
  else {
    (*DAT_00887300)(0xc,1);
    (*DAT_00887304)(10,&uStack_4);
    (*DAT_00887300)(10,param_2->field_0x5c);
    (*DAT_00887304)(0xb,&uStack_8);
    (*DAT_00887300)(0xb,param_2->field_0x60);
    uVar2 = param_2->field_0x58;
  }
  if ((uVar2 & 2) != 0) {
    FUN_003a93b0(*puVar1,param_2->field_0xc,param_3,param_2->field_0x24);
    iVar3 = param_2->field_0x64;
  }
  else {
    iVar3 = FUN_003e9700(*(unsigned int *)(puVar1[2] + 4));
    FUN_003a14f0(puVar1[1],puVar1[2]);
    FUN_003a9880(*puVar1,param_2->field_0xc,param_3,iVar3 + 0x20);
    iVar3 = param_2->field_0x64;
  }
  if (iVar3 != 0) {
    (*DAT_00887300)(10,uStack_4);
    (*DAT_00887300)(0xb,uStack_8);
  }
  return 0;
}
