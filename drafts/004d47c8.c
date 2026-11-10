/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x004d47c8 adxt_GetTime src/cri/adx_tlk.c - romwright draft, unreviewed */
typedef struct fn_004d47c8_arg0 fn_004d47c8_arg0;
struct fn_004d47c8_arg0 {
    unsigned char pad_0x0[0x1];
    byte field_0x1; /* +0x1 */
    unsigned char pad_0x2[0x2];
    int field_0x4; /* +0x4 */
    unsigned char pad_0x8[0x6a];
    char field_0x72; /* +0x72 */
    unsigned char pad_0x73[0x15];
    int field_0x88; /* +0x88 */
    unsigned char pad_0x8c[0x10];
    int field_0x9c; /* +0x9c */
    int field_0xa0; /* +0xa0 */
};


void FUN_004d47c8(fn_004d47c8_arg0 *param_1,long param_2,long param_3)

{
  int iVar1;
  long lVar2;
  int *piVar3;
  float fVar4;
  int iStack_50;
  int aiStack_4c [3];
  
  if (((param_1 == (fn_004d47c8_arg0 *)0x0) || (param_2 == 0)) || (param_3 == 0)) {
    FUN_004c6a98(0x758c80);
    return;
  }
  if (DAT_00724e64 == 0) {
    FUN_004d4630();
    return;
  }
  DAT_00724eb4 = 0.0f;
  piVar3 = (int *)param_2;
  if (param_1->field_0x1 - 3 < 2) {
    if (param_1->field_0x72 != '\0') {
      iVar1 = param_1->field_0x9c;
    }
    else {
      iVar1 = (DAT_007188b8 - param_1->field_0xa0) * 100 + param_1->field_0x9c;
    }
    *piVar3 = iVar1;
    FUN_004d4630(param_1,(long)(int)&iStack_50,aiStack_4c);
    iVar1 = DAT_00724e64;
    DAT_00724eb4 = ((float)iStack_50 / (float)aiStack_4c[0] - (float)*piVar3 / (float)DAT_00724eb0)
                   * 1000.0f;
    if ((DAT_00724eb4 <= 60.0f) && (-60.0f <= DAT_00724eb4)) {
      iVar1 = param_1->field_0x88;
      *piVar3 = *piVar3 + iVar1;
      *(int *)param_3 = DAT_00724eb0;
      return;
    }
    if (DAT_00724e88 == 1) {
      DAT_00724e64 = 0;
      FUN_004d47c8(param_1,(long)(int)&iStack_50,(long)(int)aiStack_4c);
      DAT_00724e64 = iVar1;
      FUN_004d4760();
    }
    fVar4 = ((float)iStack_50 / (float)aiStack_4c[0]) * (float)DAT_00724eb0;
    if (2.1474836e+09f > fVar4) {
      param_1->field_0x9c = (int)fVar4;
    }
    else {
      param_1->field_0x9c = (int)(fVar4 - 2.1474836e+09f) | 0x80000000;
    }
    param_1->field_0xa0 = DAT_007188b8;
  }
  else if (param_1->field_0x1 != 5) {
    *piVar3 = 0;
  }
  else {
    iStack_50 = FUN_004cee08(param_1->field_0x4);
    aiStack_4c[0] = FUN_004ced90(param_1->field_0x4);
    lVar2 = FUN_004cedc0(param_1->field_0x4);
    if (lVar2 == 0) {
      trap(7);
    }
    iVar1 = (int)(((float)(iStack_50 * (0x10 / (int)lVar2)) / (float)aiStack_4c[0]) *
                 (float)DAT_00724eb0);
    *piVar3 = iVar1;
    *piVar3 = iVar1 + param_1->field_0x9c + 1;
  }
  iVar1 = param_1->field_0x88;
  *piVar3 = *piVar3 + iVar1;
  *(int *)param_3 = DAT_00724eb0;
  return;
}
