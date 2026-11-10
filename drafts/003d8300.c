/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003d8300 func_003d8300 src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003d8300_arg2 fn_003d8300_arg2;
struct fn_003d8300_arg2 {
    unsigned char pad_0x0[0x4];
    int field_0x4; /* +0x4 */
    int field_0x8; /* +0x8 */
    unsigned char pad_0xc[0xc];
    int field_0x18; /* +0x18 */
};


/* WARNING: Type propagation algorithm not settling */

int FUN_003d8300(unsigned long long param_1,unsigned long long param_2,fn_003d8300_arg2 *param_3)

{
  int iVar1;
  unsigned char *puVar2;
  int iVar3;
  int iVar4;
  int uStack_34;
  int uStack_30;
  int iStack_2c;
  int iStack_28;
  unsigned char uStack_24;
  unsigned char uStack_23;
  unsigned char uStack_22;
  unsigned char uStack_21;
  int uStack_20;
  int uStack_1c;
  int uStack_18;
  int uStack_14;
  int iStack_10;
  int aiStack_c [2];
  unsigned short uStack_4;
  unsigned short uStack_2;
  
  iVar4 = 0x28;
  aiStack_c[1] = 0x28;
  FUN_003df240(param_1,aiStack_c + 1,4);
  aiStack_c[0] = param_3->field_0x4;
  FUN_003df240(param_1,aiStack_c,4);
  iStack_10 = param_3->field_0x8;
  FUN_003df240(param_1,&iStack_10,4);
  uStack_2 = 1;
  FUN_003df270(param_1,&uStack_2,2);
  uStack_4 = (unsigned short)param_2;
  FUN_003df270(param_1,&uStack_4,2);
  uStack_14 = 0;
  FUN_003df240(param_1,&uStack_14,4);
  uStack_18 = 0;
  FUN_003df240(param_1,&uStack_18,4);
  uStack_1c = 0;
  FUN_003df240(param_1,&uStack_1c,4);
  uStack_20 = 0;
  FUN_003df240(param_1,&uStack_20,4);
  if (param_2 < 9) {
    puVar2 = (unsigned char *)param_3->field_0x18;
    iVar1 = 1 << ((unsigned int)param_2 & 0x1f);
    iStack_28 = iVar1;
    FUN_003df240(param_1,&iStack_28,4);
    iStack_2c = iVar1;
    FUN_003df240(param_1,&iStack_2c,4);
    iVar3 = 0;
    if (0 < iVar1) {
      do {
        uStack_24 = puVar2[2];
        uStack_23 = puVar2[1];
        uStack_22 = *puVar2;
        uStack_21 = 0xff;
        FUN_003e2ab0(param_1,&uStack_24,4);
        iVar3 = iVar3 + 1;
        iVar4 = iVar4 + 4;
        puVar2 = puVar2 + 4;
      } while (iVar3 < iVar1);
    }
  }
  else {
    uStack_30 = 0;
    FUN_003df240(param_1,&uStack_30,4);
    uStack_34 = 0;
    FUN_003df240(param_1,&uStack_34,4);
  }
  return iVar4;
}
