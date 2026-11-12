/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x0039ba80 PDSRegistryCreatePipes src/promoted/code1_0039.c - romwright draft, unreviewed */
typedef struct fn_0039ba80_arg0 fn_0039ba80_arg0;
struct fn_0039ba80_arg0 {
    int field_0x0; /* +0x0 */
    int field_0x4; /* +0x4 */
    unsigned char pad_0x8[0x4];
    undefined4 field_0xc; /* +0xc */
};


long FUN_0039ba80(fn_0039ba80_arg0 *param_1)

{
  unsigned int uVar1;
  unsigned int uVar2;
  unsigned int *puVar3;
  int iVar4;
  unsigned int uVar5;
  
  uVar5 = 0;
  uVar2 = 0;
  if (param_1->field_0x4 != 0) {
    iVar4 = 0;
    do {
      puVar3 = (unsigned int *)(param_1->field_0x0 + iVar4);
      if (puVar3[3] == 1) {
        uVar1 = FUN_0039bbd0(*puVar3);
        puVar3[4] = uVar1;
        *(unsigned int *)(puVar3[4] + 0x30) = puVar3[2];
      }
      uVar2 = param_1->field_0x4;
      uVar5 = uVar5 + 1;
      iVar4 = iVar4 + 0x14;
    } while (uVar5 < uVar2);
  }
  uVar5 = 0;
  if (uVar2 != 0) {
    iVar4 = 0;
    do {
      FUN_0039b720(param_1->field_0x0 + iVar4,(long)(int)param_1);
      uVar5 = uVar5 + 1;
      iVar4 = iVar4 + 0x14;
    } while (uVar5 < (unsigned int)param_1->field_0x4);
  }
  param_1->field_0xc = 1;
  return (long)(int)param_1;
}
