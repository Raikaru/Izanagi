/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* 0x003dc610 RtQuatConvertFromMatrix src/promoted/code1_003d.c - romwright draft, unreviewed */
typedef struct fn_003dc610_arg1 fn_003dc610_arg1;
struct fn_003dc610_arg1 {
    float field_0x0; /* +0x0 */
    float field_0x4; /* +0x4 */
    float field_0x8; /* +0x8 */
    unsigned char pad_0xc[0x4];
    float field_0x10; /* +0x10 */
    float field_0x14; /* +0x14 */
    float field_0x18; /* +0x18 */
    unsigned char pad_0x1c[0x4];
    float field_0x20; /* +0x20 */
    float field_0x24; /* +0x24 */
    float field_0x28; /* +0x28 */
};


unsigned char FUN_003dc610(long param_1,fn_003dc610_arg1 *param_2)

{
  code *pcVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  
  pfVar2 = (float *)param_1;
  if (param_1 != 0 && param_2 != (fn_003dc610_arg1 *)0x0) {
    fVar5 = param_2->field_0x14;
    fVar6 = param_2->field_0x0;
    fVar3 = param_2->field_0x28;
    fVar4 = fVar3 + fVar6 + fVar5;
    if (fVar4 <= 0.0f) {
      if (fVar5 < fVar6) {
        if (fVar3 < fVar6) {
          pcVar1 = FUN_003dc490;
        }
        else {
          pcVar1 = FUN_003dc590;
        }
      }
      else if (fVar3 < fVar5) {
        pcVar1 = FUN_003dc510;
      }
      else {
        pcVar1 = FUN_003dc590;
      }
      (*pcVar1)();
    }
    else {
      fVar3 = SQRT(fVar4 + 1.0f);
      fVar7 = 0.5f / fVar3;
      pfVar2[3] = fVar3 * 0.5f;
      fVar3 = param_2->field_0x4;
      fVar4 = param_2->field_0x10;
      fVar5 = param_2->field_0x20;
      fVar6 = param_2->field_0x8;
      *pfVar2 = fVar7 * (param_2->field_0x18 - param_2->field_0x24);
      pfVar2[1] = fVar7 * (fVar5 - fVar6);
      pfVar2[2] = fVar7 * (fVar3 - fVar4);
    }
  }
  return param_1 != 0 && param_2 != (fn_003dc610_arg1 *)0x0;
}
